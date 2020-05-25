#include "ofxFFmpeg/OpenGL.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
}

#if defined _WIN32
#include "libavcodec/dxva2.h"
#include "libavcodec/d3d11va.h"
#include "libavutil/hwcontext_dxva2.h"
#include "libavutil/hwcontext_d3d11va.h"
#include "GL/glew.h"
#include "GL/wglew.h"

typedef struct {
	HANDLE hDevice;
	IDirect3DDevice9 * pDevice;
	HANDLE gl_device;
} DXVA2_OPENGL_DEVICE;

typedef struct {
	DXVA2_OPENGL_DEVICE * device;
	IDirect3DSurface9 * renderTarget;
	HANDLE gl_texture;
} DXVA2_OPENGL_RENDERER;

#elif defined __APPLE__
#include "libavcodec/videotoolbox.h"
#elif defined __linux__
#include "libavcodec/vaapi.h"
#endif

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool OpenGLDevice::open() {

	if (!HardwareDevice::open())
		return false;

	AVHWDeviceContext * device_ctx = (AVHWDeviceContext*)hwdevice_context->data;

#if defined _WIN32
	if (device_ctx->type == AV_HWDEVICE_TYPE_DXVA2) {
		AVDXVA2DeviceContext * dxva2_ctx = (AVDXVA2DeviceContext*)device_ctx->hwctx;
		HRESULT hr;
		DXVA2_OPENGL_DEVICE * device = new DXVA2_OPENGL_DEVICE();
		device_ctx->user_opaque = device;

		hr = dxva2_ctx->devmgr->OpenDeviceHandle(&device->hDevice);
		if (hr != D3D_OK)
			return false;

		hr = dxva2_ctx->devmgr->LockDevice(device->hDevice, &device->pDevice, FALSE);
		if (hr != D3D_OK)
			return false;

		device->gl_device = wglDXOpenDeviceNV(device->pDevice);
		if (device->gl_device == 0)
			return false;

		return true;
	}
#endif

	return false;
}

//--------------------------------------------------------------
void ofxFFmpeg::OpenGLDevice::close() {

	if (hwdevice_context) {
		AVHWDeviceContext * device_ctx = (AVHWDeviceContext*)hwdevice_context->data;

		if (device_ctx) {

#if defined _WIN32
			if (device_ctx->type == AV_HWDEVICE_TYPE_DXVA2) {
				DXVA2_OPENGL_DEVICE * device = (DXVA2_OPENGL_DEVICE*)device_ctx->user_opaque;

				wglDXCloseDeviceNV(device->gl_device);

				AVDXVA2DeviceContext * dxva2_ctx = (AVDXVA2DeviceContext*)device_ctx->hwctx;
				dxva2_ctx->devmgr->UnlockDevice(device->hDevice, FALSE);

				dxva2_ctx->devmgr->CloseDeviceHandle(device->hDevice);

				delete device;
				device_ctx->user_opaque = NULL;
			}
#endif
		}
	}

	HardwareDevice::close();
}

//--------------------------------------------------------------
bool OpenGLRenderer::open(OpenGLDevice & hardware, int width, int height) {

	close();

	this->hardware = &hardware;

	AVBufferRef * device_ctx_ref = hardware.getContext();
	if (!device_ctx_ref)
		return false;

	AVHWDeviceContext * device_ctx = (AVHWDeviceContext*)device_ctx_ref->data;
	if (!device_ctx)
		return false;

#if defined _WIN32
	if (device_ctx->type == AV_HWDEVICE_TYPE_DXVA2) {

		DXVA2_OPENGL_DEVICE * device = (DXVA2_OPENGL_DEVICE*)device_ctx->user_opaque;
		if (!device)
			return false;

		HRESULT hr;
		IDirect3D9 * d3d;
		hr = device->pDevice->GetDirect3D(&d3d);
		hr = d3d->CheckDeviceFormatConversion(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2'), D3DFMT_A8R8G8B8);
		if (hr != D3D_OK)
			return false;

		DXVA2_OPENGL_RENDERER * renderer = new DXVA2_OPENGL_RENDERER();
		renderer->device = device;
		hr = device->pDevice->CreateRenderTarget(
			width, height,
			D3DFMT_A8R8G8B8,
			D3DMULTISAMPLE_NONE,
			0, FALSE,
			&renderer->renderTarget,
			NULL
		);
		if (hr != D3D_OK)
			return false;

		this->target = GL_TEXTURE_RECTANGLE;
		this->width = width;
		this->height = height;

		glGenTextures(1, &texture);

		renderer->gl_texture = wglDXRegisterObjectNV(device->gl_device, renderer->renderTarget, texture, target, WGL_ACCESS_READ_ONLY_NV);
		if (!renderer->gl_texture)
			return false;

		data = renderer;
		return true;
	}
#endif

	return false;
}

//--------------------------------------------------------------
void OpenGLRenderer::close() {

	if (data && hardware) {

#if defined _WIN32
		if (hardware->getType() == AV_HWDEVICE_TYPE_DXVA2) {
			DXVA2_OPENGL_RENDERER * renderer = (DXVA2_OPENGL_RENDERER*)data;

			wglDXUnregisterObjectNV(renderer->device->gl_device, renderer->gl_texture);

			renderer->renderTarget->Release();
			delete renderer;
		}
#endif

		data = NULL;
	}

	if (texture) {
		glDeleteTextures(1, &texture);
		texture = 0;
	}

	hardware = NULL;
}

//--------------------------------------------------------------
void OpenGLRenderer::render(AVFrame * frame) {
	AVHWFramesContext * frames_ctx = (AVHWFramesContext*)frame->hw_frames_ctx->data;

#if defined _WIN32
	if (frames_ctx->device_ctx->type == AV_HWDEVICE_TYPE_DXVA2) {

		DXVA2_OPENGL_DEVICE * device = (DXVA2_OPENGL_DEVICE*)frames_ctx->device_ctx->user_opaque;
		DXVA2_OPENGL_RENDERER * renderer = (DXVA2_OPENGL_RENDERER*)data;

		if (frame->format == AV_PIX_FMT_DXVA2_VLD) {
			IDirect3DSurface9 * surface = (IDirect3DSurface9*)frame->data[3];
			HRESULT hr;
			hr = device->pDevice->BeginScene();
			hr = device->pDevice->StretchRect(surface, NULL, renderer->renderTarget, NULL, D3DTEXF_NONE);
			hr = device->pDevice->EndScene();
		}
	}
#endif
}

//--------------------------------------------------------------
void ofxFFmpeg::OpenGLRenderer::lock() {
#if defined _WIN32
	if (hardware && hardware->getType() == AV_HWDEVICE_TYPE_DXVA2) {
		DXVA2_OPENGL_RENDERER * renderer = (DXVA2_OPENGL_RENDERER*)data;
		if (renderer) {
			wglDXLockObjectsNV(renderer->device->gl_device, 1, &renderer->gl_texture);
		}
	}
#endif
}

//--------------------------------------------------------------
void ofxFFmpeg::OpenGLRenderer::unlock() {
#if defined _WIN32
	if (hardware && hardware->getType() == AV_HWDEVICE_TYPE_DXVA2) {
		DXVA2_OPENGL_RENDERER * renderer = (DXVA2_OPENGL_RENDERER*)data;
		if (renderer) {
			wglDXUnlockObjectsNV(renderer->device->gl_device, 1, &renderer->gl_texture);
		}
	}
#endif
}

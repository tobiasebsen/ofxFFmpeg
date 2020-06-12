#include "ofxFFmpeg/OpenGL.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
}

#include "GL/glew.h"

#if defined _WIN32
#include "libavcodec/dxva2.h"
#include "libavcodec/d3d11va.h"
#include "libavutil/hwcontext_dxva2.h"
#include "libavutil/hwcontext_d3d11va.h"
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

typedef struct {
	HANDLE gl_device;
} D3D11_OPENGL_DEVICE;

#elif defined __APPLE__
#include "libavcodec/videotoolbox.h"
#include "libavutil/hwcontext_videotoolbox.h"

typedef struct {
    IOSurfaceRef surface;
} VT_OPENGL_RENDERER;

#elif defined __linux__
#include "libavcodec/vaapi.h"
#endif

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool OpenGLDevice::open(HardwareDevice & hardware) {

	if (!hardware.isOpen())
		return false;

	hwdevice_context_ref = av_buffer_ref(hardware.getContextRef());
	hwdevice_context = (AVHWDeviceContext*)hwdevice_context_ref->data;

#if defined _WIN32
	if (hwdevice_context->type == AV_HWDEVICE_TYPE_DXVA2) {
		AVDXVA2DeviceContext * dxva2_ctx = (AVDXVA2DeviceContext*)hwdevice_context->hwctx;
		HRESULT hr;
		DXVA2_OPENGL_DEVICE * device = new DXVA2_OPENGL_DEVICE();
		hwdevice_context->user_opaque = device;

		hr = dxva2_ctx->devmgr->OpenDeviceHandle(&device->hDevice);
		if (hr != D3D_OK) {
			close();
			return false;
		}

		hr = dxva2_ctx->devmgr->LockDevice(device->hDevice, &device->pDevice, FALSE);
		if (hr != D3D_OK) {
			close();
			return false;
		}

		device->gl_device = wglDXOpenDeviceNV(device->pDevice);
		if (device->gl_device == 0) {
			close();
			return false;
		}

		return true;
	}
	if (hwdevice_context->type == AV_HWDEVICE_TYPE_D3D11VA) {
		/*AVD3D11VADeviceContext * d3d11_ctx = (AVD3D11VADeviceContext*)hwdevice_context->hwctx;
		D3D11_OPENGL_DEVICE * device = new D3D11_OPENGL_DEVICE();
		hwdevice_context->user_opaque = device;

		device->gl_device = wglDXOpenDeviceNV(d3d11_ctx->device);
		if (device->gl_device == 0)
			return false;

		return true;*/
	}
#elif defined __APPLE__
    if (hwdevice_context->type == AV_HWDEVICE_TYPE_VIDEOTOOLBOX) {
        return true;
    }
#endif

	av_buffer_unref(&hwdevice_context_ref);
	hwdevice_context = NULL;

	return false;
}

//--------------------------------------------------------------
bool OpenGLDevice::isOpen() const {
	return hwdevice_context_ref != NULL;
}

//--------------------------------------------------------------
void OpenGLDevice::close() {

	if (hwdevice_context_ref) {

#if defined _WIN32
		if (hwdevice_context->type == AV_HWDEVICE_TYPE_DXVA2) {
			DXVA2_OPENGL_DEVICE * device = (DXVA2_OPENGL_DEVICE*)hwdevice_context->user_opaque;

			wglDXCloseDeviceNV(device->gl_device);

			AVDXVA2DeviceContext * dxva2_ctx = (AVDXVA2DeviceContext*)hwdevice_context->hwctx;
			dxva2_ctx->devmgr->UnlockDevice(device->hDevice, FALSE);

			dxva2_ctx->devmgr->CloseDeviceHandle(device->hDevice);

			delete device;
			hwdevice_context->user_opaque = NULL;
		}
		if (hwdevice_context->type == AV_HWDEVICE_TYPE_D3D11VA) {
			D3D11_OPENGL_DEVICE * device = (D3D11_OPENGL_DEVICE*)hwdevice_context->user_opaque;

			wglDXCloseDeviceNV(device->gl_device);

			delete device;
			hwdevice_context->user_opaque = NULL;
		}
#elif defined __APPLE__
#endif
		av_buffer_unref(&hwdevice_context_ref);
		hwdevice_context = NULL;
	}
}

//--------------------------------------------------------------
int OpenGLDevice::getHardwareType() {
	return hwdevice_context ? hwdevice_context->type : AV_HWDEVICE_TYPE_NONE;
}

//--------------------------------------------------------------
AVBufferRef * OpenGLDevice::getContextRef() {
	return hwdevice_context_ref;
}

//--------------------------------------------------------------
bool OpenGLRenderer::open(OpenGLDevice & opengl, int w, int h, int target) {

	close();

	if (!opengl.isOpen())
		return false;

    for (int i=0; i<4; i++) {
        this->width[i] = w;
        this->height[i] = h;
    }
    this->target = target;

    hwdevice_context_ref = av_buffer_ref(opengl.getContextRef());
	hwdevice_context = (AVHWDeviceContext*)hwdevice_context_ref->data;

#if defined _WIN32
	if (opengl.getHardwareType() == AV_HWDEVICE_TYPE_DXVA2) {

		DXVA2_OPENGL_DEVICE * device = (DXVA2_OPENGL_DEVICE*)hwdevice_context->user_opaque;
		if (!device)
			return false;

		HRESULT hr;
		IDirect3D9 * d3d;
		hr = device->pDevice->GetDirect3D(&d3d);
		hr = d3d->CheckDeviceFormatConversion(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2'), D3DFMT_A8R8G8B8);
		if (hr != D3D_OK) {
			return false;
		}

		DXVA2_OPENGL_RENDERER * renderer = new DXVA2_OPENGL_RENDERER();
		this->opaque = renderer;
		renderer->device = device;
		HANDLE shareHandle = NULL;
		hr = device->pDevice->CreateRenderTarget(
			width[0], height[0],
			D3DFMT_X8R8G8B8,
			D3DMULTISAMPLE_NONE,
			0, FALSE,
			&renderer->renderTarget,
			&shareHandle
		);
		if (hr != D3D_OK) {
			close();
			return false;
		}

		BOOL ret = wglDXSetResourceShareHandleNV(renderer->renderTarget, shareHandle);

        pix_fmt = AV_PIX_FMT_RGB24;
        planes = 1;
		glGenTextures(planes, textures);
        formats[0] = GL_RGB;

		renderer->gl_texture = wglDXRegisterObjectNV(device->gl_device, renderer->renderTarget, textures[0], target, WGL_ACCESS_READ_WRITE_NV);
		if (renderer->gl_texture == NULL) {
			DWORD error = GetLastError();
			close();
			return false;
		}

		return true;
	}
	if (opengl.getHardwareType() == AV_HWDEVICE_TYPE_D3D11VA) {

		D3D11_OPENGL_DEVICE * device = (D3D11_OPENGL_DEVICE*)hwdevice_context->user_opaque;
		AVD3D11VADeviceContext * d3d11_ctx = (AVD3D11VADeviceContext*)hwdevice_context->hwctx;
		HRESULT hr;

		D3D11_TEXTURE2D_DESC desc;
		desc.Width = width[0];
		desc.Height = height[0];
		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		ID3D11Texture2D * texture = NULL;
		hr = d3d11_ctx->device->CreateTexture2D(&desc, NULL, &texture);
		if (hr != S_OK)
			return false;

		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		renderTargetViewDesc.Format = desc.Format;
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice = 0;

		ID3D11Texture2D* renderTargetTextureMap;
		ID3D11RenderTargetView* renderTargetViewMap;
		d3d11_ctx->device->CreateRenderTargetView(renderTargetTextureMap, &renderTargetViewDesc, &renderTargetViewMap);
	}
#elif defined __APPLE__
    if (opengl.getHardwareType() == AV_HWDEVICE_TYPE_VIDEOTOOLBOX) {
        
        VT_OPENGL_RENDERER * renderer = new VT_OPENGL_RENDERER;
        this->opaque = renderer;
        renderer->surface = nil;

        pix_fmt = AV_PIX_FMT_NV12;
        planes = 2;
        glGenTextures(planes, textures);
        formats[0] = GL_RED;
        formats[1] = GL_RG;
        width[1] = w / 2;
        height[1] = h / 2;

        return true;
    }
#endif

    close();

    return false;
}

//--------------------------------------------------------------
bool OpenGLRenderer::isOpen() const {
	return hwdevice_context_ref != NULL;
}

//--------------------------------------------------------------
void OpenGLRenderer::close() {

	if (hwdevice_context && opaque) {

#if defined _WIN32
		if (hwdevice_context->type == AV_HWDEVICE_TYPE_DXVA2) {
			DXVA2_OPENGL_RENDERER * renderer = (DXVA2_OPENGL_RENDERER*)opaque;

			wglDXUnregisterObjectNV(renderer->device->gl_device, renderer->gl_texture);

			renderer->renderTarget->Release();
			delete renderer;
		}
#elif defined __APPLE__
        if (hwdevice_context->type == AV_HWDEVICE_TYPE_VIDEOTOOLBOX) {
            VT_OPENGL_RENDERER * renderer = (VT_OPENGL_RENDERER*)opaque;
            IOSurfaceDecrementUseCount(renderer->surface);
            delete renderer;
        }
#endif

        pix_fmt = AV_PIX_FMT_NONE;

        if (planes) {
            glDeleteTextures(planes, textures);
            planes = 0;
        }

		av_buffer_unref(&hwdevice_context_ref);
		hwdevice_context = NULL;
		opaque = NULL;
	}
}

//--------------------------------------------------------------
void OpenGLRenderer::render(AVFrame * frame) {
	AVHWFramesContext * frames_ctx = (AVHWFramesContext*)frame->hw_frames_ctx->data;

#if defined _WIN32
	if (frames_ctx->device_ctx->type == AV_HWDEVICE_TYPE_DXVA2) {

		DXVA2_OPENGL_DEVICE * device = (DXVA2_OPENGL_DEVICE*)frames_ctx->device_ctx->user_opaque;
		if (!device)
			return;

		DXVA2_OPENGL_RENDERER * renderer = (DXVA2_OPENGL_RENDERER*)opaque;

		if (frame->format == AV_PIX_FMT_DXVA2_VLD) {
			IDirect3DSurface9 * surface = (IDirect3DSurface9*)frame->data[3];
			HRESULT hr;
			RECT rect = { 0, 0, width[0], height[0] };
			hr = device->pDevice->BeginScene();
			hr = device->pDevice->StretchRect(surface, &rect, renderer->renderTarget, &rect, D3DTEXF_NONE);
			hr = device->pDevice->EndScene();
		}
	}
	if (frames_ctx->device_ctx->type == AV_HWDEVICE_TYPE_D3D11VA) {

		if (frame->format == AV_PIX_FMT_D3D11VA_VLD) {
			ID3D11VideoDecoderOutputView * outputView = (ID3D11VideoDecoderOutputView*)frame->data[3];
		}
		if (frame->format == AV_PIX_FMT_D3D11) {
			ID3D11Texture2D * texture = (ID3D11Texture2D*)frame->data[0];
			intptr_t arrayIndex = (intptr_t)frame->data[1];

		}
	}
#elif defined __APPLE__
    if (frames_ctx->device_ctx->type == AV_HWDEVICE_TYPE_VIDEOTOOLBOX) {
        CVPixelBufferRef pixbuf = (CVPixelBufferRef)frame->data[3];
        IOSurfaceRef surface = CVPixelBufferGetIOSurface(pixbuf);
        VT_OPENGL_RENDERER * renderer = (VT_OPENGL_RENDERER*)opaque;
        CGLContextObj cgl_ctx = CGLGetCurrentContext();

        if (surface != renderer->surface) {
            size_t p = IOSurfaceGetPlaneCount(surface);
            p = std::min(std::min(p, planes), (size_t)4);
            
            IOSurfaceDecrementUseCount(renderer->surface);
            renderer->surface = surface;
            IOSurfaceIncrementUseCount(surface);

            for (size_t i=0; i<p; i++) {
                size_t w = IOSurfaceGetWidthOfPlane(surface, i);
                size_t h = IOSurfaceGetHeightOfPlane(surface, i);
                glBindTexture(target, textures[i]);
                CGLError error = CGLTexImageIOSurface2D(cgl_ctx, target, formats[i], w, h, formats[i], GL_UNSIGNED_BYTE, surface, i);
                while(0);
            }
            glBindTexture(target, 0);
        }
    }
#endif
}

//--------------------------------------------------------------
void OpenGLRenderer::lock() {
#if defined _WIN32
	if (hwdevice_context && hwdevice_context->type == AV_HWDEVICE_TYPE_DXVA2) {
		DXVA2_OPENGL_RENDERER * renderer = (DXVA2_OPENGL_RENDERER*)opaque;
		if (renderer) {
			wglDXLockObjectsNV(renderer->device->gl_device, 1, &renderer->gl_texture);
		}
	}
#endif
}

//--------------------------------------------------------------
void OpenGLRenderer::unlock() {
#if defined _WIN32
	if (hwdevice_context && hwdevice_context->type == AV_HWDEVICE_TYPE_DXVA2) {
		DXVA2_OPENGL_RENDERER * renderer = (DXVA2_OPENGL_RENDERER*)opaque;
		if (renderer) {
			wglDXUnlockObjectsNV(renderer->device->gl_device, 1, &renderer->gl_texture);
		}
	}
#endif
}

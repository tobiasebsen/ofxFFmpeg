#include "ofxFFmpeg/Hardware.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
}

#if defined _WIN32
#include "libavcodec/dxva2.h"
#include "libavcodec/d3d11va.h"
#include "libavutil/hwcontext_dxva2.h"
#include "libavutil/hwcontext_d3d11va.h"
#include "GL/glew.h"
#include "GL/wglew.h"
#elif defined __APPLE__
#include "libavcodec/videotoolbox.h"
#elif defined __linux__
#include "libavcodec/vaapi.h"
#endif

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool HardwareDevice::open(int device_type) {

	close();

	if (device_type < 0)
		device_type = getDefaultType();

	error = av_hwdevice_ctx_create(&hwdevice_context_ref, (AVHWDeviceType)device_type, NULL, NULL, 0);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create hardware context\n");
		return false;
	}
	this->hwdevice_context = (AVHWDeviceContext*)hwdevice_context_ref->data;

	return true;
}

//--------------------------------------------------------------
bool HardwareDevice::open(std::string device_name) {
	AVHWDeviceType type = av_hwdevice_find_type_by_name(device_name.c_str());
	if (type == AV_HWDEVICE_TYPE_NONE) {
		return false;
	}
	return open(type);
}

//--------------------------------------------------------------
void HardwareDevice::close() {
	if (hwdevice_context_ref) {
		//av_buffer_unref(&hwdevice_context);
		hwdevice_context_ref = NULL;
	}
}

//--------------------------------------------------------------
bool HardwareDevice::isOpen() {
	return hwdevice_context_ref != NULL;
}

//--------------------------------------------------------------
int HardwareDevice::getType() const {
	return hwdevice_context ? hwdevice_context->type : AV_HWDEVICE_TYPE_NONE;
}

//--------------------------------------------------------------
std::string HardwareDevice::getName() {
	return getName(getType());
}

//--------------------------------------------------------------
int HardwareDevice::getPixelFormat(const AVCodec * codec) {
	const AVCodecHWConfig *config = getConfig(codec);
	return config != NULL ? config->pix_fmt : AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
const AVCodecHWConfig * HardwareDevice::getConfig(const AVCodec * codec) const {
	AVHWDeviceType type = hwdevice_context ? hwdevice_context->type : AV_HWDEVICE_TYPE_NONE;
	const AVCodecHWConfig *config = NULL;
	for (int i = 0; ; i++) {
		config = avcodec_get_hw_config(codec, i);
		if (config == NULL) break;
		if (config->device_type == type)
			return config;
	}
	return NULL;
}

//--------------------------------------------------------------
AVBufferRef * HardwareDevice::getContextRef() const {
	return hwdevice_context_ref;
}

//--------------------------------------------------------------
std::vector<int> HardwareDevice::getFormats() {
	std::vector<int> formats;
	AVHWFramesConstraints * constraints = av_hwdevice_get_hwframe_constraints(hwdevice_context_ref, NULL);
	if (constraints != NULL) {
		for (int i = 0; constraints->valid_sw_formats[i] != AV_PIX_FMT_NONE; i++) {
			formats.push_back(constraints->valid_sw_formats[i]);
		}
		av_hwframe_constraints_free(&constraints);
	}
	return formats;
}

//--------------------------------------------------------------
bool HardwareDevice::transfer(AVFrame * hw_frame, AVFrame * sw_frame) {
	int error;
	error = av_hwframe_transfer_data(sw_frame, hw_frame, 0);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Hardware transfer failed.\n");
		return false;
	}
	sw_frame->pts = hw_frame->pts;
	sw_frame->pkt_duration = hw_frame->pkt_duration;
	return true;
}

//--------------------------------------------------------------
AVFrame * HardwareDevice::transfer(AVFrame * hw_frame) {
	AVFrame * sw_frame = av_frame_alloc();
	if (!transfer(hw_frame, sw_frame)) {
		free(sw_frame);
		return NULL;
	}
	return sw_frame;
}

//--------------------------------------------------------------
void HardwareDevice::free(AVFrame * frame) {
	av_frame_unref(frame);
	av_frame_free(&frame);
}

//--------------------------------------------------------------
std::vector<int> HardwareDevice::getTypes() {
    std::vector<int> types;

    AVHWDeviceType t = AV_HWDEVICE_TYPE_NONE;
    do {
        t = av_hwdevice_iterate_types(t);
        if (t != AV_HWDEVICE_TYPE_NONE) {
			types.push_back(t);
        }
    } while (t != AV_HWDEVICE_TYPE_NONE);

    return types;
}

//--------------------------------------------------------------
std::vector<std::string> HardwareDevice::getTypeNames() {
	std::vector<std::string> names;
	std::vector<int> types = getTypes();

	for (int t : types) {
		std::string n = av_hwdevice_get_type_name((AVHWDeviceType)t);
		names.push_back(n);
	}
	return names;
}

//--------------------------------------------------------------
int HardwareDevice::getNumHardwareConfig(const AVCodec * codec) {
	const AVCodecHWConfig *config = NULL;
	for (int i=0; ; i++) {
		config = avcodec_get_hw_config(codec, i);
		if (config == NULL) return i;
	}
	return 0;
}

//--------------------------------------------------------------
int HardwareDevice::getDefaultType() {
#if defined _WIN32
	return AV_HWDEVICE_TYPE_DXVA2;
	//return AV_HWDEVICE_TYPE_D3D11VA;
	//return AV_HWDEVICE_TYPE_CUDA;
#elif defined __APPLE__
	return AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
#elif defined __linux__
	return AV_HWDEVICE_TYPE_OPENCL;
#else
	return AV_HWDEVICE_TYPE_NONE;
#endif
}

//--------------------------------------------------------------
std::string HardwareDevice::getName(int device_type) {
	if (device_type <= 0)
		device_type = getDefaultType();
	return av_hwdevice_get_type_name((AVHWDeviceType)device_type);
}

//--------------------------------------------------------------
static enum AVPixelFormat get_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
	AVHWDeviceContext * hw_device_ctx = (AVHWDeviceContext*)ctx->hw_device_ctx->data;
	ctx->hw_frames_ctx = av_hwframe_ctx_alloc(ctx->hw_device_ctx);
	AVHWFramesContext * hw_frames_ctx = (AVHWFramesContext*)ctx->hw_frames_ctx->data;

	for (auto p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {

		if (hw_device_ctx->type == AV_HWDEVICE_TYPE_DXVA2 && *p == AV_PIX_FMT_DXVA2_VLD) {

			hw_frames_ctx->format = AV_PIX_FMT_DXVA2_VLD;
			hw_frames_ctx->sw_format = AV_PIX_FMT_NV12; //ctx->sw_pix_fmt;
			hw_frames_ctx->width = ctx->coded_width;
			hw_frames_ctx->height = ctx->coded_height;
			hw_frames_ctx->initial_pool_size = 20;

			int error = av_hwframe_ctx_init(ctx->hw_frames_ctx);
			if (error < 0)
				return AV_PIX_FMT_NONE;

			return AV_PIX_FMT_DXVA2_VLD;
		}

	}
	return AV_PIX_FMT_NONE;
}


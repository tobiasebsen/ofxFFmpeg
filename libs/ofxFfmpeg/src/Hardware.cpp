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
HardwareDevice::HardwareDevice() {
	device_type = AV_HWDEVICE_TYPE_NONE;
}

//--------------------------------------------------------------
bool HardwareDevice::open(int device_type) {

	close();

	if (device_type < 0)
		device_type = getDefaultType();

	error = av_hwdevice_ctx_create(&hwdevice_context, (AVHWDeviceType)device_type, NULL, NULL, 0);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create hardware context\n");
		return false;
	}

	this->device_type = device_type;

	return true;
}

//--------------------------------------------------------------
void HardwareDevice::close() {
	if (hwdevice_context) {
		av_buffer_unref(&hwdevice_context);
	}
}

//--------------------------------------------------------------
bool HardwareDevice::isOpen() {
	return hwdevice_context != NULL;
}

//--------------------------------------------------------------
int ofxFFmpeg::HardwareDevice::getType() const {
	return device_type;
}

//--------------------------------------------------------------
std::string HardwareDevice::getName() {
	return getName(device_type);
}

//--------------------------------------------------------------
int HardwareDevice::getPixelFormat(const AVCodec * codec) {
	const AVCodecHWConfig *config = getConfig(codec);
	return config != NULL ? config->pix_fmt : AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
const AVCodecHWConfig * HardwareDevice::getConfig(const AVCodec * codec) const {
	const AVCodecHWConfig *config = NULL;
	for (int i = 0; ; i++) {
		config = avcodec_get_hw_config(codec, i);
		if (config == NULL) break;
		if (config->device_type == device_type)
			return config;
	}
	return NULL;
}

//--------------------------------------------------------------
AVBufferRef * HardwareDevice::getContext() const {
	return hwdevice_context;
}

//--------------------------------------------------------------
std::vector<int> HardwareDevice::getFormats() {
	std::vector<int> formats;
	AVHWFramesConstraints * constraints = av_hwdevice_get_hwframe_constraints(hwdevice_context, NULL);
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
    std::vector<int> deviceTypes;

    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    do {
        type = av_hwdevice_iterate_types(type);
        if (type != AV_HWDEVICE_TYPE_NONE) {
			deviceTypes.push_back(type);
        }
    } while (type != AV_HWDEVICE_TYPE_NONE);

    return deviceTypes;
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

//--------------------------------------------------------------
bool HardwareDecoder::open(Reader & reader, HardwareDevice & device) {

	int stream_index = reader.getVideoStreamIndex();
	if (stream_index < 0)
		return false;

	AVCodec * codec = reader.getVideoCodec();
	AVStream * stream = reader.getStream(stream_index);

	if (!Decoder::allocate(codec, stream))
		return false;

	hw_config = device.getConfig(codec);
	if (hw_config) {
		if (hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
			codec_context->hw_device_ctx = av_buffer_ref(device.getContext());
		//if (hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX)
		//	codec_context->get_format = get_format;
	}

	if (!Decoder::open(codec))
		return false;

	return true;
}

//--------------------------------------------------------------
bool HardwareDecoder::isHardwareFrame(AVFrame * frame) {
	return hw_config && (frame->format == hw_config->pix_fmt);
}

//--------------------------------------------------------------
bool HardwareDecoder::hasHardwareDecoder() {
	return codec_context && (codec_context->hw_device_ctx != NULL);
}

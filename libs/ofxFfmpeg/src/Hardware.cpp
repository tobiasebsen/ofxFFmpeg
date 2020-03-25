#include "ofxFFmpeg/HardwareDecoder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
ofxFFmpeg::HardwareDecoder::HardwareDecoder() {
	device_type = AV_HWDEVICE_TYPE_NONE;
}

//--------------------------------------------------------------
bool HardwareDecoder::open(int device_type) {

	close();

	if (device_type < 0)
		device_type = getDefaultDeviceType();

	error = av_hwdevice_ctx_create(&hw_context, (AVHWDeviceType)device_type, NULL, NULL, 0);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create hardware context\n");
		return false;
	}

	this->device_type = device_type;
	
	return true;
}

//--------------------------------------------------------------
void HardwareDecoder::close() {
	if (hw_context) {
		av_buffer_unref(&hw_context);
	}
}

//--------------------------------------------------------------
bool ofxFFmpeg::HardwareDecoder::isOpen() {
	return hw_context != NULL;
}

//--------------------------------------------------------------
std::string ofxFFmpeg::HardwareDecoder::getDeviceName() {
	return getDeviceName(device_type);
}

//--------------------------------------------------------------
int ofxFFmpeg::HardwareDecoder::getPixelFormat(const AVCodec * codec) {
	const AVCodecHWConfig *config = getConfig(codec);
	return config != NULL ? config->pix_fmt : AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
const AVCodecHWConfig * HardwareDecoder::getConfig(const AVCodec * codec) const {
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
AVBufferRef * ofxFFmpeg::HardwareDecoder::getContext() const {
	return hw_context;
}

//--------------------------------------------------------------
std::vector<int> HardwareDecoder::getFormats() {
	std::vector<int> formats;
	AVPixelFormat * pfmts = NULL;
	error = av_hwframe_transfer_get_formats(hw_context, AV_HWFRAME_TRANSFER_DIRECTION_TO, &pfmts, 0);
	if (error < 0) {
		return formats;
	}
	AVPixelFormat * p = pfmts;

	while (*pfmts != AV_PIX_FMT_NONE) {
		formats.push_back(*pfmts);
		pfmts++;
	}
	av_freep(&p);

	return formats;
}

//--------------------------------------------------------------
bool HardwareDecoder::transfer(AVFrame * hw_frame, AVFrame * sw_frame) {
	int error = av_hwframe_transfer_data(sw_frame, hw_frame, 0);
	return error >= 0;
}

//--------------------------------------------------------------
AVFrame * HardwareDecoder::transfer(AVFrame * hw_frame) {
	AVFrame * sw_frame = av_frame_alloc();
	if (!transfer(hw_frame, sw_frame)) {
		free(sw_frame);
		return NULL;
	}
	return sw_frame;
}

//--------------------------------------------------------------
void HardwareDecoder::free(AVFrame * frame) {
	av_frame_unref(frame);
}

//--------------------------------------------------------------
std::vector<int> HardwareDecoder::getDeviceTypes() {
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
int HardwareDecoder::getNumHardwareConfig(const AVCodec * codec) {
	const AVCodecHWConfig *config = NULL;
	for (int i=0; ; i++) {
		config = avcodec_get_hw_config(codec, i);
		if (config == NULL) return i;
	}
	return 0;
}

//--------------------------------------------------------------
int HardwareDecoder::getDefaultDeviceType() {
#if defined _WIN32
	//return AV_HWDEVICE_TYPE_D3D11VA;
	return AV_HWDEVICE_TYPE_DXVA2;
#elif defined __APPLE__
	return AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
#elif defined __linux__
	return AV_HWDEVICE_TYPE_OPENCL;
#else
	return AV_HWDEVICE_TYPE_NONE;
#endif
}

//--------------------------------------------------------------
std::string HardwareDecoder::getDeviceName(int device_type) {
	if (device_type <= 0)
		device_type = getDefaultDeviceType();
	return av_hwdevice_get_type_name((AVHWDeviceType)device_type);
}

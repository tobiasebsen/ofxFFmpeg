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
bool HardwareDecoder::open(Reader & reader) {

	if (!VideoDecoder::open(reader))
		return false;

	hw_format = -1;
	sw_format = -1;

#if defined _WIN32
	AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_DXVA2;
#elif defined __APPLE__
	AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
#else
	AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;
#endif

	int i = 0;
	const AVCodecHWConfig *config = NULL;
	do {
		config = avcodec_get_hw_config(codec_context->codec, i);
		if (config && config->device_type == hw_type) {
			hw_format = config->pix_fmt;
		}
		i++;
	} while (config != NULL);

	error = av_hwdevice_ctx_create(&hardware_context, hw_type, NULL, NULL, 0);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create hardware context\n");
	}
	else {
		codec_context->hw_device_ctx = av_buffer_ref(hardware_context);
	}

	/*if (hardwareAccel) {
		AVPixelFormat * formats = NULL;
		codec_context->hw_frames_ctx = av_hwframe_ctx_alloc(hardware_context);
		av_hwframe_ctx_init(codec_context->hw_frames_ctx);
		error = av_hwframe_transfer_get_formats(codec_context->hw_frames_ctx, AV_HWFRAME_TRANSFER_DIRECTION_TO, &formats, 0);
		if (error >= 0) {
			while (*formats != AV_PIX_FMT_NONE) {
				sw_format = *formats;
				formats++;
			}
			av_freep(&formats);
		}
	}*/
	return true;
}

//--------------------------------------------------------------
void HardwareDecoder::close() {
	VideoDecoder::close();

	if (hardware_context) {
		av_buffer_unref(&hardware_context);
	}
}

//--------------------------------------------------------------
bool HardwareDecoder::decode(AVPacket * packet, FrameReceiver * receiver) {
	if (send(packet)) {

		AVFrame * frame = NULL;
		while ((frame = receive()) != NULL) {

			if (frame->format == hw_format) {
				AVFrame * sw_frame = av_frame_alloc();
				error = av_hwframe_transfer_data(sw_frame, frame, 0);
				receiver->receive(sw_frame, stream->index);
				av_frame_free(&sw_frame);
			}
			else {
				receiver->receive(frame, stream->index);
			}

			free(frame);
		}
		return true;
	}
	return false;
}

//--------------------------------------------------------------
int HardwareDecoder::getPixelFormat() const {
	return AV_PIX_FMT_NV12;
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
int ofxFFmpeg::HardwareDecoder::getNumHardwareConfig(const AVCodec * codec) {
	const AVCodecHWConfig *config = NULL;
	for (int i=0; ; i++) {
		config = avcodec_get_hw_config(codec, i);
		if (config == NULL) return i;
	}
	return 0;
}

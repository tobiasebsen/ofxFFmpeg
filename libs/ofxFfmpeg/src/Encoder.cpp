#include "Encoder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

bool Encoder::setup(AVCodec * codec) {

	close();

	codec_context = avcodec_alloc_context3(codec);
	if (!codec_context) {
		av_log(NULL, AV_LOG_ERROR, "Could not allocate an encoding context\n");
		return false;
	}

	if (codec->pix_fmts)
		codec_context->pix_fmt = codec->pix_fmts[0];

	return true;
}

bool ofxFFmpeg::Encoder::setup(int codecId) {

	AVCodec * codec;

	/** Find the encoder to be used by its name. */
	if (!(codec = avcodec_find_encoder((enum AVCodecID)codecId))) {
		av_log(NULL, AV_LOG_ERROR, "Could not find the encoder\n");
		return false;
	}

	return setup(codec);
}

bool ofxFFmpeg::Encoder::setup(std::string codecName) {

	AVCodec * codec;

	/** Find the encoder to be used by its name. */
	if (!(codec = avcodec_find_encoder_by_name(codecName.c_str()))) {
		av_log(NULL, AV_LOG_ERROR, "Could not find the encoder\n");
		return false;
	}

	return setup(codec);
}

void ofxFFmpeg::Encoder::close() {
	if (codec_context) {
		avcodec_free_context(&codec_context);
		codec_context = NULL;
	}
}

bool ofxFFmpeg::Encoder::encode(AVFrame * frame, AVPacket * packet) {

	error = avcodec_send_frame(codec_context, frame);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cound not send frame\n");
		return false;
	}

	av_init_packet(packet);

	return false;
}

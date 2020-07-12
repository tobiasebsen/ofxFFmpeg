#include "ofxFFmpeg/Encoder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool Encoder::allocate(int codecId) {

	AVCodec * codec;

	/** Find the encoder to be used by its name. */
	if (!(codec = avcodec_find_encoder((enum AVCodecID)codecId))) {
		av_log(NULL, AV_LOG_ERROR, "Could not find the encoder\n");
		return false;
	}

	return Codec::allocate(codec);
}

//--------------------------------------------------------------
bool Encoder::allocate(std::string codecName) {

	AVCodec * codec;

	/** Find the encoder to be used by its name. */
	if (!(codec = avcodec_find_encoder_by_name(codecName.c_str()))) {
		av_log(NULL, AV_LOG_ERROR, "Could not find the encoder\n");
		return false;
	}

	return Codec::allocate(codec);
}

//--------------------------------------------------------------
bool Encoder::open(AVStream * stream) {

	stream->avg_frame_rate = context->framerate;
	stream->r_frame_rate = context->framerate;
	stream->time_base = av_inv_q(context->framerate);
	context->time_base = av_inv_q(context->framerate);

	//if (output_format_context->flags & AVFMT_GLOBALHEADER)
	context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (!Codec::open(stream)) {
		return false;
	}

	if ((error = avcodec_parameters_from_context(stream->codecpar, context)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream\n");
		return false;
	}

	return true;
}

//--------------------------------------------------------------
bool Encoder::encode(AVFrame * frame, PacketReceiver * receiver) {

	if (!isOpen())
		return false;

	metrics.begin();

	error = avcodec_send_frame(context, frame);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cound not send frame\n");
		return false;
	}

	AVPacket packet;
	av_init_packet(&packet);

	while (error >= 0) {
		error = avcodec_receive_packet(context, &packet);
		if (error >= 0) {
			packet.stream_index = stream->index;

			av_packet_rescale_ts(&packet, context->time_base, stream->time_base);

			receiver->receive(&packet);

			av_packet_unref(&packet);
		}
	}

	metrics.end();

	return true;
}

//--------------------------------------------------------------
void Encoder::flush(PacketReceiver * receiver) {
	encode(NULL, receiver);
}

const Metrics & Encoder::getMetrics() const {
	return metrics;
}

//--------------------------------------------------------------
bool VideoEncoder::open(AVStream * stream, HardwareDevice & hardware) {
	return false;
}

//--------------------------------------------------------------
int64_t VideoEncoder::rescaleFrameNum(int64_t frame_num) {
	return av_rescale_q(frame_num, context->time_base, stream->time_base);
}

//--------------------------------------------------------------
int64_t AudioEncoder::rescaleSampleCount(int64_t nb_samples) {
	return av_rescale_q(nb_samples, context->time_base, stream->time_base);
}

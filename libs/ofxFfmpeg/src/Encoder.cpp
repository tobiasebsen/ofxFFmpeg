#include "ofxFFmpeg/Encoder.h"

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

	codec_context->gop_size = 15;

	return true;
}

bool Encoder::setup(int codecId) {

	AVCodec * codec;

	/** Find the encoder to be used by its name. */
	if (!(codec = avcodec_find_encoder((enum AVCodecID)codecId))) {
		av_log(NULL, AV_LOG_ERROR, "Could not find the encoder\n");
		return false;
	}

	return setup(codec);
}

bool Encoder::setup(std::string codecName) {

	AVCodec * codec;

	/** Find the encoder to be used by its name. */
	if (!(codec = avcodec_find_encoder_by_name(codecName.c_str()))) {
		av_log(NULL, AV_LOG_ERROR, "Could not find the encoder\n");
		return false;
	}

	return setup(codec);
}

void Encoder::close() {
	if (codec_context) {
		avcodec_free_context(&codec_context);
		codec_context = NULL;
	}
}

void Encoder::freeFrame(AVFrame * frame) {
	if (frame) {
		av_frame_free(&frame);
	}
}

bool Encoder::begin(AVStream * stream) {

	this->stream = stream;

	codec_context->time_base = av_inv_q(codec_context->framerate);
	stream->avg_frame_rate = codec_context->framerate;

	//if (output_format_context->flags & AVFMT_GLOBALHEADER)
	codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if ((error = avcodec_open2(codec_context, codec_context->codec, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream\n");
		return false;
	}

	if ((error = avcodec_parameters_from_context(stream->codecpar, codec_context)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream\n");
		return false;
	}

	return true;
}

void Encoder::end() {

}

bool Encoder::encode(AVFrame * frame, PacketReceiver * receiver) {

	error = avcodec_send_frame(codec_context, frame);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cound not send frame\n");
		return false;
	}

	AVPacket packet;
	av_init_packet(&packet);
	packet.stream_index = stream->index;
	packet.pos = -1;

	while (error >= 0) {
		error = avcodec_receive_packet(codec_context, &packet);
		if (error >= 0) {
			receiver->receivePacket(&packet);
			av_packet_unref(&packet);
		}
	}
	return true;
}

//--------------------------------------------------------------
void Encoder::flush(PacketReceiver * receiver) {
	encode(NULL, receiver);
}

//--------------------------------------------------------------
AVFrame * VideoEncoder::allocateFrame() {
	AVFrame * frame = av_frame_alloc();
	frame->width = codec_context->width;
	frame->height = codec_context->height;
	frame->format = codec_context->pix_fmt;
	frame->pts = 0;

	if ((error = av_frame_get_buffer(frame, 32)) < 0) {
		av_frame_free(&frame);
		return NULL;
	}
	return frame;
}

//--------------------------------------------------------------
void VideoEncoder::setTimeStamp(AVFrame * frame, int frame_num) {
	frame->pts = frame_num;// av_rescale_q(frame_num, av_inv_q(codec_context->framerate), stream->time_base);
}

//--------------------------------------------------------------
void VideoEncoder::setTimeStamp(AVFrame * frame, double time_sec) {
	frame->pts = time_sec / av_q2d(stream->time_base);
}

//--------------------------------------------------------------
void ofxFFmpeg::VideoEncoder::setTimeStamp(AVPacket * packet) {
	packet->pts = av_rescale_q(packet->pts, codec_context->time_base, stream->time_base);
	packet->dts = av_rescale_q(packet->dts, codec_context->time_base, stream->time_base);
}

//--------------------------------------------------------------
void VideoEncoder::setWidth(int width) {
	codec_context->width = width;
}
//--------------------------------------------------------------
void VideoEncoder::setHeight(int height) {
	codec_context->height = height;
}
//--------------------------------------------------------------
void VideoEncoder::setFrameRate(float frameRate) {
	codec_context->framerate = av_d2q(frameRate, 1000);
}
//--------------------------------------------------------------
void VideoEncoder::setBitRate(int bitRate) {
	codec_context->bit_rate = bitRate;
}
//--------------------------------------------------------------
void VideoEncoder::setMaxBitRate(int bitRate) {
	codec_context->rc_min_rate = codec_context->bit_rate;
	codec_context->rc_max_rate = bitRate;
}
//--------------------------------------------------------------
void ofxFFmpeg::VideoEncoder::setBufferSize(int bufferSize) {
	codec_context->rc_buffer_size = bufferSize;
}
//--------------------------------------------------------------
void VideoEncoder::setProfile(int profile) {
	codec_context->profile = profile;
}
//--------------------------------------------------------------
void VideoEncoder::setLevel(int level) {
	codec_context->level = level;
}
//--------------------------------------------------------------
void VideoEncoder::setKeyFrame(int key_frame) {
	codec_context->gop_size = key_frame;
}

int VideoEncoder::getWidth() {
	return codec_context->width;
}

int VideoEncoder::getHeight() {
	return codec_context->height;
}

int VideoEncoder::getPixelFormat() {
	return codec_context->pix_fmt;
}

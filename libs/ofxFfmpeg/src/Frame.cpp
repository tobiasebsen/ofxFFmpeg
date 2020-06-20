#include "ofxFFmpeg/Frame.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
Frame Frame::allocate() {
	return Frame(av_frame_alloc());
}
//--------------------------------------------------------------
void Frame::free(Frame & f) {
	av_frame_free(&f.frame);
}
//--------------------------------------------------------------
void Frame::free(AVFrame * f) {
	av_frame_free(&f);
}
//--------------------------------------------------------------
uint8_t * Frame::getData(int plane) {
	return frame->data[plane];
}
//--------------------------------------------------------------
int64_t Frame::getTimeStamp() const {
	return frame->pts;
}
//--------------------------------------------------------------
void Frame::setTimeStamp(int64_t pts) {
	frame->pts = pts;
}
//--------------------------------------------------------------
VideoFrame VideoFrame::allocate(int width, int height, int pix_fmt) {
	AVFrame * frame = av_frame_alloc();
	frame->width = width;
	frame->height = height;
	frame->format = pix_fmt;
	frame->pts = 0;

	int error = av_frame_get_buffer(frame, 0);

	return VideoFrame(frame);
}
//--------------------------------------------------------------
int VideoFrame::getWidth() const {
	return frame->width;
}
//--------------------------------------------------------------
int VideoFrame::getHeight() const {
	return frame->height;
}
//--------------------------------------------------------------
int VideoFrame::getPixelFormat() const {
	return frame->format;
}
//--------------------------------------------------------------
int VideoFrame::getLineSize(int plane) const {
	return frame->linesize[plane];
}
//--------------------------------------------------------------
AudioFrame AudioFrame::allocate(int nb_samples, int nb_channels, int sample_fmt) {
	AVFrame * frame = av_frame_alloc();
	frame->nb_samples = nb_samples;
	frame->channel_layout = av_get_default_channel_layout(nb_channels);
	frame->format = sample_fmt;
	frame->pts = 0;

	av_frame_get_buffer(frame, 0);

	return AudioFrame(frame);
}

//--------------------------------------------------------------
int AudioFrame::getNumSamples() const {
	return frame->nb_samples;
}

//--------------------------------------------------------------
void AudioFrame::setNumSamples(int nb_samples) {
	frame->nb_samples = nb_samples;
}

//--------------------------------------------------------------
int64_t Packet::getTimeStamp() const {
	return packet->pts;
}

//--------------------------------------------------------------
void Packet::setTimeStamp(int64_t pts) {
	packet->pts = pts;
}

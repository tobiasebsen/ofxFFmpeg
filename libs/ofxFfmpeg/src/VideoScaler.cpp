#include "ofxFFmpeg/VideoScaler.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool VideoScaler::allocate(int src_width, int src_height, int src_fmt, int dst_width, int dst_height, int dst_fmt) {
	free();
	sws_context = sws_getContext(src_width, src_height, (enum AVPixelFormat)src_fmt, dst_width, dst_height, (enum AVPixelFormat)dst_fmt, 0, 0, 0, 0);
	return sws_context != NULL;
}

//--------------------------------------------------------------
bool VideoScaler::allocate(int width, int height, int src_fmt, int dst_fmt) {
	return allocate(width, height, src_fmt, width, height, dst_fmt);
}

//--------------------------------------------------------------
bool VideoScaler::allocate(VideoDecoder & decoder) {
    return allocate(decoder.getWidth(), decoder.getHeight(), decoder.getPixelFormat(), AV_PIX_FMT_RGB24);
}

//--------------------------------------------------------------
bool VideoScaler::allocate(VideoEncoder & encoder) {
	return allocate(encoder.getWidth(), encoder.getHeight(), AV_PIX_FMT_RGB24, encoder.getPixelFormat());
}

//--------------------------------------------------------------
bool VideoScaler::isAllocated() {
	return sws_context != NULL;
}

//--------------------------------------------------------------
void VideoScaler::free() {
	if (sws_context) {
		sws_freeContext(sws_context);
		sws_context = NULL;
	}
}

//--------------------------------------------------------------
bool VideoScaler::scale(AVFrame *frame, const uint8_t *pixelData, int line_stride) {
	if (!isAllocated()) {
		av_log(NULL, AV_LOG_ERROR, "Video scaler not allocated\n");
		return false;
	}

     const int out_linesize[1] = { line_stride };
	 metrics.begin();
     int ret = sws_scale(sws_context, frame->data, frame->linesize, 0, (int)frame->height, (uint8_t * const *)&pixelData, out_linesize);
	 metrics.end();
     return true;
}

//--------------------------------------------------------------
bool VideoScaler::scale(const uint8_t * imageData, int line_stride, int height, AVFrame * frame) {
	if (!isAllocated()) {
		av_log(NULL, AV_LOG_ERROR, "Video scaler not allocated\n");
		return false;
	}

	const int in_linesize[1] = { line_stride };
	metrics.begin();
	sws_scale(sws_context, (const uint8_t * const *)&imageData, in_linesize, 0, height, frame->data, frame->linesize);
	metrics.end();
	return true;
}

//--------------------------------------------------------------
void VideoScaler::copy(AVFrame * src_frame, uint8_t * dst_data, int dst_size, int align) {
	error = av_image_copy_to_buffer(dst_data, dst_size, src_frame->data, src_frame->linesize, (AVPixelFormat)src_frame->format, src_frame->width, src_frame->height, align);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot copy image to buffer\n");
	}
}

//--------------------------------------------------------------
void VideoScaler::copyPlane(AVFrame * src_frame, int plane, uint8_t * dst_data, int dst_linesize, int height) {
	int bytewidth = av_image_get_linesize((AVPixelFormat)src_frame->format, src_frame->width, plane);
	av_image_copy_plane(dst_data, dst_linesize, src_frame->data[plane], src_frame->linesize[plane], bytewidth, height);
}

//--------------------------------------------------------------
uint8_t * ofxFFmpeg::VideoScaler::getData(AVFrame * frame) {
	return frame->data[0];
}

//--------------------------------------------------------------
void VideoScaler::start() {
}

//--------------------------------------------------------------
void VideoScaler::stop() {
}

//--------------------------------------------------------------
void VideoScaler::scaleThread() {

	while (running) {

	}
}

//--------------------------------------------------------------
bool VideoScaler::supportsInput(int src_format) {
	return sws_isSupportedInput((AVPixelFormat)src_format);
}

//--------------------------------------------------------------
bool VideoScaler::supportsOutput(int dst_format) {
	return sws_isSupportedOutput((AVPixelFormat)dst_format);
}

//--------------------------------------------------------------
Metrics & VideoScaler::getMetrics() {
	return metrics;
}

//--------------------------------------------------------------
const Metrics & VideoScaler::getMetrics() const {
	return metrics;
}

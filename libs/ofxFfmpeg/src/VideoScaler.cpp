#include "ofxFFmpeg/VideoScaler.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

bool ofxFFmpeg::VideoScaler::setup(int src_width, int src_height, int src_fmt, int dst_width, int dst_height, int dst_fmt) {
	clear();
	sws_context = sws_getContext(src_width, src_height, (enum AVPixelFormat)src_fmt, dst_width, dst_height, (enum AVPixelFormat)dst_fmt, 0, 0, 0, 0);
	return sws_context != NULL;
}

bool ofxFFmpeg::VideoScaler::setup(int width, int height, int src_fmt, int dst_fmt) {
	return setup(width, height, src_fmt, width, height, dst_fmt);
}

bool VideoScaler::setup(VideoDecoder & decoder) {
    return setup(decoder.getWidth(), decoder.getHeight(), decoder.getPixelFormat(), AV_PIX_FMT_RGB24);
}

bool ofxFFmpeg::VideoScaler::setup(VideoEncoder & encoder) {
	return setup(encoder.getWidth(), encoder.getHeight(), AV_PIX_FMT_RGB24, encoder.getPixelFormat());
}

bool ofxFFmpeg::VideoScaler::isSetup() {
	return sws_context != NULL;
}

void VideoScaler::clear() {
	if (sws_context) {
		sws_freeContext(sws_context);
		sws_context = NULL;
	}
}

bool VideoScaler::scale(AVFrame *frame, const uint8_t *pixelData, int line_stride) {
     const int out_linesize[1] = { line_stride };
     int ret = sws_scale(sws_context, frame->data, frame->linesize, 0, (int)frame->height, (uint8_t * const *)&pixelData, out_linesize);
     return true;
}

bool ofxFFmpeg::VideoScaler::scale(const uint8_t * imageData, int line_stride, int height, AVFrame * frame) {
	const int in_linesize[1] = { line_stride };
	sws_scale(sws_context, (const uint8_t * const *)&imageData, in_linesize, 0, height, frame->data, frame->linesize);
	return true;
}


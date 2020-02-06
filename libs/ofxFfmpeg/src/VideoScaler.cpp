#include "VideoScaler.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

void VideoScaler::setup(int width, int height, int pix_fmt) {
    sws_context = sws_getContext(width, height, (enum AVPixelFormat)pix_fmt, width, height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);
}

void VideoScaler::setup(Decoder & decoder) {
    setup(decoder.getWidth(), decoder.getHeight(), decoder.getPixelFormat());
}

bool VideoScaler::scale(AVFrame *frame, const uint8_t *pixelData) {
     const int out_linesize[1] = { 3 * frame->width };
     sws_scale(sws_context, frame->data, frame->linesize, 0, (int)frame->height, (uint8_t * const *)&pixelData, out_linesize);
     return true;
}


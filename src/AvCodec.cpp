#include "AvCodec.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

using namespace ofxFFmpeg;
using namespace std;

AvCodec::AvCodec(AVCodecContext * context, AVCodec * codec) {
    this->context = context;
    this->codec = codec;
    context->pix_fmt = codec->pix_fmts[0];
}

AvCodec::~AvCodec() {

    avcodec_free_context(&context);
}

void AvCodec::setWidth(size_t width) {
    context->width = width;
}

void AvCodec::setHeight(size_t height) {
    context->height = height;
}

void AvCodec::setFrameRate(int frameRate) {
	context->time_base.num = 1;
	context->time_base.den = frameRate;
	context->framerate.num = frameRate;
	context->framerate.den = 1;
}

void AvCodec::setPixelFormat(int pixelFormat) {
    context->pix_fmt = (AVPixelFormat)pixelFormat;
}

vector<int> AvCodec::getPixelFormats() {
    vector<int> pix_fmts;
    const AVPixelFormat * pix_fmt_arr = codec->pix_fmts;
    while (pix_fmt_arr != NULL && pix_fmt_arr[0] != -1) {
        int pix_fmt = pix_fmt_arr[0];
        pix_fmts.push_back(pix_fmt);
        pix_fmt_arr++;
    }
    return pix_fmts;
}

bool AvCodec::open() {
    return avcodec_open2(context, codec, NULL) == 0;
}

AvFramePtr AvCodec::allocFrame() {
    AVFrame * frame = av_frame_alloc();
    if (!frame)
        return NULL;

    frame->width = context->width;
    frame->height = context->height;
    frame->format = context->pix_fmt;
    frame->pts = 0;
    av_frame_get_buffer(frame, 32);

    return AvFramePtr(new AvFrame(frame));
}

bool AvCodec::encode(AvFramePtr frame) {

    int ret;
    ret = avcodec_send_frame(context, frame->getFrame());
    if (ret < 0)
        return false;
    return true;
}

bool AvCodec::encode() {
    
    int ret;
    ret = avcodec_send_frame(context, NULL);
    if (ret < 0)
        return false;
    return true;
}

bool AvCodec::receivePacket(AvPacket & pkt) {
    int ret;
    ret = avcodec_receive_packet(context, pkt.getPacket());
    return ret >= 0;
}

#include "ofxFFmpeg/Queue.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

using namespace ofxFFmpeg;

AVPacket * PacketQueue::clone(AVPacket *p) {
    return av_packet_clone(p);
}

void PacketQueue::free(AVPacket *p) {
    av_packet_unref(p);
}

AVFrame * ofxFFmpeg::FrameQueue::clone(AVFrame * f) {
	return av_frame_clone(f);
}

void ofxFFmpeg::FrameQueue::free(AVFrame * f) {
	av_frame_unref(f);
}

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

/*AVPacket * PacketQueue::supplyPacket() {
    return Queue<AVPacket>::pop();
}*/

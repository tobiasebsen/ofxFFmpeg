#include "AvPacket.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

using namespace ofxFFmpeg;

AvPacket::AvPacket() {
    pkt = av_packet_alloc();
}

AvPacket::~AvPacket() {
    av_packet_free(&pkt);
}

unsigned char * AvPacket::getData() {
    return pkt->data;
}

unsigned int AvPacket::getSize() {
    return pkt->size;
}

void AvPacket::unref() {
    av_packet_unref(pkt);
}

AVPacket * AvPacket::getPacket() {
    return pkt;
}

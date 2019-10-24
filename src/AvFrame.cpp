#include "AvFrame.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

using namespace ofxFFmpeg;

void AvFrame::getBuffer(int align) {
    av_frame_get_buffer(frame, align);
}

void AvFrame::makeWritable() {
    av_frame_make_writable(frame);
}

void AvFrame::setPts(int pts) {
    frame->pts = pts;
}

#include "ofxFFmpeg/Queue.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
AVPacket * PacketQueue::clone(AVPacket *p) {
    return av_packet_clone(p);
}

//--------------------------------------------------------------
void PacketQueue::free(AVPacket *p) {
    av_packet_unref(p);
	av_packet_free(&p);
}

//--------------------------------------------------------------
AVFrame * FrameQueue::supply(int64_t pts) {
	std::lock_guard<std::mutex> lock(mutex);

	if (queue.size() > 0) {
		AVFrame * frame = queue.front();
		if (pts >= frame->pts && pts < frame->pts + frame->pkt_duration) {
			queue.pop_front();
			cond_pop.notify_one();
			return frame;
		}
	}
	return nullptr;
}

//--------------------------------------------------------------
bool FrameQueue::pop(int64_t min_pts, int64_t max_pts) {
	std::lock_guard<std::mutex> lock(mutex);

	if (queue.size() == 0)
		return false;

	AVFrame * frame = queue.front();
	bool in_range = false;
	if (min_pts > max_pts) {
		if (frame->pts >= min_pts || frame->pts <= max_pts)
			in_range = true;
	}
	else {
		if (frame->pts >= min_pts && frame->pts <= max_pts)
			in_range = true;
	}
	if (in_range) {
		free(frame);
		queue.pop_front();
		cond_pop.notify_one();
		return true;
	}
	return false;
}

//--------------------------------------------------------------
size_t FrameQueue::clear(int64_t min_pts, int64_t max_pts) {

	size_t n = 0;
	while (pop(min_pts, max_pts)) {
		n++;
	}
	return n;
}

//--------------------------------------------------------------
AVFrame * FrameQueue::clone(AVFrame * f) {
	return av_frame_clone(f);
}

//--------------------------------------------------------------
void FrameQueue::free(AVFrame * f) {
	av_frame_unref(f);
	av_frame_free(&f);
}

//--------------------------------------------------------------
int64_t FrameQueue::get_head(AVFrame * frame) {
	return frame->pts + frame->pkt_duration;
}

//--------------------------------------------------------------
int64_t FrameQueue::get_tail(AVFrame * frame) {
	return frame->pts;
}

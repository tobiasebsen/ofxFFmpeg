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
AVFrame * ofxFFmpeg::FrameQueue::supply(int64_t pts) {
	std::lock_guard<std::mutex> lock(mutex);
	/*for (AVFrame * frame : queue) {
		if (pts >= frame->pts && pts < frame->pts + frame->pkt_duration) {
			return frame;
		}
	}*/
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
bool ofxFFmpeg::FrameQueue::pop(int64_t min_pts, int64_t max_pts) {
	std::lock_guard<std::mutex> lock(mutex);
	if (queue.size() > 0) {
		AVFrame * frame = queue.front();
		if (frame->pts >= min_pts && frame->pts <= max_pts) {
			free(frame);
			queue.pop_front();
			cond_pop.notify_one();
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------
size_t ofxFFmpeg::FrameQueue::flush(int64_t min_pts, int64_t max_pts) {
	/*std::lock_guard<std::mutex> lock(mutex);
	for (auto it = queue.begin(); it != queue.end(); ) {
		AVFrame * frame = *it;
		if (frame->pts >= min_pts && frame->pts <= max_pts) {
			free(frame);
			it = queue.erase(it);
			cond_pop.notify_one();
		}
		else {
			return;
		}
	}*/
	size_t n = 0;
	while (pop(min_pts, max_pts)) {
		n++;
	}
	return n;
}

//--------------------------------------------------------------
AVFrame * ofxFFmpeg::FrameQueue::clone(AVFrame * f) {
	return av_frame_clone(f);
}

//--------------------------------------------------------------
void ofxFFmpeg::FrameQueue::free(AVFrame * f) {
	av_frame_unref(f);
	av_frame_free(&f);
}

//--------------------------------------------------------------
int64_t ofxFFmpeg::FrameQueue::get_head(AVFrame * frame) {
	return frame->pts + frame->pkt_duration;
}

//--------------------------------------------------------------
int64_t ofxFFmpeg::FrameQueue::get_tail(AVFrame * frame) {
	return frame->pts;
}

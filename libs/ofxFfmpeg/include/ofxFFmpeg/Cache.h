#pragma once

#include <list>
#include <mutex>
#include <atomic>

#include "AvTypes.h"
#include "Queue.h"

namespace ofxFFmpeg {
    
    class FrameCache : public FrameQueue {
    public:
		void receive(AVFrame * frame) { push(clone(frame)); }
		void terminateFrameReceiver() { terminate();  flush(); }
		void resumeFrameReceiver() { resume(); }

		AVFrame * supply(int64_t pts) { AVFrame * f = fetch(pts); flush(pts); return f; }

		AVFrame * fetch(int64_t pts);
		void flush(int64_t pts);
		void flush() { Queue<AVFrame>::flush(); }
    };
}

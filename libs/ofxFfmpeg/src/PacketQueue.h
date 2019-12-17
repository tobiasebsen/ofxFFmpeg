#pragma once

#include <deque>
#include <stdint.h>
#include <mutex>

struct AVPacket;

namespace ofxFFmpeg {

	class PacketQueue {
	public:

		void push(AVPacket * packet);
		AVPacket * pop();
        
        size_t size() {
            return queue.size();
        }

		void wait();
        void notify();

	private:
		std::deque<AVPacket*> queue;
		uint64_t ptsHead = 0;
		uint64_t ptsTail = 0;

		std::mutex lock;
		std::condition_variable condition;
	};

}

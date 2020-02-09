#pragma once

#include <deque>
#include <stdint.h>
#include <mutex>

#include "Reader.h"

struct AVPacket;

namespace ofxFFmpeg {

    class PacketQueue : public PacketReceiver, public PacketSupplier {
	public:
        PacketQueue(size_t size) : maxSize(size) {}

        void receivePacket(AVPacket * packet);
        AVPacket * supplyPacket();

        size_t size() { return queue.size(); }

		void wait();
        void notify();

	private:
		std::deque<AVPacket*> queue;
        size_t maxSize;
		uint64_t ptsHead = 0;
		uint64_t ptsTail = 0;

		std::mutex lock;
		std::condition_variable condition;
	};

}

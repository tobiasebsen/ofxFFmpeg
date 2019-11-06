#pragma once

#include <thread>
#include <mutex>

#include "PacketQueue.h"

struct AVCodecContext;

namespace ofxFFmpeg {

	class VideoThread {
	public:
		VideoThread(PacketQueue & videoPackets) : threadObj(&VideoThread::videoThread, this, std::ref(videoPackets)), running(true) {}
		~VideoThread() {
			stop();
		}
		void stop() {
			running = false;
			condition.notify_all();
			threadObj.join();
		}
		bool isRunning() {
			return running;
		}

		void videoThread(PacketQueue & videoPackets);

	private:
		std::thread threadObj;
		std::mutex lock;
		std::condition_variable condition;
		bool running = true;
	};
}
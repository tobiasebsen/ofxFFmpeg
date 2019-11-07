#pragma once

#include <thread>
#include <mutex>

#include "PacketQueue.h"

struct AVCodecContext;
struct SwsContext;

namespace ofxFFmpeg {

	class VideoThread {
	public:
		VideoThread(AVCodecContext * video_context, PacketQueue & vpackets) : threadObj(&VideoThread::videoThread, this, video_context), running(true), videoPackets(vpackets) {}
		~VideoThread() {
			stop();
		}
		void stop() {
			running = false;
			condition.notify_all();
            videoPackets.notify();
			threadObj.join();
		}
		bool isRunning() {
			return running;
		}

		void videoThread(AVCodecContext * video_context);

	private:
		std::thread threadObj;
		std::mutex lock;
		std::condition_variable condition;
		bool running = true;

        PacketQueue & videoPackets;
        
        SwsContext * sws_context = NULL;
	};
}

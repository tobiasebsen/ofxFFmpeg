#pragma once

#include <thread>
#include <mutex>

#include "PacketQueue.h"

struct AVFormatContext;
struct AVStream;

namespace ofxFFmpeg {

	class Reader {
	public:
		Reader(const char * filename, PacketQueue & videoQueue) : threadObj(&Reader::readThread, this, filename, std::ref(videoQueue) ), running(true) {}
		~Reader() {
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

		void readThread(const char * filename, PacketQueue & videoQueue);

		void read(uint64_t pts);

		int getWidth();
		int getHeight();

		int getTotalNumFrames() const;
		float getDuration() const;

	private:
		std::thread threadObj;
		std::mutex lock;
		std::condition_variable condition;
		bool running;

		AVFormatContext * format_context = NULL;
		AVStream * video_stream = NULL;
	};
}
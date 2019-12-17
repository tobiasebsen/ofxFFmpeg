#pragma once

#include <thread>
#include <mutex>
#include <queue>

#include "PacketQueue.h"
#include "VideoThread.h"

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
            video.reset();
			running = false;
			condition.notify_all();
			threadObj.join();
		}
		bool isRunning() {
			return running;
		}

		void readThread(const char * filename, PacketQueue & videoQueue);
        
        uint64_t getLastVideoTime() {
            return lastVideoPts;
        }

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
        
        uint64_t lastVideoPts;
        
        class Action {
        public:
            enum _type {
                READ,
                SEEK
            } type;
            uint64_t pts;
            Action(_type t, uint64_t p) : type(t), pts(p) {}
        };
        
        std::queue<Action> actions;
        
        std::shared_ptr<VideoThread> video;
	};
}

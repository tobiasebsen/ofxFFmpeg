#pragma once

#include <thread>
#include <mutex>
#include <queue>

#include "AvTypes.h"
#include "Flow.h"

namespace ofxFFmpeg {
    
    class Reader {
	public:
		~Reader() {
            close();
		}

        /////////////////////////////////////////////////

        bool open(std::string filename);
		bool isOpen() const;
        void close();

		/////////////////////////////////////////////////

		bool read(AVPacket * packet);
		bool read(PacketReceiver * receiver);
		AVPacket * read();
        int getStreamIndex(AVPacket * packet);

        void seek(uint64_t pts);

        /////////////////////////////////////////////////

        unsigned int getNumStreams();
        int getVideoStreamIndex();
        int getAudioStreamIndex();
        AVStream * getStream(int stream_index);
        
        /////////////////////////////////////////////////

        bool start(PacketReceiver * receiver);
        void stop(PacketReceiver * receiver);
		void readThread(PacketReceiver * receiver);
        bool isRunning() const {
            return running;
        }

		std::string getName();
		std::string getLongName();
		float getDuration() const;
        uint64_t getBitRate() const;
        double getTimeBase() const;

	protected:
        int error;

        AVFormatContext * format_context = NULL;
        
        std::thread * thread_obj = NULL;
        std::mutex mutex;
        std::condition_variable condition;
        bool running = false;
	};
}

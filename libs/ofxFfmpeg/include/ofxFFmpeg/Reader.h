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
		// OPEN AND CLOSE

        bool open(std::string filename);
		bool isOpen() const;
        void close();

		/////////////////////////////////////////////////
		// READING AND SEEKING

		bool read(AVPacket * packet);
		bool read(PacketReceiver * receiver);
		AVPacket * read();
        int getStreamIndex(AVPacket * packet);

        void seek(uint64_t pts);

		/////////////////////////////////////////////////
        // STREAMS

        unsigned int getNumStreams() const;
        int getVideoStreamIndex();
        int getAudioStreamIndex();
        AVStream * getStream(int stream_index) const;
		AVCodec * getVideoCodec();
		AVCodec * getAudioCodec();

		/////////////////////////////////////////////////
        // THREADING

        bool start(PacketReceiver * receiver);
        void stop();
		void readThread();
        bool isRunning() const {
            return running;
        }

		/////////////////////////////////////////////////
		// ACCESSORS

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
		PacketReceiver * receiver;
	};
}

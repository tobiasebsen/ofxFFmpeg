#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <map>

#include "AvTypes.h"
#include "Flow.h"
#include "Metrics.h"

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
		void free(AVPacket * packet);

		void setLoop(bool loop);
		bool getLoop() const;

        void seek(uint64_t pts);
		void seek(uint64_t pts, PacketReceiver * receiver);

		/////////////////////////////////////////////////
        // THREADING

        bool start(PacketReceiver * receiver);
        void stop();
		void readThread();
        bool isRunning() const {
            return running && thread_obj;
        }

		/////////////////////////////////////////////////
		// STREAMS

		unsigned int getNumStreams() const;
		int getVideoStreamIndex();
		int getAudioStreamIndex();
		int getStreamIndex(AVPacket * packet);
		AVStream * getStream(int stream_index) const;
		AVStream * getVideoStream();
		AVStream * getAudioStream();

		AVCodec * getVideoCodec();
		AVCodec * getAudioCodec();

		/////////////////////////////////////////////////
		// ACCESSORS

		std::string getName();
		std::string getLongName();
		int64_t getDuration() const;
		double getDurationSeconds() const;
		uint64_t getBitRate() const;
        double getTimeBase() const;

		std::map<std::string, std::string> getMetadata();

		const Metrics & getMetrics() const;

	protected:
        int error;

        AVFormatContext * format_context = NULL;
        
        std::thread * thread_obj = NULL;
		std::mutex mutex;
        bool running = false;
		bool loop = true;
		PacketReceiver * receiver;
		int64_t seek_pts = FFMPEG_NOPTS_VALUE;

		Metrics metrics;
	};
}

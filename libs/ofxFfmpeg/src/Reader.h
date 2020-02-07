#pragma once

#include <thread>
#include <mutex>
#include <queue>

struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVPacket;

namespace ofxFFmpeg {
    
    class PacketSupplier {
    public:
        virtual AVPacket * supplyPacket() = 0;
        void free(AVPacket * packet);
    };
    
    class PacketReceiver {
    public:
        virtual bool readyPacket() { return true; }
        virtual void receivePacket(AVPacket *packet) = 0;
    };

    class Reader : public PacketSupplier {
	public:
		~Reader() {
            close();
		}

        /////////////////////////////////////////////////

        bool open(std::string filename);
        void close();

		/////////////////////////////////////////////////

		bool read(AVPacket * packet);
		bool read(PacketReceiver * packet);
		AVPacket * supplyPacket();
        int getStreamIndex(AVPacket * packet);

        /////////////////////////////////////////////////

        unsigned int getNumStreams();
        int getVideoStreamIndex();
        int getAudioStreamIndex();
        AVStream * getStream(int stream_index);

        /////////////////////////////////////////////////

        bool start(PacketReceiver * receiver);
        void stop();
		bool isRunning() {
			return running;
		}
		void readThread(PacketReceiver * receiver);
        void notify();

		float getDuration() const;

	protected:
        int error;

        AVFormatContext * format_context = NULL;
        
        std::thread * threadObj;
        std::mutex lock;
        std::condition_variable condition;
        bool running;
	};
}

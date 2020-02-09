#pragma once

#include <thread>
#include <mutex>

#include "Reader.h"
#include "PacketQueue.h"

struct AVStream;
struct AVCodecContext;
struct AVFrame;

namespace ofxFFmpeg {
    
    class FrameReceiver {
    public:
        virtual void receiveFrame(AVFrame * frame, int stream_index) = 0;
    };

	class Decoder {
	public:

        bool open(AVStream * stream);
        void close();
        
        /////////////////////////////////////////////////

        bool match(AVPacket * packet);
        bool send(AVPacket * packet);

        bool receive(AVFrame * frame);
        AVFrame * receive();
        void free(AVFrame * frame);

        bool decode(AVPacket * packet, FrameReceiver * receiver);
		bool flush(FrameReceiver * receiver);

        /////////////////////////////////////////////////

        bool start(PacketSupplier * supplier, FrameReceiver * receiver);
        void stop();
		void decodeThread(PacketSupplier * supplier, FrameReceiver * receiver);
        bool isRunning() const {
            return running;
        }
        
        /////////////////////////////////////////////////

        int getStreamIndex() const;
        int getTotalNumFrames() const;
        int getBitsPerSample() const;
        uint64_t getBitRate() const;

    protected:
        int error;

		std::thread * threadObj;
		std::mutex lock;
		std::condition_variable condition;
		bool running = false;

        AVStream * stream = NULL;
        AVCodecContext * codec_context = NULL;
	};
    
    class VideoDecoder : public Decoder {
    public:
		bool open(Reader & reader);
        int getWidth() const;
        int getHeight() const;
        int getPixelFormat() const;
    };
    
    class AudioDecoder : public Decoder {
    public:
		bool open(Reader & reader);
		int getNumChannels() const;
        int getSampleRate() const;
        int getFrameSize() const;
    };
}

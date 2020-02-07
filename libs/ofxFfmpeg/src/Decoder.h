#pragma once

#include <thread>
#include <mutex>

#include "Reader.h"
#include "PacketQueue.h"

struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;

namespace ofxFFmpeg {
    
    class FrameReceiver {
    public:
        virtual void receiveFrame(AVFrame * frame) = 0;
    };

	class Decoder {
	public:

        bool open(AVStream * stream);
        void close();
        
        int getStreamIndex() const;
        int getTotalNumFrames() const;
        
        /////////////////////////////////////////////////

        bool match(AVPacket * packet);
        bool send(AVPacket * packet);
        bool receive(AVFrame * frame);
        AVFrame * receive();
        void free(AVFrame * frame);

        bool decode(AVPacket * packet, FrameReceiver * receiver);
        
        /////////////////////////////////////////////////

        bool start(PacketSupplier * supplier, FrameReceiver * receiver);
        void stop();
        bool isRunning() {
            return running;
        }
		void decodeThread(PacketSupplier * supplier, FrameReceiver * receiver);

	protected:
        int error;

		std::thread * threadObj;
		std::mutex lock;
		std::condition_variable condition;
		bool running = false;

        AVStream * stream = NULL;
        AVCodec * codec = NULL;
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
    };
}

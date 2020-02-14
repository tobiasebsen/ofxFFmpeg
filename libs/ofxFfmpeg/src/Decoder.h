#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Flow.h"
#include "Reader.h"
#include "PacketQueue.h"

namespace ofxFFmpeg {
    
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

		void copy(AVFrame * src_frame, uint8_t * dst_data, int dst_size, int align = 1);

        /////////////////////////////////////////////////
        
        bool start(PacketSupplier * supplier, FrameReceiver * receiver);
        void stop();
		void decodeThread(PacketSupplier * supplier, FrameReceiver * receiver);
        bool isRunning() const {
            return running;
        }
        
        /////////////////////////////////////////////////
        
		std::string getName();
		std::string getLongName();
		int getStreamIndex() const;
        int getTotalNumFrames() const;
        int getBitsPerSample() const;
        uint64_t getBitRate() const;
        double getTimeBase() const;
		uint64_t getTimeStamp(AVPacket * frame) const;
		uint64_t getTimeStamp(AVFrame * frame) const;
		uint64_t getTimeStamp(int frame_num) const;
		int getFrameNum(uint64_t pts) const;

    protected:
        int error;

		std::thread * threadObj;
		std::mutex mutex;
		std::condition_variable condition;
		bool running = false;

        AVStream * stream = NULL;
        AVCodecContext * codec_context = NULL;
	};

	/////////////////////////////////////////////////////
    
    class VideoDecoder : public Decoder {
    public:
		bool open(Reader & reader);
        int getWidth() const;
        int getHeight() const;
        int getPixelFormat() const;
		double getFrameRate();
    };
    
	/////////////////////////////////////////////////////
	
	class AudioDecoder : public Decoder {
    public:
		bool open(Reader & reader);
		int getNumChannels() const;
        int getSampleRate() const;
        int getFrameSize() const;
    };
}

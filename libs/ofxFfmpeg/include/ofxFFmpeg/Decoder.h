#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Flow.h"
#include "Reader.h"
#include "Queue.h"

namespace ofxFFmpeg {
    
	class Decoder {
	public:

		/////////////////////////////////////////////////
		// OPEN AND CLOSE

        bool open(AVStream * stream);
        void close();
		bool isOpen();
        
        /////////////////////////////////////////////////
		// DECODING

        bool match(AVPacket * packet);
        
		bool send(AVPacket * packet);
        bool receive(AVFrame * frame);
        AVFrame * receive();
        void free(AVFrame * frame);

        bool decode(AVPacket * packet, FrameReceiver * receiver);
		bool flush(FrameReceiver * receiver);

		void copy(AVFrame * src_frame, uint8_t * dst_data, int dst_size, int align = 1);

        /////////////////////////////////////////////////
		// THREADING
        
        bool start(PacketSupplier * supplier, FrameReceiver * receiver);
        void stop();
		void decodeThread();
        bool isRunning() const {
            return running;
        }
        
        /////////////////////////////////////////////////
		// ACCESSORS
        
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

		std::thread * thread_obj = NULL;
		bool running = false;
		PacketSupplier * supplier = NULL;
		FrameReceiver * receiver = NULL;

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
        uint64_t getChannelLayout() const;
        int getSampleRate() const;
        int getSampleFormat() const;
        int getFrameSize() const;
    };
}

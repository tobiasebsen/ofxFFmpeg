#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Flow.h"
#include "Reader.h"
#include "HardwareDecoder.h"
#include "Queue.h"
#include "Metrics.h"

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
        
        virtual bool decode(AVPacket * packet, FrameReceiver * receiver);
		bool flush(FrameReceiver * receiver);

		void copy(AVFrame * src_frame, uint8_t * dst_data, int dst_size, int align = 1);
		void copyPlane(AVFrame * src_frame, int plane, uint8_t * dst_data, int dst_linesize, int height);

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

		const Metrics & getMetrics() const;

    protected:

		bool allocate(AVCodec * codec, AVStream * stream);
		bool open(AVCodec * codec);

		bool send(AVPacket * packet);
		bool receive(AVFrame * frame);
		AVFrame * receive();
		void free(AVFrame * frame);

        int error;

		std::thread * thread_obj = NULL;
		std::mutex mutex;
		bool running = false;
		PacketSupplier * supplier = NULL;
		FrameReceiver * receiver = NULL;

        AVStream * stream = NULL;
		AVCodec * codec = NULL;
        AVCodecContext * codec_context = NULL;
		int stream_index = -1;

		Metrics metrics;
	};

	/////////////////////////////////////////////////////
    
    class VideoDecoder : public Decoder {
    public:
		//bool open(Reader & reader);
		bool open(Reader & reader, HardwareDecoder * hw_decoder = NULL);
		bool decode(AVPacket * packet, FrameReceiver * receiver);
        int getWidth() const;
        int getHeight() const;
        int getPixelFormat() const;
		int getNumPlanes() const;
		bool getIsHardwareFrame(AVFrame * frame);
		double getFrameRate();
	protected:
		const AVCodecHWConfig * hw_config = NULL;
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

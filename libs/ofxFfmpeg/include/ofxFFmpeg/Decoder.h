#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Flow.h"
#include "Reader.h"
#include "Queue.h"
#include "Hardware.h"
#include "Metrics.h"

namespace ofxFFmpeg {
    
	class Decoder {
	public:

		/////////////////////////////////////////////////
		// OPEN AND CLOSE

		bool open(AVStream * stream);
        void close();
		bool isOpen() const;
        
        /////////////////////////////////////////////////
		// DECODING

        bool match(AVPacket * packet);
        
        bool decode(AVPacket * packet, FrameReceiver * receiver);
		bool flush(FrameReceiver * receiver);
		void flush();

        /////////////////////////////////////////////////
		// THREADING
        
        bool start(PacketSupplier * supplier, FrameReceiver * receiver);
        void stop();
		void decodeThread();
        bool isRunning() const {
            return running && thread_obj;
        }
        
        /////////////////////////////////////////////////
		// ACCESSORS
        
		std::string getName();
		std::string getLongName();
		int getStreamIndex() const;
        int getTotalNumFrames() const;
        int getBitsPerSample() const;
		int64_t getBitRate() const;

        double getTimeBase() const;
		int64_t rescaleTime(int64_t ts) const;
		int64_t rescaleTimeInv(int64_t ts) const;
		int64_t rescaleTime(AVPacket * packet) const;
		int64_t rescaleTime(AVFrame * frame) const;
		int64_t rescaleDuration(AVFrame * frame) const;
		int64_t rescaleFrameNum(int frame_num) const;
		int64_t getTimeStamp(AVFrame * frame) const;
		int getFrameNum(int64_t pts) const;
		int getFrameNum(AVFrame * frame) const;

		uint8_t * getFrameData(AVFrame * frame, int plane = 0);

		bool hasHardwareDecoder();

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
		bool open(Reader & reader);
		bool open(Reader & reader, HardwareDevice & hardware);
        int getWidth() const;
		int getWidth(AVFrame * frame) const;
        int getHeight() const;
		int getHeight(AVFrame * frame) const;
        int getPixelFormat() const;
		int getPixelFormat(AVFrame * frame) const;
		int getNumPlanes() const;
		int getLineSize(AVFrame * frame, int plane) const;
		double getFrameRate();

		bool isKeyFrame(AVPacket * packet);
		bool isHardwareFrame(AVFrame * frame);

	protected:
		const AVCodecHWConfig * hw_config = NULL;
    };

	/////////////////////////////////////////////////////
	class PixelFormat {
	public:
		PixelFormat(int pix_fmt) : format(pix_fmt) {}
		std::string getName();
		int getBitsPerPixel();
		int getNumPlanes();
		bool hasAlpha();
	private:
		int format = -1;
		const AVPixFmtDescriptor * desc = NULL;
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

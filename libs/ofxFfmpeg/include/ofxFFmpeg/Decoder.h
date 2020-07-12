#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Codec.h"
#include "Flow.h"
#include "Reader.h"
#include "Queue.h"
#include "Hardware.h"
#include "Metrics.h"

namespace ofxFFmpeg {
    
	class Decoder : public virtual Codec {
	public:

		/////////////////////////////////////////////////
		// OPEN AND CLOSE

		bool open(AVCodec * codec, AVStream * stream);
        void close();
        
        /////////////////////////////////////////////////
		// DECODING

		// Decode packet
        bool decode(AVPacket * packet, FrameReceiver * receiver);
		// Flush decoder
		bool flush(FrameReceiver * receiver);
		// Flush decoder and stop thread (if funning)
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
        
        double getTimeBase() const;
		int64_t rescaleTime(int64_t ts) const;
		int64_t rescaleTimeInv(int64_t ts) const;
		int64_t rescaleTime(AVPacket * packet) const;
		int64_t rescaleTime(AVFrame * frame) const;
		int64_t rescaleDuration(AVFrame * frame) const;
		int64_t rescaleFrameNum(int frame_num) const;
		int getFrameNum(int64_t pts) const;
		int getFrameNum(AVFrame * frame) const;

		bool hasHardwareDecoder();

		const Metrics & getMetrics() const;

    protected:

		bool send(AVPacket * packet);
		bool receive(AVFrame * frame);
		AVFrame * receive();

        int error;

		std::thread * thread_obj = NULL;
		std::mutex mutex;
		bool running = false;
		//bool flushing = false;
		PacketSupplier * supplier = NULL;
		FrameReceiver * receiver = NULL;

		Metrics metrics;
	};

	/////////////////////////////////////////////////////
    
    class VideoDecoder : public Decoder, public VideoCodec {
    public:
		bool open(Reader & reader);
		bool open(Reader & reader, HardwareDevice & hardware);

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
	
	class AudioDecoder : public Decoder, public AudioCodec {
    public:
		bool open(Reader & reader);
    };
}

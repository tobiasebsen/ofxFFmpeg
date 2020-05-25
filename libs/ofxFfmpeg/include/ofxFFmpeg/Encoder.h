#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Flow.h"

namespace ofxFFmpeg {

	class Encoder {
	public:
		~Encoder() {
			close();
		}

		/////////////////////////////////////////////////
		// OPEN AND CLOSE

		bool open(AVCodec * codec);
		bool open(int codecId);
		bool open(std::string codecName);
		void close();

		/////////////////////////////////////////////////
		// ENCODING
		void freeFrame(AVFrame * frame);

		bool begin(AVStream * stream);
		void end();

		bool encode(AVFrame * frame, PacketReceiver * receiver);
		void flush(PacketReceiver * receiver);

	protected:
		int error;

		AVCodecContext * codec_context = NULL;
		AVStream * stream = NULL;
	};

	class VideoEncoder : public Encoder {
	public:

		AVFrame * allocateFrame();
		void setTimeStamp(AVFrame * frame, int64_t pts);
		void setTimeStamp(AVPacket * packet, int64_t pts);
		int64_t getTimeStamp(AVFrame * frame);
		int64_t getTimeStamp(AVPacket * packet);

		int64_t rescaleFrameNum(int64_t frame_num);

		void setWidth(int width);
		void setHeight(int height);
		void setFrameRate(float frameRate);
		void setBitRate(int bitRate);
		void setMaxBitRate(int bitRate);
		void setBufferSize(int bufferSize);
		void setProfile(int profile);
		void setLevel(int level);
		void setKeyFrame(int key_frame);

		int getWidth();
		int getHeight();
		int getPixelFormat();
	};

	class AudioEncoder : public Encoder {
	public:

	};
}
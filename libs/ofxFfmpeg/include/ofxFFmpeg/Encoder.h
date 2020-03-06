#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Flow.h"

namespace ofxFFmpeg {

	class Encoder {
	public:
		bool setup(AVCodec * codec);
		bool setup(int codecId);
		bool setup(std::string codecName);
		void close();

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
		void setTimeStamp(AVFrame * frame, int frame_num);
		void setTimeStamp(AVFrame * frame, double time_sec);
		void setTimeStamp(AVPacket * packet);

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
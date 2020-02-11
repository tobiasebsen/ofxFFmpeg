#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"

namespace ofxFFmpeg {

	class Encoder {
	public:
		bool setup(AVCodec * codec);
		bool setup(int codecId);
		bool setup(std::string codecName);
		void close();

		void setWidth(int width);
		void setHeight(int height);
		void setFrameRate(float frameRate);
		void setBitRate(int bitRate);
		void setProfile(int profile);
		void setLevel(int level);

		void begin();
		void end();

		bool encode(AVFrame * frame, AVPacket * packet);

	protected:
		int error;

		AVCodecContext * codec_context = NULL;
		AVStream * stream = NULL;
	};
}
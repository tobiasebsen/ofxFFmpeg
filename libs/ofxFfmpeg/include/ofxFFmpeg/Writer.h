#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"

namespace ofxFFmpeg {

	class Writer {
	public:

		bool open(const std::string filename);
		void close();

		AVStream * addStream(const AVCodec * codec = nullptr);

		bool begin();
		void end();

		void write(AVPacket * packet);

		/////////////////////////////////////////////////
		// ACCESSORS

		std::string getName();
		std::string getLongName();

	protected:
		int error = 0;

		AVIOContext *io_context = nullptr;
		AVFormatContext *format_context = nullptr;
	};
}
#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"

namespace ofxFFmpeg {

	class Writer {
	public:

		bool open(const std::string filename);
		void close();

		AVStream * addStream();

		bool begin();
		void end();

		void write(AVPacket * packet);

	protected:
		int error = 0;

		AVIOContext *io_context = NULL;
		AVFormatContext *format_context = NULL;
	};
}
#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Flow.h"
#include "Metrics.h"


namespace ofxFFmpeg {

	class Writer {
	public:

		bool open(const std::string filename);
		void close();

		AVStream * addStream(const AVCodec * codec = nullptr);

		bool begin();
		void end();

		bool write(AVPacket * packet);

		/////////////////////////////////////////////////
		// THREADING

		bool start(PacketSupplier * supplier);
		void stop();
		void writeThread();
		bool isRunning() const {
			return running && thread_obj;
		}

		/////////////////////////////////////////////////
		// ACCESSORS

		std::string getName();
		std::string getLongName();

		const Metrics getMetrics() const;

	protected:
		int error = 0;

		AVIOContext *io_context = nullptr;
		AVFormatContext *format_context = nullptr;

		std::thread * thread_obj = NULL;
		std::mutex mutex;
		bool running = false;
		PacketSupplier * supplier;

		Metrics metrics;
	};
}
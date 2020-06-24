#pragma once

#include <thread>
#include <mutex>

#include "AvTypes.h"
#include "Codec.h"
#include "Hardware.h"
#include "Flow.h"

namespace ofxFFmpeg {

	class Encoder : public virtual Codec {
	public:
		~Encoder() {
			Codec::free();
		}

		/////////////////////////////////////////////////
		// ALLOCATE, OPEN AND CLOSE

		bool allocate(int codecId);
		bool allocate(std::string codecName);
		using Codec::free;

		/////////////////////////////////////////////////
		// ENCODING

		bool open(AVStream * stream);

		bool encode(AVFrame * frame, PacketReceiver * receiver);
		void flush(PacketReceiver * receiver);
	};

	/////////////////////////////////////////////////

	class VideoEncoder : public Encoder, public VideoCodec {
	public:

		using Encoder::open;
		bool open(AVStream * stream, HardwareDevice & hardware);

		int64_t rescaleFrameNum(int64_t frame_num);
	};

	/////////////////////////////////////////////////

	class AudioEncoder : public Encoder, public AudioCodec {
	public:

		int64_t rescaleSampleCount(int64_t nb_samples);
	};
}
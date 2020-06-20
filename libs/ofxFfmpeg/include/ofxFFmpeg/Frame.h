#pragma once

#include "AvTypes.h"
#include <cinttypes>

namespace ofxFFmpeg {

	class Frame {
	public:
		Frame(AVFrame * f) : frame(f) {}

		operator AVFrame * () { return frame;  }

		static Frame allocate();
		static void free(Frame & f);
		static void free(AVFrame * f);

		uint8_t * getData(int plane = 0);
		int64_t getTimeStamp() const;
		void setTimeStamp(int64_t pts);

	protected:
		AVFrame * frame;
	};

	/////////////////////////////////////////////////

	class VideoFrame : public Frame {
	public:
		VideoFrame(AVFrame * frame) : Frame(frame) {}

		static VideoFrame allocate(int width, int height, int pix_fmt);

		int getWidth() const;
		int getHeight() const;
		int getPixelFormat() const;
		int getNumPlanes() const;
		int getLineSize(int plane) const;
	};

	/////////////////////////////////////////////////

	class AudioFrame : public Frame {
	public:
		AudioFrame(AVFrame * frame) : Frame(frame) {}

		static AudioFrame allocate(int nb_samples, int nb_channels, int sample_fmt);

		int getNumSamples() const;
		void setNumSamples(int nb_samples);
	};

	/////////////////////////////////////////////////

	class Packet {
	public:
		Packet(AVPacket * p) : packet(p) {}

		int64_t getTimeStamp() const;
		void	setTimeStamp(int64_t pts);

	protected:
		AVPacket * packet;
	};
}
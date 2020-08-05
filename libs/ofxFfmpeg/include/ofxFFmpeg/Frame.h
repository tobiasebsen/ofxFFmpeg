#pragma once

#include "AvTypes.h"
#include <cinttypes>

namespace ofxFFmpeg {

	class Frame {
	public:
		Frame();
		Frame(AVFrame * f);
		virtual ~Frame();

		operator AVFrame * () { return frame;  }

		static Frame allocate();
		static void free(Frame & f);
		static void free(AVFrame * f);

		uint8_t * getData(int plane = 0);
		int       getSize(int plane = 0) const;

		int64_t getTimeStamp() const;
		void setTimeStamp(int64_t pts);

		int64_t getDecodeTime() const;
		void setDecodeTime(int64_t dts);

		int64_t getDuration() const;
		void setDuration(int64_t duration);

	protected:
		AVFrame * frame;
	};

	/////////////////////////////////////////////////

	class VideoFrame : public Frame {
	public:
		using Frame::Frame;

		static VideoFrame allocate(int width, int height, int pix_fmt);
		
		void allocate(size_t size, int plane = 0);

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

		uint8_t * getData() const;
		int	      getSize() const;

		int64_t getTimeStamp() const;
		void	setTimeStamp(int64_t pts);

		int64_t	getDuration() const;
		void	setDuration(int64_t duration);

	protected:
		AVPacket * packet;
	};
}
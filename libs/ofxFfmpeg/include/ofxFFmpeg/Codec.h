#pragma once

#include "AvTypes.h"
#include <string>

namespace ofxFFmpeg {

	class Codec {
	public:

		bool allocate(AVCodec * codec);
		bool isAllocated() const;
		void free();

		bool open();
		bool open(const AVCodec * codec);
		bool open(AVStream * stream);
		bool isOpen() const;
		bool match(AVPacket * packet);


		std::string getName() const;
		std::string getLongName() const;
		int getStreamIndex() const;
		int getTotalNumFrames() const;
		int getBitsPerSample() const;

		int64_t	getBitRate() const;
		void	setBitRate(int64_t bitRate);
		void	setMaxBitRate(int bitRate);
		void	setBufferSize(int bufferSize);

		AVCodecContext * getContext() const;

	protected:
		int error = 0;
		AVCodecContext * context = nullptr;
		AVStream * stream = nullptr;

		friend class VideoCodec;
		friend class AudioCodec;
	};

	/////////////////////////////////////////////////////

	class VideoCodec : public virtual Codec {
	public:

		int		getWidth() const;
		void	setWidth(int width);
		int		getHeight() const;
		void	setHeight(int height);
		int		getPixelFormat() const;
		void	setPixelFormat(int pix_fmt);

		double	getFrameRate() const;
		void	setFrameRate(double frameRate);
		void	setProfile(int profile);
		void	setLevel(int level);
		void	setKeyFrame(int key_frame);
	};

	/////////////////////////////////////////////////////

	class AudioCodec : public virtual Codec {
	public:

		int		getSampleRate() const;
		void	setSampleRate(int sample_rate);
		int		getNumChannels() const;
		void	setNumChannels(int nb_channels);
		int		getSampleFormat() const;
		int		getChannelLayout() const;
		int		getFrameSize() const;
	};
}
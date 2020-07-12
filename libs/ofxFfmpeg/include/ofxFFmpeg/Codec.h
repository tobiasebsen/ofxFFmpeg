#pragma once

#include "AvTypes.h"
#include <string>
#include <vector>

namespace ofxFFmpeg {

	class Codec {
	public:

		bool allocate(AVCodec * codec);
		bool isAllocated() const;
		void free();

		bool open(AVStream * stream);
		bool isOpen() const;
		void close();
		bool match(AVPacket * packet);

		int getId() const;
		unsigned int getTag() const;
		std::string getTagString();

		std::string getName() const;
		std::string getLongName() const;
		int getStreamIndex() const;
		int getTotalNumFrames() const;
		int getBitsPerSample() const;

		int64_t	getBitRate() const;
		void	setBitRate(int64_t bitRate);
		void	setMaxBitRate(int bitRate);
		void	setBufferSize(int bufferSize);

		double	getTimeBase() const;
		double	getDurationSeconds() const;
		int64_t	getDuration() const;

		int getError() const;
		std::string getErrorString() const;
		AVCodecContext * getContext() const;
		const AVCodec * getCodec() const;

	protected:
		int error = 0;
		AVCodec * codec;
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
		int		getNumSamples() const;
	};

	/////////////////////////////////////////////////////

	class Codecs : public std::vector<const AVCodec*> {
	public:

		static Codecs getCodecs();
		static Codecs getID(const Codecs & codecs, int codecID);
		static Codecs getHardware(const Codecs & codecs);
		static Codecs getDecode(const Codecs & codecs);
		static Codecs getEncode(const Codecs & codecs);
		static Codecs getVideo(const Codecs & codecs);
		static Codecs getAudio(const Codecs & codecs);

		std::vector<std::string> getNames();
	};
}
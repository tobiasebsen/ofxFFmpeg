#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "AvTypes.h"
#include "Decoder.h"
#include "Metrics.h"

namespace ofxFFmpeg {

	class HardwareDevice {
	public:
		HardwareDevice();
		~HardwareDevice() {
			close();
		}

		bool open(int device_type = -1);
		void close();
		bool isOpen();

		int getType() const;
		std::string getName();
		int getPixelFormat(const AVCodec * codec);
		const AVCodecHWConfig * getConfig(const AVCodec * codec) const;
		AVBufferRef * getContext() const;
		std::vector<int> getFormats();

		static bool transfer(AVFrame * hw_frame, AVFrame * sw_frame);
		static AVFrame * transfer(AVFrame * hw_frame);
		static void free(AVFrame * frame);

		static int getNumHardwareConfig(const AVCodec * codec);

		static std::vector<int> getTypes();
		static int getType(const AVCodec * codec, int config_index);
		static int getDefaultType();
		static std::string getName(int device_type);

	protected:
		int error = 0;
		int device_type;
		AVBufferRef * hwdevice_context = NULL;
	};

	/////////////////////////////////////////////////////

	class HardwareDecoder : public VideoDecoder {
	public:
		bool open(Reader & reader, HardwareDevice & device);

		bool isHardwareFrame(AVFrame * frame);
		bool hasHardwareDecoder();

	protected:
		const AVCodecHWConfig * hw_config = NULL;
		AVBufferRef * hwframes_context = NULL;
	};
}

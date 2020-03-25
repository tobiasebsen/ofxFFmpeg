#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "AvTypes.h"

namespace ofxFFmpeg {

	class HardwareDecoder {
	public:
		HardwareDecoder();
		~HardwareDecoder() {
			close();
		}

		bool open(int device_type = -1);
		void close();
		bool isOpen();

		std::string getDeviceName();
		int getPixelFormat(const AVCodec * codec);
		const AVCodecHWConfig * getConfig(const AVCodec * codec) const;
		AVBufferRef * getContext() const;
		std::vector<int> getFormats();

		static bool transfer(AVFrame * hw_frame, AVFrame * sw_frame);
		static AVFrame * transfer(AVFrame * hw_frame);
		static void free(AVFrame * frame);

        static std::vector<int> getDeviceTypes();
		static int getNumHardwareConfig(const AVCodec * codec);
		static int getDeviceType(const AVCodec * codec, int config_index);
		static int getDefaultDeviceType();
		static std::string getDeviceName(int device_type);

	protected:
		int error = 0;
		int device_type;
		AVBufferRef * hw_context = NULL;
		//AVCodecHWConfig * hw_config;
	};
}

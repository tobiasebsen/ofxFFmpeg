#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "AvTypes.h"
#include "Metrics.h"

namespace ofxFFmpeg {

	class HardwareDevice {
	public:
		~HardwareDevice() {	close(); }

		bool open(int device_type = -1);
		bool open(std::string device_name);
		void close();
		bool isOpen();

		int getType() const;
		std::string getName();

		int getPixelFormat(const AVCodec * codec);
		const AVCodecHWConfig * getConfig(const AVCodec * codec) const;
		AVBufferRef * getContextRef() const;
		std::vector<int> getFormats();

		/////////////////////////////////////////////////

		static bool transfer(AVFrame * hw_frame, AVFrame * sw_frame);
		static AVFrame * transfer(AVFrame * hw_frame);
		static void free(AVFrame * frame);

		static int getNumHardwareConfig(const AVCodec * codec);

		static std::vector<int> getTypes();
		static std::vector<std::string> getTypeNames();
		static int getType(const AVCodec * codec, int config_index);
		static int getType(std::string name);
		static int getDefaultType();
		static std::string getName(int device_type);

	protected:
		int error = 0;
		AVBufferRef * hwdevice_context_ref = NULL;
		AVHWDeviceContext * hwdevice_context = NULL;
	};
}

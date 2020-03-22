#pragma once

#include "Decoder.h"

namespace ofxFFmpeg {

	class HardwareDecoder : public VideoDecoder {
	public:
		bool open(Reader & reader);
		void close();

		bool decode(AVPacket *packet, FrameReceiver * receiver);

		int getPixelFormat() const;

        static std::vector<int> getDeviceTypes();
		static int getNumHardwareConfig(const AVCodec * codec);
		static int getDeviceType(const AVCodec * codec, int config_index);

	protected:
		AVBufferRef * hardware_context = NULL;
		int hw_format = -1;
		int sw_format = -1;
	};
}

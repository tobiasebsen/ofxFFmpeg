#pragma once

#include "AvTypes.h"

namespace ofxFFmpeg {

	class PacketSupplier {
	public:
		virtual AVPacket * supplyPacket() = 0;
		virtual void freePacket(AVPacket * packet) = 0;
	};

	class PacketReceiver {
	public:
		virtual void receivePacket(AVPacket * packet) = 0;
		virtual void endPacket() {}
		virtual void notifyPacket() {}
	};

	class FrameReceiver {
	public:
		virtual void receiveFrame(AVFrame * frame, int stream_index) = 0;
		virtual void endFrame() {}
	};

	class ImageReceiver {
	public:
		virtual void receiveImage(uint64_t pts, uint64_t duration, const std::shared_ptr<uint8_t> imageData) = 0;
		virtual void endImage() {}
	};
}
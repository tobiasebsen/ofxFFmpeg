#pragma once

#include "AvTypes.h"

namespace ofxFFmpeg {

	class PacketSupplier {
	public:
		virtual AVPacket * supply() = 0;
		virtual void free(AVPacket * packet) = 0;
		virtual void terminatePacketSupplier() {}
		virtual void resumePacketSupplier() {}
	};

	class PacketReceiver {
	public:
		virtual void receive(AVPacket * packet) = 0;
		virtual void notifyEndPacket() {}
		virtual void terminatePacketReceiver() {}
		virtual void resumePacketReceiver() {}

	};

	class FrameSupplier {
	public:
		virtual AVFrame * supply() = 0;
		virtual void terminateFrameSupplier() {}
		virtual void resumeFrameSupplier() {}
	};

	class FrameReceiver {
	public:
		virtual void receive(AVFrame * frame, int stream_index) = 0;
		virtual void notifyEndFrame() {}
		virtual void terminateFrameReceiver() {}
		virtual void resumeFrameReceiver() {}
	};

	class ImageReceiver {
	public:
		virtual void receive(uint64_t pts, uint64_t duration, const std::shared_ptr<uint8_t> imageData) = 0;
		virtual void notifyEnd() {}
	};
}
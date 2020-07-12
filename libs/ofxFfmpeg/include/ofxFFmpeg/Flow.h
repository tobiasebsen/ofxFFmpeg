#pragma once

#include "AvTypes.h"

namespace ofxFFmpeg {

	class PacketSupplier {
	public:
		virtual AVPacket * supply() = 0;
		virtual void free(AVPacket * packet) = 0;
		virtual void terminatePacketSupplier() {}
		virtual void resumePacketSupplier() {}
		virtual bool isPacketsTerminated() { return false; }
		//virtual void flush(class PacketReceiver * receiver) = 0;
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
		virtual void free(AVFrame * frame) = 0;
		virtual void terminateFrameSupplier() {}
		virtual void resumeFrameSupplier() {}
	};

	class FrameReceiver {
	public:
		virtual void receive(AVFrame * frame, int stream_index) = 0;
		virtual void notifyEndFrame(int stream_index) {}
		virtual void terminateFrameReceiver() {}
		virtual void resumeFrameReceiver() {}
	};

}
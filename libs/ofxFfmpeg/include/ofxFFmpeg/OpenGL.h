#pragma once

#include "Hardware.h"

namespace ofxFFmpeg {

	class OpenGLDevice {
	public:
		~OpenGLDevice() { close(); }

		bool open(HardwareDevice & hardware);
		bool isOpen() const;
		void close();

		int getHardwareType();
		AVBufferRef * getContextRef();

	protected:
		AVBufferRef * hwdevice_context_ref = NULL;
		AVHWDeviceContext * hwdevice_context = NULL;
	};


	class OpenGLRenderer {
	public:
		~OpenGLRenderer() { close(); }

		bool open(OpenGLDevice & device, int width, int height, int target);
		bool isOpen() const;
		void close();

        size_t getNumPlanes() { return planes; }
		unsigned int getTexture(int plane = 0) { return textures[plane]; }
		int getTarget() { return target; }
		int getWidth(int plane = 0) { return width[plane]; }
		int getHeight(int plane = 0) { return height[plane]; }

		void render(AVFrame * frame);
		void lock();
		void unlock();

	protected:
		AVBufferRef * hwdevice_context_ref = NULL;
		AVHWDeviceContext * hwdevice_context = NULL;
        size_t planes = 0;
		unsigned int textures[4];
        unsigned int formats[4];
		int target;
		int width[4];
		int height[4];
		void * opaque = NULL;
	};
}

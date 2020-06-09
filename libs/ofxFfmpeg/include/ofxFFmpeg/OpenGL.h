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

		bool open(OpenGLDevice & device, int width, int height);
		bool isOpen() const;
		void close();

		unsigned int getTexture() { return texture; }
		int getTarget() { return target; }
		int getWidth() { return width; }
		int getHeight() { return height; }

		void render(AVFrame * frame);
		void lock();
		void unlock();

	protected:
		AVBufferRef * hwdevice_context_ref = NULL;
		AVHWDeviceContext * hwdevice_context = NULL;
		unsigned int texture = 0;
		int target;
		int width;
		int height;
		void * opaque = NULL;
	};
}
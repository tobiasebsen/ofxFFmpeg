#pragma once

#include "Hardware.h"

namespace ofxFFmpeg {

	class OpenGLDevice : public HardwareDevice {
	public:
		~OpenGLDevice() { close(); }

		bool open();
		void close();

	protected:
	};


	class OpenGLRenderer {
	public:
		~OpenGLRenderer() { close(); }

		bool open(OpenGLDevice & hardware, int width, int height);
		void close();

		unsigned int getTexture() { return texture; }
		int getTarget() { return target; }
		int getWidth() { return width; }
		int getHeight() { return height; }

		void render(AVFrame * frame);
		void lock();
		void unlock();

	protected:
		OpenGLDevice * hardware = NULL;
		unsigned int texture = 0;
		int target;
		int width;
		int height;
		void * data = NULL;
	};
}
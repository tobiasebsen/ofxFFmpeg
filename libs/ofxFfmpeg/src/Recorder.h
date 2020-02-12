#pragma once

#include "ofMain.h"

#include "AvTypes.h"
#include "Flow.h"
#include "Writer.h"
#include "Encoder.h"
#include "VideoScaler.h"

namespace ofxFFmpeg {

	class Recorder : public PacketReceiver {
	public:
		bool open(const string filename);
		void close();

		bool setVideoCodec(int codecId);
		bool setVideoCodec(string codecName);

		VideoEncoder & getVideoEncoder();

        bool start();
        void stop();

        void write(const ofPixels & pixels);

		int getError();
		string getErrorString();

	protected:

		void receivePacket(AVPacket * packet);

		int error = 0;

		Writer writer;
		VideoEncoder video;
		VideoScaler scaler;

		AVFrame *frame = NULL;
        uint64_t n_frame;
	};
}

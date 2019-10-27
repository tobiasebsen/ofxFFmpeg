#pragma once

#include "ofMain.h"

struct AVIOContext;
struct AVFormatContext;
struct AVCodec;
struct AVStream;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace ofxFFmpeg {

	class Recorder {
	public:
		bool open(const string filename, int width, int height, int frameRate);
		void close();

		void write(const ofPixels & pixels);
		void write(AVFrame * frame);
		void flush();

	protected:
		bool init();
		bool openCodec(int width, int height, int frameRate);

		AVIOContext *io_context = NULL;
		AVFormatContext *format_context = NULL;
		AVCodec *video_codec = NULL;
		AVStream *video_stream = NULL;
		AVCodecContext *video_context = NULL;
		AVFrame *frame = NULL;
        uint64_t pts;

		SwsContext * sws_context;
	};
}

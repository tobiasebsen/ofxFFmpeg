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

		AVIOContext *output_io_context = NULL;
		AVFormatContext *output_format_context = NULL;
		AVCodec *output_codec = NULL;
		AVStream *stream = NULL;
		AVCodecContext *avctx = NULL;
		AVFrame *frame = NULL;
        uint64_t pts;

		SwsContext * sws_ctx;
	};
}

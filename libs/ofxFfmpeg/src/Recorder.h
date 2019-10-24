#pragma once

#include "ofMain.h"

struct AVIOContext;
struct AVFormatContext;
struct AVCodec;
struct AVStream;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

namespace ofxFFmpeg {

	class Recorder {
	public:
		bool open(const string filename, int width, int height, int frameRate);
		void close();

		void write(const ofPixels & pixels);

	protected:
		AVIOContext *output_io_context = NULL;
		AVFormatContext *output_format_context = NULL;
		AVCodec *output_codec = NULL;
		AVStream *stream = NULL;
		AVCodecContext *avctx = NULL;
		AVFrame *frame = NULL;
	};
}
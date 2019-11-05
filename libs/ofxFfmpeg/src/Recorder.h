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
        bool init();
		bool open(const string filename);
		void close();

        bool setCodec(string codecName);
        bool setCodec(int codecId);
        void setWidth(int width);
        void setHeight(int height);
        void setFrameRate(float frameRate);
        void setBitRate(int bitRate);
		void setProfile(int profile);
		void setLevel(int level);
        void setKeyFrame(int keyFrameRate);
        
        bool start();
        void stop();

        void write(const ofPixels & pixels);
		void write(AVFrame * frame);
		void flush();

	protected:

		AVIOContext *io_context = NULL;
		AVFormatContext *format_context = NULL;
		AVCodec *video_codec = NULL;
		AVStream *video_stream = NULL;
		AVCodecContext *video_context = NULL;
		AVFrame *frame = NULL;
        uint64_t n_frame;
        uint64_t pts;
        int keyFrameRate = 15;

		SwsContext * sws_context;
	};
}

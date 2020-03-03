#pragma once

#include <thread>
#include <mutex>

#include "Decoder.h"
#include "Encoder.h"

struct AVCodecContext;
struct AVFrame;
struct SwsContext;

namespace ofxFFmpeg {
    
    class VideoScaler {
    public:

		bool setup(int src_width, int src_height, int src_fmt, int dst_width, int dst_height, int dst_fmt);
		bool setup(int width, int height, int src_fmt, int dst_fmt);
        bool setup(VideoDecoder & decoder);
		bool setup(VideoEncoder & encoder);
		bool isSetup();
		void clear();

		bool scale(AVFrame * frame, const uint8_t * imageData, int line_stride);
		bool scale(const uint8_t * imageData, int line_stride, int height, AVFrame * frame);

        void start();
        void stop();
        void scaleThread();

    private:
        SwsContext * sws_context = NULL;
    };
}

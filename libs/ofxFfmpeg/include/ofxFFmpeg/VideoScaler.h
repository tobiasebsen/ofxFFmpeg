#pragma once

#include <thread>
#include <mutex>

#include "Decoder.h"
#include "Encoder.h"
#include "Metrics.h"

struct AVCodecContext;
struct AVFrame;
struct SwsContext;

namespace ofxFFmpeg {
    
    class VideoScaler {
    public:

		bool allocate(int src_width, int src_height, int src_fmt, int dst_width, int dst_height, int dst_fmt);
		bool allocate(int width, int height, int src_fmt, int dst_fmt);
        bool allocate(VideoDecoder & decoder, int dst_fmt);
		bool allocate(VideoEncoder & encoder);
		bool isAllocated();
		void free();

		bool scale(AVFrame * src_frame, AVFrame * dst_frame);
		AVFrame * scale(AVFrame * frame);

		//bool scale(AVFrame * frame, const uint8_t * imageData, int line_stride);
		bool scale(const uint8_t * imageData, int line_stride, int height, AVFrame * frame);

		void copy(AVFrame * src_frame, uint8_t * dst_data, int dst_size, int align = 1);
		void copyPlane(AVFrame * src_frame, int plane, uint8_t * dst_data, int dst_linesize, int height);

		uint8_t * getData(AVFrame * frame);

        bool start(FrameSupplier * supplier, FrameReceiver * receiver, int stream_index);
        void stop();
        void scaleThread();

		static bool supportsInput(int src_format);
		static bool supportsOutput(int dst_format);

		Metrics & getMetrics();
		const Metrics & getMetrics() const;

    private:
		int error;
        SwsContext * sws_context = NULL;
		int dst_width;
		int dst_height;
		int dst_fmt;

		std::thread * thread_obj = NULL;
		bool running = false;
		FrameSupplier * supplier = NULL;
		FrameReceiver * receiver = NULL;
		int stream_index = -1;

		Metrics metrics;
    };
}

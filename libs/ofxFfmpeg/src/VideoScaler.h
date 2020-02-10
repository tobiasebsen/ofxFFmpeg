#pragma once

#include <thread>
#include <mutex>

#include "Decoder.h"

struct AVCodecContext;
struct AVFrame;
struct SwsContext;

namespace ofxFFmpeg {
    
    class ImageReceiver {
    public:
        virtual void receiveImage(uint64_t pts, uint64_t duration, const std::shared_ptr<uint8_t> imageData) = 0;
    };

    class VideoScaler {
    public:

        bool setup(int width, int height, int pix_fmt);
        bool setup(VideoDecoder & decoder);
		void clear();

		bool scale(AVFrame * frame, ImageReceiver * receiver);
		bool scale(AVFrame * frame, const uint8_t * imageData);
        
        void start();
        void stop();
        void scaleThread();

    private:
        SwsContext * sws_context = NULL;
        const uint8_t * pixelData;
    };
}

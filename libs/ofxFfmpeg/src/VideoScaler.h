#pragma once

#include <thread>
#include <mutex>

#include "Decoder.h"

struct AVCodecContext;
struct AVFrame;
struct SwsContext;

namespace ofxFFmpeg {
    
    class PixelReceiver {
    public:
        virtual void receivePixels(const uint8_t * pixelData) = 0;
    };

    class VideoScaler {
    public:

        bool setup(int width, int height, int pix_fmt);
        bool setup(VideoDecoder & decoder);
		void clear();

        bool scale(AVFrame * frame, const uint8_t * pixelData);
        uint8_t * scale(AVFrame * frame);
        void free(uint8_t * pixelData);
        
        void start();
        void stop();
        void scaleThread();

    private:
        SwsContext * sws_context = NULL;
        const uint8_t * pixelData;
    };
}

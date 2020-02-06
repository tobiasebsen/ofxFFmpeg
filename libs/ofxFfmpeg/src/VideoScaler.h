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
        void receivePixels();
    };

    class VideoScaler {
    public:

        void setup(int width, int height, int pix_fmt);
        void setup(Decoder & decoder);

        bool scale(AVFrame * frame, const uint8_t * pixelData);
        uint8_t * scale(AVFrame * frame);
        void free(uint8_t * pixelData);
        
        void start();
        void stop();
        void scaleThread();

    private:
        SwsContext * sws_context;
        const uint8_t * pixelData;
    };
}

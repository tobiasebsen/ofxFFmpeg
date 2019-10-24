#pragma once

#include <stdlib.h>
#include <memory>

#include "AvFrame.h"
#include "AvPacket.h"
#include <vector>

struct AVCodecContext;
struct AVCodec;

namespace ofxFFmpeg {
    
    class AvCodec {
    public:
        AvCodec(AVCodecContext *context, AVCodec * codec);
        ~AvCodec();
        
        void setWidth(size_t width);
        void setHeight(size_t height);
        void setFrameRate(int frameRate);
        void setPixelFormat(int pixelFormat);
        vector<int> getPixelFormats();
        
        bool open();

        AvFramePtr allocFrame();
        bool encode(AvFramePtr frame);
        bool receivePacket(AvPacket & pkt);

    private:
        AvCodec() {}
        AVCodecContext * context;
        AVCodec * codec;
    };

    typedef std::shared_ptr<AvCodec> AvCodecPtr;
}

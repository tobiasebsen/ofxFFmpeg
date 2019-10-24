#pragma once

struct AVPacket;

namespace ofxFFmpeg {
    
    class AvPacket {
    public:
        AvPacket();
        ~AvPacket();
        
        unsigned char * getData();
        unsigned int getSize();
        
        void unref();
        
        AVPacket * getPacket();

    private:
        AVPacket *pkt;
    };
}

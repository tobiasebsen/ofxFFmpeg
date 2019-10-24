#pragma once

#include <stdlib.h>
#include <memory>

using namespace std;

struct AVFrame;

namespace ofxFFmpeg {

    class AvFrame {
    public:
        AvFrame(AVFrame * f) : frame(f) {}

        void getBuffer(int align);
        void makeWritable();
        
        void setPts(int pts);
        
        AVFrame * getFrame() {
            return frame;
        }

    private:
        AvFrame() {}
        AVFrame * frame;
    };

    typedef shared_ptr<AvFrame> AvFramePtr;
}

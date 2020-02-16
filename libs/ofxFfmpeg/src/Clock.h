#pragma once

#include <mutex>

namespace ofxFFmpeg {
    
    class Clock {
    public:
        
        void tick(uint64_t);

    protected:
        int64_t pts;
    };
}

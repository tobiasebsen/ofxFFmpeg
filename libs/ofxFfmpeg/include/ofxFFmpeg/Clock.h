#pragma once

#include <mutex>

namespace ofxFFmpeg {
    
    class Clock {
    public:
        
        void reset();
        
        void tick(uint64_t duration);
        
        uint64_t getTime();

    protected:
        int64_t pts;
        std::mutex mutex;
    };
}

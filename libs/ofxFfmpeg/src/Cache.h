#pragma once

#include <map>
#include <mutex>

#include "AvTypes.h"

namespace ofxFFmpeg {
    
    template<typename T>

    class Cache {
    public:

        void store(T * t);
        T * fetch(uint64_t pts);

    protected:
        std::map<uint64_t, T*> cache;
        std::mutex mutex;
        std::condition_variable condition;
    };
    
    template<typename T>
    void Cache<T>::store(T * t) {
    }
    
    template<typename T>
    T * Cache<T>::fetch(uint64_t pts) {
    }
    

    class FrameCache : public Cache<AVFrame> {
    public:
    };
}

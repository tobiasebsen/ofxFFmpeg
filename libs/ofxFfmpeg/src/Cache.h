#pragma once

#include <map>
#include <mutex>

#include "AvTypes.h"

namespace ofxFFmpeg {
    
    template<typename T>

    class Cache {
    public:

        void store(uint64_t pts, T * t);
        T * fetch(uint64_t pts);

    protected:
        std::map<uint64_t, T*> cache;
        std::mutex mutex;
        std::condition_variable condition;
    };
    
    template<typename T>
    void Cache<T>::store(uint64_t pts, T * t) {
        std::lock_guard<std::mutex> lock(mutex);
        cache.emplace(std::pair<uint64_t, T*>(pts, t));
    }
    
    template<typename T>
    T * Cache<T>::fetch(uint64_t pts) {
        std::lock_guard<std::mutex> lock(mutex);
    }
    

    class FrameCache : public Cache<AVFrame> {
    public:
    };
}

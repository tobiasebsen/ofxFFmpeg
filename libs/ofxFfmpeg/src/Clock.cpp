#include "Clock.h"

using namespace ofxFFmpeg;

void Clock::reset() {
    pts = 0;
}

void Clock::tick(uint64_t duration) {
    std::lock_guard<std::mutex> lock(mutex);
    pts += duration;
}

uint64_t Clock::getTime() {
    return pts;
}

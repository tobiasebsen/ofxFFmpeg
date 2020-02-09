#include "PacketQueue.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

using namespace ofxFFmpeg;

void PacketQueue::receivePacket(AVPacket *packet) {
    std::lock_guard<std::mutex> locker(lock);
    AVPacket * p = av_packet_clone(packet);
    queue.emplace_back(p);
    condition.notify_one();
}

AVPacket * PacketQueue::supplyPacket() {
    std::unique_lock<std::mutex> locker(lock);
    while (queue.size() == 0) {
        condition.wait(locker);
        if (queue.size() > 0) {
            AVPacket * packet = queue.front();
            queue.pop_front();
            condition.notify_one();
            return packet;
        }
    }
    return NULL;
}

void PacketQueue::wait() {
	std::unique_lock<std::mutex> locker(lock);
	condition.wait(locker);
}

void PacketQueue::notify() {
    std::unique_lock<std::mutex> locker(lock);
    condition.notify_all();
}

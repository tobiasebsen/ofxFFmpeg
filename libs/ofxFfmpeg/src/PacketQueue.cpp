#include "PacketQueue.h"

using namespace ofxFFmpeg;

void PacketQueue::push(AVPacket * packet) {
	std::unique_lock<std::mutex> locker(lock);
	queue.emplace_back(packet);
	condition.notify_all();
}

void PacketQueue::wait() {
	std::unique_lock<std::mutex> locker(lock);
	condition.wait(locker);
}
#pragma once

#include <queue>
#include <mutex>
#include <atomic>

#include "AvTypes.h"
#include "Flow.h"

namespace ofxFFmpeg {
    
    template<typename T>

    class Queue {
    public:
        Queue(size_t size = 2) : max_size(size), terminated(false){}

        bool push(T * t);
        T * pop();

        size_t size() { return queue.size(); }

		void terminate();
		void resume();

		virtual void free(T *) = 0;
		void flush();

    protected:
        size_t max_size;
        std::queue<T*> queue;
        std::mutex mutex;
        std::condition_variable cond_push;
        std::condition_variable cond_pop;
		std::atomic<bool> terminated;
    };
    
    template<typename T>
    bool Queue<T>::push(T *t) {
        while (queue.size() >= max_size && !terminated) {
            std::unique_lock<std::mutex> lock(mutex);
            cond_pop.wait(lock);
        }
        if (!terminated) {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(t);
            cond_push.notify_one();
			return true;
        }
		return false;
    }
    
    template<typename T>
    T * Queue<T>::pop() {
        while (queue.size() == 0 && !terminated) {
            std::unique_lock<std::mutex> lock(mutex);
            cond_push.wait(lock);
        }
        if (!terminated) {
            std::lock_guard<std::mutex> lock(mutex);
			T * front = queue.front();
            queue.pop();
            cond_pop.notify_one();
            return front;
        }
		return nullptr;
    }

	template<typename T>
	void Queue<T>::terminate() {
		terminated = true;
		cond_push.notify_all();
		cond_pop.notify_all();
	}

	template<typename T>
	void Queue<T>::resume() {
		terminated = false;
	}

	template<typename T>
	void Queue<T>::flush() {
		std::lock_guard<std::mutex> lock(mutex);
		while (queue.size() > 0) {
			free(queue.front());
			queue.pop();
		}
	}


    class PacketQueue : public Queue<AVPacket>, public PacketReceiver, public PacketSupplier {
    public:
		void receive(AVPacket * packet) { push(clone(packet)); }
		void terminatePacketReceiver() { terminate(); flush(); }
		void resumePacketReceiver() { resume(); }

		AVPacket * supply() { return pop(); }
		void terminatePacketSupplier() { terminate(); flush(); }
		void resumePacketSupplier() { resume(); }

        AVPacket * clone(AVPacket * p);
        virtual void free(AVPacket * p);
    };


    class FrameQueue : public Queue<AVFrame> {
    public:
        AVFrame * clone(AVFrame * f);
        virtual void free(AVFrame * f);
    };
}

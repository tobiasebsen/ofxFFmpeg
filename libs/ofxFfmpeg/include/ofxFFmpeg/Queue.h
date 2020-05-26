#pragma once

#include <list>
#include <mutex>
#include <atomic>

#include "AvTypes.h"
#include "Flow.h"

namespace ofxFFmpeg {
    
    template<typename T>

    class Queue {
    public:
        Queue(size_t size = 4) : max_size(size), terminated(false){}
		~Queue() { flush(); }

        bool push(T * t);
        T * pop();

        size_t size() const { return queue.size(); }
		size_t capacity() const { return max_size; }
		void resize(size_t s) { max_size = s; }

		void terminate();
		void resume();

		void flush();

		void lock() { mutex.lock(); }
		T * operator[](size_t index) { return queue[index]; }
		void unlock() { mutex.unlock(); }

    protected:
		virtual void free(T *) = 0;

		size_t max_size;
        std::list<T*> queue;
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
            queue.push_back(t);
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
			if (queue.size() > 0) {
				T * front = queue.front();
				queue.pop_front();
				cond_pop.notify_one();
				return front;
			}
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
			queue.pop_front();
		}
		cond_pop.notify_all();
	}

	template<typename T>
	class TimeQueue : public Queue<T> {
	public:

		void receive(T * t, int stream_index) {
            if (Queue<T>::push(clone(t))) {
				head_pts = get_head(t);
			}
		}

		T * supply() {
			T * t = Queue<T>::pop();
			return t;
		}

		int64_t get_head() const { return head_pts; }
		int64_t get_tail() const { return tail_pts; }

	protected:
		virtual T * clone(T * t) = 0;
		virtual int64_t get_head(T*) = 0;
		virtual int64_t get_tail(T*) = 0;
		int64_t head_pts;
		int64_t tail_pts;
	};

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


    class FrameQueue : public TimeQueue<AVFrame>, public FrameReceiver, public FrameSupplier {
    public:
		void receive(AVFrame * frame, int stream_index) { TimeQueue::receive(frame, stream_index); }
		void terminateFrameReceiver() { terminate(); Queue::flush(); }
		void resumeFrameReceiver() { resume(); }

		AVFrame * supply() { return Queue::pop(); }
		AVFrame * supply(int64_t pts);
		void terminateFrameSupplier() { terminate(); Queue::flush(); }
		void resumeFrameSupplier() { resume(); }

		bool pop(int64_t min_pts, int64_t max_pts);
		size_t flush(int64_t min_pts, int64_t max_pts);

		virtual void free(AVFrame * f);

		using Queue::flush;
		using TimeQueue::get_head;
		using TimeQueue::get_tail;
	
	protected:

		AVFrame * clone(AVFrame * f);
		virtual int64_t get_head(AVFrame * frame);
		virtual int64_t get_tail(AVFrame * frame);
	};
}

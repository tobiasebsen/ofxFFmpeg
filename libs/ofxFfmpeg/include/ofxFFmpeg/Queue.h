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
		~Queue() { clear(); }

        bool push(T * t);
        T * pop();
		T * peek();

        size_t size() const { return queue.size(); }
		size_t capacity() const { return max_size; }
		void resize(size_t s) { max_size = s; }

		void terminate(bool notifyPush = true, bool notifyPop = true);
		void resume();
		bool is_terminated() { return terminated; }

		size_t clear();

		void lock() { mutex.lock(); }
		void unlock() { mutex.unlock(); }

    protected:
		virtual void free(T *) = 0;
		virtual T * clone(T *) = 0;

		size_t max_size;
        std::list<T*> queue;
        std::mutex mutex;
        std::condition_variable cond_push;
        std::condition_variable cond_pop;
		bool terminated;
    };
    
    template<typename T>
    bool Queue<T>::push(T *t) {
        while (queue.size() >= max_size && !terminated) {
            std::unique_lock<std::mutex> lock(mutex);
            cond_pop.wait(lock);
        }
        if (!terminated) {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push_back(clone(t));
            cond_push.notify_one();
			return true;
        }
		return false;
    }
    
    template<typename T>
    T * Queue<T>::pop() {
        while (queue.size() == 0) {
            std::unique_lock<std::mutex> lock(mutex);
			if (terminated) return nullptr;
            cond_push.wait(lock);
        }
        {
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
	T * Queue<T>::peek() {
		std::lock_guard<std::mutex> lock(mutex);
		if (queue.size() > 0) {
			T * front = queue.front();
			return front;
		}
		return nullptr;
	}

	template<typename T>
	void Queue<T>::terminate(bool notifyPush, bool notifyPop) {
		terminated = true;
		if (notifyPush)
			cond_push.notify_all();
		if (notifyPop)
			cond_pop.notify_all();
	}

	template<typename T>
	void Queue<T>::resume() {
		terminated = false;
	}

	template<typename T>
	size_t Queue<T>::clear() {
		std::lock_guard<std::mutex> lock(mutex);
		size_t i = 0;
		while (queue.size() > 0) {
			free(queue.front());
			queue.pop_front();
			i++;
		}
		cond_pop.notify_all();
		return i;
	}

	template<typename T>
	class TimeQueue : public Queue<T> {
	public:

		bool receive(T * t) {
            if (Queue<T>::push(t)) {
				std::lock_guard<std::mutex> lock(mutex);
				if (queue.size() > 0) {
					head_pts = get_head(queue.back());
					if (tail_pts == -1) {
						tail_pts = get_tail(queue.front());
					}
				}
				return true;
			}
			return false;
		}

		T * supply() {
			T * t = Queue<T>::pop();
			{
				std::lock_guard<std::mutex> lock(mutex);
				if (queue.size() > 0) {
					tail_pts = get_tail(queue.front());
				}
				else {
					head_pts = -1;
					tail_pts = -1;
				}
			}
			return t;
		}

		int64_t get_head() const { return head_pts; }
		int64_t get_tail() const { return tail_pts; }

	protected:
		virtual T * clone(T * t) = 0;
		virtual int64_t get_head(T*) = 0;
		virtual int64_t get_tail(T*) = 0;
		int64_t head_pts = -1;
		int64_t tail_pts = -1;
	};

    class PacketQueue : public Queue<AVPacket>, public PacketReceiver, public PacketSupplier {
    public:
		void receive(AVPacket * packet) { push(packet); }
		void terminatePacketReceiver() { terminate(false, true); }
		void resumePacketReceiver() { resume(); }
		bool isPacketsTerminated() { return is_terminated(); }

		AVPacket * supply() { return pop(); }
		void terminatePacketSupplier() { terminate(true, false); }
		void resumePacketSupplier() { resume(); }

        virtual AVPacket * clone(AVPacket * p);
        virtual void free(AVPacket * p);

		using Queue::clear;

		/*void flush(PacketReceiver * receiver) {
			std::lock_guard<std::mutex> lock(mutex);
			while (queue.size() > 0) {
				AVPacket * front = queue.front();
				queue.pop_front();
				receiver->receive(front);
				free(front);
				cond_pop.notify_one();
			}
		}*/
    };


    class FrameQueue : public TimeQueue<AVFrame>, public FrameReceiver, public FrameSupplier {
    public:
		void receive(AVFrame * frame, int stream_index) { TimeQueue::receive(frame); }
		void terminateFrameReceiver() { terminate(false, true); }
		void resumeFrameReceiver() { resume(); }

		AVFrame * supply() { return Queue::pop(); }
		AVFrame * supply(int64_t pts);
		void terminateFrameSupplier() { terminate(true, false); }
		void resumeFrameSupplier() { resume(); }

		/*AVFrame * get(int index) {
			auto it = queue.begin();
			std::advance(it, index);
			return *it;
		}*/

		bool pop(int64_t min_pts, int64_t max_pts);
		size_t clear(int64_t min_pts, int64_t max_pts);

		virtual void free(AVFrame * f);

		using Queue::clear;
		using TimeQueue::get_head;
		using TimeQueue::get_tail;
	
	protected:

		virtual AVFrame * clone(AVFrame * f);
		virtual int64_t get_head(AVFrame * frame);
		virtual int64_t get_tail(AVFrame * frame);
	};
}

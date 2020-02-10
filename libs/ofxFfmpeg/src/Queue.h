#pragma once

#include <queue>
#include <mutex>

namespace ofxFFmpeg {
    
    template<typename T>

    class Queue {
    public:
        Queue(size_t size = 2) : max_size(size) {}

        void push(T * t);
        T * pop();

        size_t size() { return queue.size(); }

    protected:
        size_t max_size;
        std::queue<T*> queue;
        std::mutex mutex;
        std::condition_variable cond_push;
        std::condition_variable cond_pop;
    };
    
    template<typename T>
    void Queue<T>::push(T *t) {
        //std::lock_guard<std::mutex> lock(mutex);
        while (queue.size() >= max_size) {
            std::unique_lock<std::mutex> lock(mutex);
            cond_pop.wait(lock);
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(t);
            cond_push.notify_one();
        }
    }
    
    template<typename T>
    T * Queue<T>::pop() {
        //std::lock_guard<std::mutex> lock(mutex);
        while (queue.size() == 0) {
            std::unique_lock<std::mutex> lock(mutex);
            cond_push.wait(lock);
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            T * back = queue.back();
            queue.pop();
            cond_pop.notify_one();
            return back;
        }
    }
}

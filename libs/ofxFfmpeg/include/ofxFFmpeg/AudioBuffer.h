#pragma once

#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>

namespace ofxFFmpeg {

    template<typename T>
	class AudioBuffer {
	public:
        void allocate(size_t size) {
            buffer.resize(size);
            reset();
        }

		size_t size() const { return buffer.size(); }

        void reset() {
            read_total = 0;
            write_total = 0;
			terminated = false;
		}

		int read(T * dst, int samples);
        int write(T * src, int samples);

		size_t getTotalRead() const { return read_total; }
		size_t getTotalWrite() const { return write_total; }

        int getAvailableRead() const {
            return (write_total - read_total);
        }
        int getAvailableWrite() const {
            return buffer.size() - (write_total - read_total);
        }
        
        void wait(size_t write_samples = 0) {
            while (write_samples > getAvailableWrite() && !terminated) {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock);
            }
        }

		void terminate() {
			terminated = true;
			condition.notify_all();
		}

		void resume() {
			terminated = false;
		}

	protected:
		std::vector<T> buffer;

        size_t read_total;
        size_t write_total;

        std::mutex mutex;
        std::condition_variable condition;
		std::atomic<bool> terminated = false;
	};
    
    template<typename T>
    int AudioBuffer<T>::write(T * src, int samples) {
        
        samples = std::min(samples, getAvailableWrite());
        int write_point = write_total % buffer.size();
        int head_samples = std::min(samples, (int)buffer.size() - write_point);
        memcpy(buffer.data() + write_point, src, head_samples * sizeof(T));
        int tail_samples = samples - head_samples;
        memcpy(buffer.data(), src + head_samples, tail_samples * sizeof(T));
        
        write_total += samples;

        return samples;
    }

    template<typename T>
    int AudioBuffer<T>::read(T * dst, int samples) {

        samples = std::min(samples, getAvailableRead());
        int read_point = read_total % buffer.size();
        int head_samples = std::min(samples, (int)buffer.size() - read_point);
        memcpy(dst, buffer.data() + read_point, head_samples * sizeof(T));
        int tail_samples = samples - head_samples;
        memcpy(dst + head_samples, buffer.data(), tail_samples * sizeof(T));
        
        read_total += samples;
        condition.notify_all();

        return samples;
    }
}

#pragma once

#include <vector>
#include <mutex>

namespace ofxFFmpeg {

    template<typename T>
	class AudioBuffer {
	public:
        void setup(size_t size) {
            buffer.resize(size);
        }

        void reset() {
            read_total = 0;
            write_total = 0;
        }

		int read(T * buffer, int samples);
        int write(T * buffer, int samples);
        
        int get_read_available() {
            return (write_total - read_total);
        }
        int get_write_available() {
            return buffer.size() - (write_total - read_total);
        }

	protected:
		std::vector<T> buffer;

        size_t read_total;
        size_t write_total;

        std::mutex mutex;
        std::condition_variable condition;
	};
    
    template<typename T>
    int AudioBuffer<T>::write(T * buffer, int samples) {
        return 0;
    }

    template<typename T>
    int AudioBuffer<T>::read(T * buffer, int samples) {
        return 0;
    }
}

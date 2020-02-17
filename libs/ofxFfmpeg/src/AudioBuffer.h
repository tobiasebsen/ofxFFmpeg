#pragma once

#include <vector>

namespace ofxFFmpeg {

	class AudioBuffer {
	public:
		void setup(int channels, int samples);

        void reset();

		int read(float * buffer, int frames, int channels, int sample_rate);
        int write(float * buffer, int frames, int channels, int sample_rate);
	
	protected:
		std::vector<float> buffer;
		int samples;
		int channels;
        int sample_rate;

		int read_offset;
		int write_offset;

        uint64_t read_total;
        uint64_t write_total;
	};
}

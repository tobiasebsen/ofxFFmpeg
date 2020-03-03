#pragma once

#include <vector>

namespace ofxFFmpeg {

	class AudioBuffer {
	public:
		void setup(int channels, int samples);

		int read(float * buffer, int frames, int channels, int sample_rate);
	
	protected:
		std::vector<float> buffer;
		int samples;
		int channels;
        int sample_rate;

		int read_offset;
		int write_offset;
	};
}

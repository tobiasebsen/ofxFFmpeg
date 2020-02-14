#pragma once

#include <vector>

namespace ofxFFmpeg {

	class AudioBuffer {
	public:
		void setup(int channels, int num_samples);
	
	protected:
		std::vector<float> buffer;
		int channels;
		int num_samples;
	};
}
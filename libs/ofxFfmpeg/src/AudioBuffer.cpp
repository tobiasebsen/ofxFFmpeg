#include "ofxFFmpeg/AudioBuffer.h"

using namespace ofxFFmpeg;

int AudioBuffer::read(float * read_buf, int frames, int channels, int sample_rate) {

	size_t remain = buffer.size() - read_offset;
	size_t read_frames = frames * channels;
	//size_t first_read = std::minu

	return frames;
}

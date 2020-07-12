#include "ofxFFmpeg/Metrics.h"

extern "C" {
#include "libavutil/time.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
void Metrics::begin() {
	time_begin_last = time_begin;
	time_begin = getTime();
}

//--------------------------------------------------------------
void Metrics::end() {
	time_end = getTime();

	duration_period = time_end - time_begin;
	if (duration_period > duration_period_filtered)
		duration_period_filtered = duration_period;
	else
		duration_period_filtered = duration_period_filtered * 0.96 + duration_period * 0.04;

	duration_frame = time_begin - time_begin_last;
	if (duration_frame > duration_frame_filtered)
		duration_frame_filtered = duration_frame;
	else
		duration_frame_filtered = duration_frame_filtered * 0.96 + duration_frame * 0.04;
}

//--------------------------------------------------------------
int32_t Metrics::getPeriod() const {
	return duration_period;
}

//--------------------------------------------------------------
int32_t Metrics::getPeriodFiltered() const {
	return duration_period_filtered;
}

//--------------------------------------------------------------
float Metrics::getDutyCycle() const {
	return duration_frame ? (float)duration_period / (float)duration_frame : 0;
}

//--------------------------------------------------------------
float Metrics::getDutyCycleFiltered() const {
	return duration_frame_filtered ? (float)duration_period_filtered / (float)duration_frame_filtered : 0;
}

//--------------------------------------------------------------
int64_t Metrics::getTime() {
	return av_gettime_relative();
}

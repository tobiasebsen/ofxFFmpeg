#pragma once

#include <stdint.h>

namespace ofxFFmpeg{

	class Metrics {
	public:

		void begin();
		void end();

		int32_t getPeriod() const;
		int32_t getPeriodFiltered() const;
		float getDutyCycle() const;
		float getDutyCycleFiltered() const;
		float getFrequency() const;

	private:

		int64_t getTime();

		int64_t time_begin;
		int64_t time_begin_last;
		int64_t time_end;

		int32_t duration_period;
		int32_t duration_period_filtered;
		int32_t duration_frame;
		int32_t duration_frame_filtered;
	};
}
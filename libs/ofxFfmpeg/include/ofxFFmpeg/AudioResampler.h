#pragma once

#include "AvTypes.h"
#include "Decoder.h"
#include "Metrics.h"

namespace ofxFFmpeg {

    class AudioResampler {
    public:
        bool allocate(int in_sample_rate, uint64_t in_ch_layout, int in_sample_fmt, int out_sample_rate, uint64_t out_ch_layout, int out_sample_fmt);
        bool allocate(AudioDecoder & decoder, int sample_rate, int channels, int sample_fmt);
		bool isAllocated();
		void free();

        template<typename T>
        static int getSampleFormat() { return getSampleFormat(typeid(T)); }
        static int getSampleFormat(const std::type_info & tinfo);

        int resample(AVFrame * frame, void * out_buffer, int out_samples);
        void * resample(AVFrame * frame, int * out_samples);
        void free(void * buffer);

		const Metrics & getMetrics() const;

    protected:
        SwrContext * swr_context;
        int delay;
        int in_sample_rate;
        int out_sample_rate;
        int out_channels;
        int out_format;

		Metrics metrics;
    };
}

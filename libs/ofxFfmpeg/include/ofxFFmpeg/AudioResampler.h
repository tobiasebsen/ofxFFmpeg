#pragma once

#include "AvTypes.h"
#include "Decoder.h"
#include "Encoder.h"
#include "Metrics.h"

namespace ofxFFmpeg {

    class AudioResampler {
    public:
        bool allocate(int in_sample_rate, uint64_t in_ch_layout, int in_sample_fmt, int out_sample_rate, uint64_t out_ch_layout, int out_sample_fmt);
        bool allocate(AudioDecoder & decoder, int sample_rate, int channels, int sample_fmt);
		bool allocate(int sample_rate, int channels, int sample_fmt, AudioEncoder & encoder);
		bool isAllocated() const;
		void free();

        template<typename T>
        static int getSampleFormat() { return getSampleFormat(typeid(T)); }
        static int getSampleFormat(const std::type_info & tinfo);

		int resample(void * in_buffer, int in_samples, AVFrame * frame);
        int resample(AVFrame * frame, void * out_buffer, int out_samples);
        void * resample(AVFrame * frame, int * out_samples);

		int getInSamples(int out_samples);
		int getOutSamples(int in_samples);
		void * allocateSamplesInput(int nb_samples);
		void * allocateSamplesOutput(int nb_samples);
		void * allocateSamples(int nb_samples, int nb_channels, int format);
        void free(void * buffer);

		const Metrics & getMetrics() const;

    protected:
		int error = 0;
        SwrContext * swr_context = NULL;
        int delay;
        int in_sample_rate;
		int in_channels;
		int in_format;
        int out_sample_rate;
        int out_channels;
        int out_format;

		Metrics metrics;
    };
}

#pragma once

#include "AvTypes.h"
#include "Decoder.h"

namespace ofxFFmpeg {

    class AudioResampler {
    public:
        bool setup(int in_sample_rate, int in_ch_layout, int in_sample_fmt, int out_sample_rate, int out_ch_layout, int out_sample_fmt);
        bool setup(AudioDecoder & decoder, int sample_rate, int channels, int sample_fmt);

        template<typename T>
        static int getSampleFormat() { return getSampleFormat(typeid(T)); }
        static int getSampleFormat(const std::type_info & tinfo);

        int resample(AVFrame * frame, void * out_buffer, int out_samples);
        void * resample(AVFrame * frame, int & out_samples);
        void free(void * buffer);

    protected:
        SwrContext * swr_context;
        int delay;
    };
}

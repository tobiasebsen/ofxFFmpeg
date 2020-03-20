#include "ofxFFmpeg/AudioResampler.h"

extern "C" {
#include "libswresample/swresample.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool AudioResampler::setup(int in_sample_rate, uint64_t in_ch_layout, int in_sample_fmt, int out_sample_rate, uint64_t out_ch_layout, int out_sample_fmt) {
    
    this->in_sample_rate = in_sample_rate;
    this->out_sample_rate = out_sample_rate;
    this->out_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    this->out_format = out_sample_fmt;

    swr_context = swr_alloc_set_opts(NULL, out_ch_layout, (AVSampleFormat)out_sample_fmt, out_sample_rate, in_ch_layout, (AVSampleFormat)in_sample_fmt, in_sample_rate, 0, NULL);
    if (!swr_context) {
        return false;
    }
    
    int error = swr_init(swr_context);
    if (error < 0) {
        return false;
    }
    
    delay = swr_get_delay(swr_context, in_sample_rate);

    return true;
}

//--------------------------------------------------------------
bool AudioResampler::setup(AudioDecoder & decoder, int out_sample_rate, int out_channels, int out_sample_fmt) {
    uint64_t out_ch_layout = av_get_default_channel_layout(out_channels);
    return setup(decoder.getSampleRate(), decoder.getChannelLayout(), decoder.getSampleFormat(), out_sample_rate, out_ch_layout, out_sample_fmt);
}

//--------------------------------------------------------------
int AudioResampler::resample(AVFrame *frame, void *out_buffer, int out_samples) {
    return swr_convert(swr_context, (uint8_t**)&out_buffer, out_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
}

//--------------------------------------------------------------
void * AudioResampler::resample(AVFrame *frame, int * out_samples_ptr) {
    
    int out_samples = av_rescale_rnd(delay + frame->nb_samples, out_sample_rate, in_sample_rate, AV_ROUND_UP);

    uint8_t *out_buffer;
    av_samples_alloc(&out_buffer, NULL, out_channels, out_samples, (AVSampleFormat)out_format, 0);
    
    *out_samples_ptr = resample(frame, out_buffer, out_samples);
    
    return out_buffer;
}

//--------------------------------------------------------------
void AudioResampler::free(void *buffer) {
    av_freep(&buffer);
}

//--------------------------------------------------------------
int AudioResampler::getSampleFormat(const std::type_info & tinfo) {
    if (tinfo.name() == typeid(unsigned char).name())
        return AV_SAMPLE_FMT_U8;
    if (tinfo.name() == typeid(short).name())
        return AV_SAMPLE_FMT_S16;
    if (tinfo.name() == typeid(int).name())
        return AV_SAMPLE_FMT_S32;
    if (tinfo.name() == typeid(float).name())
        return AV_SAMPLE_FMT_FLT;
    if (tinfo.name() == typeid(double).name())
        return AV_SAMPLE_FMT_DBL;
    return AV_SAMPLE_FMT_NONE;
}

#include "ofxFFmpeg/AudioResampler.h"

extern "C" {
#include "libswresample/swresample.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool AudioResampler::allocate(int in_sample_rate, uint64_t in_ch_layout, int in_sample_fmt, int out_sample_rate, uint64_t out_ch_layout, int out_sample_fmt) {

	free();
    
    this->in_sample_rate = in_sample_rate;
	this->in_channels = av_get_channel_layout_nb_channels(in_ch_layout);
	this->in_format = in_sample_fmt;
    this->out_sample_rate = out_sample_rate;
    this->out_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    this->out_format = out_sample_fmt;

    swr_context = swr_alloc_set_opts(NULL, out_ch_layout, (AVSampleFormat)out_sample_fmt, out_sample_rate, in_ch_layout, (AVSampleFormat)in_sample_fmt, in_sample_rate, 0, NULL);
    if (!swr_context) {
        return false;
    }
    
    error = swr_init(swr_context);
    if (error < 0) {
        return false;
    }
    
    delay = swr_get_delay(swr_context, in_sample_rate);

    return true;
}

//--------------------------------------------------------------
bool AudioResampler::allocate(AudioDecoder & decoder, int out_sample_rate, int out_channels, int out_sample_fmt) {
    uint64_t out_ch_layout = av_get_default_channel_layout(out_channels);
    return allocate(decoder.getSampleRate(), decoder.getChannelLayout(), decoder.getSampleFormat(), out_sample_rate, out_ch_layout, out_sample_fmt);
}

//--------------------------------------------------------------
bool AudioResampler::allocate(int in_sample_rate, int in_channels, int in_sample_fmt, AudioEncoder & encoder) {
	uint64_t in_ch_layout = av_get_default_channel_layout(in_channels);
	return allocate(in_sample_rate, in_ch_layout, in_sample_fmt, encoder.getSampleRate(), encoder.getChannelLayout(), encoder.getSampleFormat());
}

//--------------------------------------------------------------
bool AudioResampler::isAllocated() const {
	return swr_context != NULL;
}

//--------------------------------------------------------------
void AudioResampler::free() {
	if (swr_context) {
		swr_free(&swr_context);
	}
}

//--------------------------------------------------------------
int AudioResampler::resample(void * in_buffer, int in_samples, AVFrame * frame) {
	metrics.begin();
	int samples = swr_convert(swr_context, (uint8_t**)frame->extended_data, frame->nb_samples, (const uint8_t**)&in_buffer, in_samples);
	metrics.end();
	return samples;
}

//--------------------------------------------------------------
int AudioResampler::resample(AVFrame *frame, void *out_buffer, int out_samples) {
	metrics.begin();
    int samples = swr_convert(swr_context, (uint8_t**)&out_buffer, out_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
	metrics.end();
	return samples;
}

//--------------------------------------------------------------
void * AudioResampler::resample(AVFrame *frame, int * out_samples_ptr) {
    
	int out_samples = getOutSamples(frame->nb_samples);
    void *out_buffer = allocateSamplesOutput(out_samples);
    
    int samples = resample(frame, out_buffer, out_samples);
	if (out_samples_ptr) {
		*out_samples_ptr = samples >= 0 ? samples : 0;
	}

    return out_buffer;
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

//--------------------------------------------------------------
int AudioResampler::getInSamples(int out_samples) {
	return av_rescale_rnd(out_samples, in_sample_rate, out_sample_rate, AV_ROUND_UP);
}

//--------------------------------------------------------------
int AudioResampler::getOutSamples(int in_samples) {
	//return swr_get_out_samples(swr_context, in_samples);
	return av_rescale_rnd(in_samples, out_sample_rate, in_sample_rate, AV_ROUND_UP);
}

//--------------------------------------------------------------
void * AudioResampler::allocateSamplesInput(int nb_samples) {
	return allocateSamples(nb_samples, in_channels, in_format);
}

//--------------------------------------------------------------
void * AudioResampler::allocateSamplesOutput(int nb_samples) {
	return allocateSamples(nb_samples, out_channels, out_format);
}

//--------------------------------------------------------------
void * AudioResampler::allocateSamples(int nb_samples, int nb_channels, int format) {
	uint8_t * buffer = NULL;
	int error = av_samples_alloc(&buffer, NULL, nb_channels, nb_samples, (AVSampleFormat)format, 0);
	return buffer;
}

//--------------------------------------------------------------
void AudioResampler::free(void *buffer) {
	av_freep(&buffer);
}

//--------------------------------------------------------------
const Metrics & AudioResampler::getMetrics() const {
	return metrics;
}

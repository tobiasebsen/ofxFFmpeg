#include "ofxFFmpeg/Codec.h"

#include <algorithm>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool Codec::allocate(AVCodec * codec) {

	free();

	this->codec = codec;

	if (!codec)
		return false;

	context = avcodec_alloc_context3(codec);
	if (!context) {
		av_log(NULL, AV_LOG_ERROR, "Could not allocate codec context\n");
		return false;
	}

	if (codec->pix_fmts)
		context->pix_fmt = codec->pix_fmts[0];
	if (codec->sample_fmts)
		context->sample_fmt = codec->sample_fmts[0];

	context->gop_size = 15;

	return true;
}

//--------------------------------------------------------------
bool Codec::isAllocated() const {
	return context != NULL;
}

//--------------------------------------------------------------
void Codec::free() {
	codec = NULL;
	if (context) {
		avcodec_free_context(&context);
		context = NULL;
	}
	stream = NULL;
}

//--------------------------------------------------------------
bool Codec::open(AVStream * stream) {

	if (!context) {
		av_log(NULL, AV_LOG_ERROR, "Codec context is not allocated\n");
		return false;
	}

	this->stream = stream;

	if ((error = avcodec_open2(context, context->codec, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open video codec\n");
		stream = NULL;
		return false;
	}

	return true;
}

//--------------------------------------------------------------
bool Codec::isOpen() const {
	return stream != NULL;
}

//--------------------------------------------------------------
void Codec::close() {
	stream = NULL;
	free();
}

//--------------------------------------------------------------
bool Codec::match(AVPacket * packet) {
	return packet && stream && packet->stream_index == stream->index;
}

//--------------------------------------------------------------
int Codec::getId() const {
	return codec ? (int)codec->id : (int)AV_CODEC_ID_NONE;
}

//--------------------------------------------------------------
unsigned int Codec::getTag() const {
	return context ? context->codec_tag : 0;
}

//--------------------------------------------------------------
std::string Codec::getTagString() {
	char buf[AV_FOURCC_MAX_STRING_SIZE];
	return av_fourcc_make_string(buf, context->codec_tag);
}

//--------------------------------------------------------------
std::string Codec::getName() const {
	return codec ? std::string(codec->name) : std::string();
}

//--------------------------------------------------------------
std::string Codec::getLongName() const {
	return codec ? std::string(codec->long_name) : std::string();
}

//--------------------------------------------------------------
int Codec::getStreamIndex() const {
	return stream ? stream->index : -1;
}

//--------------------------------------------------------------
int Codec::getTotalNumFrames() const {
	return stream ? stream->nb_frames : 0;
}

//--------------------------------------------------------------
int Codec::getBitsPerSample() const {
	return context ? context->bits_per_coded_sample : 0;
}

//--------------------------------------------------------------
int64_t Codec::getBitRate() const {
	return context ? context->bit_rate : 0;
}

//--------------------------------------------------------------
void Codec::setBitRate(int64_t bit_rate) {
	if (context) context->bit_rate = bit_rate;
}
//--------------------------------------------------------------
void Codec::setMaxBitRate(int bitRate) {
	if (context) {
		context->rc_min_rate = context->bit_rate;
		context->rc_max_rate = bitRate;
	}
}
//--------------------------------------------------------------
void Codec::setBufferSize(int bufferSize) {
	if (context) context->rc_buffer_size = bufferSize;
}

//--------------------------------------------------------------
double Codec::getTimeBase() const {
	return stream ? av_q2d(stream->time_base) : 1;
}

//--------------------------------------------------------------
double Codec::getDurationSeconds() const {
	return stream ? av_rescale_q(stream->duration, stream->time_base, { 1, AV_TIME_BASE }) : 0;
}

//--------------------------------------------------------------
int64_t Codec::getDuration() const {
	return stream->duration;
}

//--------------------------------------------------------------
int Codec::getError() const {
	return 0;
}

//--------------------------------------------------------------
std::string Codec::getErrorString() const {
	char errstr[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(error, errstr, sizeof(errstr));
	return std::string(errstr);
}

//--------------------------------------------------------------
AVCodecContext * Codec::getContext() const {
	return context;
}

//--------------------------------------------------------------
const AVCodec * Codec::getCodec() const {
	return context ? context->codec : NULL;
}

//--------------------------------------------------------------
int VideoCodec::getWidth() const {
	return context ? context->width : 0;
}

//--------------------------------------------------------------
void VideoCodec::setWidth(int width) {
	if (context) context->width = width;
}

//--------------------------------------------------------------
int VideoCodec::getHeight() const {
	return context ? context->height : 0;
}

//--------------------------------------------------------------
void VideoCodec::setHeight(int height) {
	if (context) context->height = height;
}

//--------------------------------------------------------------
int VideoCodec::getPixelFormat() const {
	return context ? context->pix_fmt : AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
void VideoCodec::setPixelFormat(int pix_fmt) {
	if (context) context->pix_fmt = (AVPixelFormat)pix_fmt;
}

//--------------------------------------------------------------
int VideoCodec::getCodedWidth() const {
	return context ? context->coded_width : 0;
}

//--------------------------------------------------------------
int VideoCodec::getCodedHeight() const {
	return context ? context->coded_height : 0;
}

//--------------------------------------------------------------
double VideoCodec::getFrameRate() const {
	if (context && context->framerate.num > 0)
		return context ? av_q2d(context->framerate) : 0;
	else if (stream)
		return av_q2d(stream->r_frame_rate);
	else
		return 0;
}

//--------------------------------------------------------------
void VideoCodec::setFrameRate(double frame_rate) {
	if (context) {
		context->framerate = av_d2q(frame_rate, 1000);
		context->time_base = av_inv_q(context->framerate);
	}
}
//--------------------------------------------------------------
void VideoCodec::setProfile(int profile) {
	if (context) context->profile = profile;
}
//--------------------------------------------------------------
void VideoCodec::setLevel(int level) {
	if (context) context->level = level;
}
//--------------------------------------------------------------
void VideoCodec::setKeyFrame(int key_frame) {
	if (context) context->gop_size = key_frame;
}

//--------------------------------------------------------------
int AudioCodec::getSampleRate() const {
	return context ? context->sample_rate : 0;
}

//--------------------------------------------------------------
void AudioCodec::setSampleRate(int sample_rate) {
	if (context) {
		context->sample_rate = sample_rate;
		context->time_base = av_make_q(1, context->sample_rate);
	}
}

//--------------------------------------------------------------
int AudioCodec::getNumChannels() const {
	return context ? context->channels : 0;
}

//--------------------------------------------------------------
void AudioCodec::setNumChannels(int nb_channels) {
	if (context) {
		context->channels = nb_channels;
		context->channel_layout = av_get_default_channel_layout(nb_channels);
	}
}

//--------------------------------------------------------------
int AudioCodec::getSampleFormat() const {
	return context ? context->sample_fmt : AV_SAMPLE_FMT_NONE;
}

//--------------------------------------------------------------
int AudioCodec::getChannelLayout() const {
	return context ? context->channel_layout : 0;
}

//--------------------------------------------------------------
int AudioCodec::getNumSamples() const {
	return context ? context->frame_size : 0;
}

//--------------------------------------------------------------
Codecs Codecs::getCodecs() {
	Codecs codecs;
	const AVCodec *codec = nullptr;
	void *i = 0;
	while ((codec = av_codec_iterate(&i))) {
		codecs.push_back(codec);
	}
	return codecs;
}

//--------------------------------------------------------------
Codecs Codecs::getID(const Codecs & codecs, int codec_id) {
	Codecs hwcodecs;
	std::copy_if(codecs.begin(), codecs.end(), std::back_inserter(hwcodecs), [codec_id](const AVCodec * c) {return c->id == (AVCodecID)codec_id; });
	return hwcodecs;
}

//--------------------------------------------------------------
Codecs Codecs::getHardware(const Codecs & codecs) {
	Codecs hwcodecs;
	std::copy_if(codecs.begin(), codecs.end(), std::back_inserter(hwcodecs), [](const AVCodec * c) {return c->capabilities & AV_CODEC_CAP_HARDWARE; });
	return hwcodecs;
}
//--------------------------------------------------------------
Codecs Codecs::getDecode(const Codecs & codecs) {
	Codecs decoders;
	std::copy_if(codecs.begin(), codecs.end(), std::back_inserter(decoders), [](const AVCodec * c) {return av_codec_is_decoder(c) != 0; });
	return decoders;
}
//--------------------------------------------------------------
Codecs Codecs::getEncode(const Codecs & codecs) {
	Codecs encoders;
	std::copy_if(codecs.begin(), codecs.end(), std::back_inserter(encoders), [](const AVCodec * c) {return av_codec_is_encoder(c) != 0; });
	return encoders;
}
//--------------------------------------------------------------
Codecs Codecs::getVideo(const Codecs & codecs) {
	Codecs video;
	std::copy_if(codecs.begin(), codecs.end(), std::back_inserter(video), [](const AVCodec * c) {return c->type == AVMEDIA_TYPE_VIDEO; });
	return video;
}
//--------------------------------------------------------------
Codecs Codecs::getAudio(const Codecs & codecs) {
	Codecs audio;
	std::copy_if(codecs.begin(), codecs.end(), std::back_inserter(audio), [](const AVCodec * c) {return c->type == AVMEDIA_TYPE_AUDIO; });
	return audio;
}

//--------------------------------------------------------------
std::vector<std::string> Codecs::getNames() {
	std::vector<std::string> names;
	std::transform(begin(), end(), std::back_inserter(names), [](const AVCodec * c) {return c->name; });
	return names;
}

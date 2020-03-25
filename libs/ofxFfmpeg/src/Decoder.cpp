#include "ofxFFmpeg/Decoder.h"
#include "ofxFFmpeg/VideoScaler.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
static enum AVPixelFormat get_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {

	for (auto p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
		if (VideoScaler::supportsInput(*p))
			return *p;
	}
	return AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
bool ofxFFmpeg::Decoder::allocate(AVCodec * codec, AVStream * stream) {

	this->codec = codec;
	this->stream = stream;
	this->stream_index = stream->index;

	if ((codec_context = avcodec_alloc_context3(codec)) == NULL) {
		av_log(NULL, AV_LOG_ERROR, "Cannot allocate codec context\n");
		return false;
	}

	avcodec_parameters_to_context(codec_context, stream->codecpar);
	//codec_context->get_format = get_format;

	return true;
}

//--------------------------------------------------------------
bool Decoder::open(AVStream * stream) {

	close();
   
    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Codec not found\n");
        return false;
    }
 
	if (!allocate(codec, stream)) {
		return false;
	}

	if (!open(codec)) {
		return false;
	}

	return true;
}

//--------------------------------------------------------------
bool Decoder::open(AVCodec * codec) {

	this->codec = codec;

	if ((error = avcodec_open2(codec_context, codec, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open codec context\n");
		return false;
	}

	return true;
}

//--------------------------------------------------------------
void Decoder::close() {
    stop();
    if (codec_context) {
		if (codec_context->hw_device_ctx) {
			av_buffer_unref(&codec_context->hw_device_ctx);
		}
        avcodec_close(codec_context);
        codec_context = NULL;
    }
	stream = NULL;
	stream_index = -1;
}

//--------------------------------------------------------------
bool ofxFFmpeg::Decoder::isOpen() {
	return codec_context != NULL;
}

//--------------------------------------------------------------
bool Decoder::match(AVPacket * packet) {
    return packet->stream_index == stream_index;
}

//--------------------------------------------------------------
bool Decoder::send(AVPacket *packet) {
    error = avcodec_send_packet(codec_context, packet);
    return error >= 0;
}

//--------------------------------------------------------------
bool Decoder::receive(AVFrame *frame) {
    error = avcodec_receive_frame(codec_context, frame);
    return error >= 0;
}

//--------------------------------------------------------------
AVFrame * Decoder::receive() {
    AVFrame * frame = av_frame_alloc();
    error = avcodec_receive_frame(codec_context, frame);
    if (error < 0) {
        av_frame_free(&frame);
    }
    return frame;
}

//--------------------------------------------------------------
void Decoder::free(AVFrame *frame) {
    av_frame_unref(frame);
}

//--------------------------------------------------------------
bool Decoder::decode(AVPacket *packet, FrameReceiver * receiver) {
	std::lock_guard<std::mutex> lock(mutex);

	if (!isOpen())
		return false;

	metrics.begin();

	if (!send(packet))
		return false;

    AVFrame * frame = NULL;
    while ((frame = receive()) != NULL) {

		metrics.end();
		receiver->receive(frame, stream_index);
        free(frame);
    }
    return true;
}

//--------------------------------------------------------------
bool Decoder::flush(FrameReceiver * receiver) {
	if (!decode(NULL, receiver))
		return false;

	avcodec_flush_buffers(codec_context);

	return true;
}

//--------------------------------------------------------------
bool Decoder::start(PacketSupplier * supplier, FrameReceiver * receiver) {
    if (!isOpen() || running)
        return false;
    
	this->supplier = supplier;
	this->supplier->resumePacketSupplier();
	this->receiver = receiver;
	this->receiver->resumeFrameReceiver();

    running = true;
    thread_obj = new std::thread(&Decoder::decodeThread, this);

    return true;
}

//--------------------------------------------------------------
void Decoder::stop() {
    if (running && thread_obj) {
        running = false;
		receiver->terminateFrameReceiver();
		supplier->terminatePacketSupplier();
        if (thread_obj->joinable()) thread_obj->join();
		delete thread_obj;
		thread_obj = NULL;
    }
}

//--------------------------------------------------------------
void Decoder::decodeThread() {

    while (running) {
        auto packet = supplier->supply();
        if (packet && stream_index == packet->stream_index) {

            decode(packet, receiver);

            supplier->free(packet);
        }
    }
}

//--------------------------------------------------------------
int Decoder::getTotalNumFrames() const {
    return stream ? stream->nb_frames : 0;
}

//--------------------------------------------------------------
std::string Decoder::getName() {
	return codec_context->codec->name;
}

//--------------------------------------------------------------
std::string Decoder::getLongName() {
	return codec_context->codec->long_name;
}

//--------------------------------------------------------------
int Decoder::getStreamIndex() const {
    return stream_index;
}

//--------------------------------------------------------------
int Decoder::getBitsPerSample() const {
    return codec_context ? codec_context->bits_per_coded_sample : 0;
}

//--------------------------------------------------------------
uint64_t Decoder::getBitRate() const {
    return codec_context ? codec_context->bit_rate : 0;
}

//--------------------------------------------------------------
double Decoder::getTimeBase() const {
    return stream ? av_q2d(stream->time_base) : 0.;
}

//--------------------------------------------------------------
uint64_t Decoder::getTimeStamp(AVPacket * packet) const {
	return av_rescale_q(packet->pts, stream->time_base, { 1, AV_TIME_BASE });
}

//--------------------------------------------------------------
uint64_t Decoder::getTimeStamp(AVFrame * frame) const {
	return av_rescale_q(frame->pts, stream->time_base, { 1, AV_TIME_BASE });
	//return frame->pts;
}

//--------------------------------------------------------------
uint64_t Decoder::getTimeStamp(int frame_num) const {
	return av_rescale_q(frame_num, { AV_TIME_BASE, 1 }, codec_context->framerate);
}

//--------------------------------------------------------------
int Decoder::getFrameNum(uint64_t pts) const {
	return av_rescale_q(pts, codec_context->framerate, { AV_TIME_BASE, 1 });
}

//--------------------------------------------------------------
const Metrics & ofxFFmpeg::Decoder::getMetrics() const {
	return metrics;
}

//--------------------------------------------------------------
bool VideoDecoder::open(Reader & reader, HardwareDecoder * hw_decoder) {
	
	int stream_index = reader.getVideoStreamIndex();
	if (stream_index == -1)
		return false;

	AVCodec * codec = reader.getVideoCodec();
	AVStream * stream = reader.getStream(stream_index);

	if (!Decoder::allocate(codec, stream))
		return false;

	hw_config = hw_decoder ? hw_decoder->getConfig(codec) : NULL;

	if (hw_config) {
		if (hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
			codec_context->hw_device_ctx = av_buffer_ref(hw_decoder->getContext());
	}

	return Decoder::open(codec);
}

//--------------------------------------------------------------
bool VideoDecoder::decode(AVPacket * packet, FrameReceiver * receiver) {
	std::lock_guard<std::mutex> lock(mutex);

	if (!isOpen())
		return false;

	metrics.begin();

	if (!send(packet))
		return false;

	AVFrame * frame = NULL;
	while ((frame = receive()) != NULL) {

		if (getIsHardwareFrame(frame)) {
			AVFrame * sw_frame = HardwareDecoder::transfer(frame);
			metrics.end();
			receiver->receive(sw_frame, stream_index);
			HardwareDecoder::free(sw_frame);
		}
		else {
			metrics.end();
			receiver->receive(frame, stream_index);
		}
		free(frame);
	}
	return true;
}

//--------------------------------------------------------------
int VideoDecoder::getWidth() const {
	return codec_context ? codec_context->width : 0;
}

//--------------------------------------------------------------
int VideoDecoder::getHeight() const {
	return codec_context ? codec_context->height : 0;
}

//--------------------------------------------------------------
int VideoDecoder::getPixelFormat() const {
	if (hw_config) return AV_PIX_FMT_NV12;// hw_config->pix_fmt;
    return codec_context ? codec_context->pix_fmt : AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
int VideoDecoder::getNumPlanes() const {
	return av_pix_fmt_count_planes((AVPixelFormat)getPixelFormat());
}

//--------------------------------------------------------------
bool VideoDecoder::getIsHardwareFrame(AVFrame * frame) {
	return hw_config && (frame->format == hw_config->pix_fmt);
}

//--------------------------------------------------------------
double VideoDecoder::getFrameRate() {
	return av_q2d(codec_context->framerate);
}

//--------------------------------------------------------------
bool AudioDecoder::open(Reader & reader) {
	int stream_index = reader.getAudioStreamIndex();
	if (stream_index < 0)
		return false;
	AVStream * stream = reader.getStream(stream_index);
	return Decoder::open(stream);
}

//--------------------------------------------------------------
int AudioDecoder::getNumChannels() const {
	return codec_context->channels;
}

//--------------------------------------------------------------
uint64_t AudioDecoder::getChannelLayout() const {
	return codec_context->channel_layout;
}

//--------------------------------------------------------------
int AudioDecoder::getSampleRate() const {
	return codec_context->sample_rate;
}

//--------------------------------------------------------------
int AudioDecoder::getSampleFormat() const {
	return codec_context->sample_fmt;
}

//--------------------------------------------------------------
int AudioDecoder::getFrameSize() const {
	return codec_context->frame_size;
}

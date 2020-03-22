#include "ofxFFmpeg/Decoder.h"

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
bool Decoder::open(AVStream * stream) {

	close();

    this->stream = stream;
    
    AVCodec * codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Codec not found\n");
        return false;
    }
    
    if ((codec_context = avcodec_alloc_context3(codec)) == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Cannot allocate codec context\n");
        return false;
    }
    
    avcodec_parameters_to_context(codec_context, stream->codecpar);

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
        avcodec_close(codec_context);
        codec_context = NULL;
    }
	stream = NULL;
}

//--------------------------------------------------------------
bool ofxFFmpeg::Decoder::isOpen() {
	return codec_context != NULL;
}

//--------------------------------------------------------------
bool Decoder::match(AVPacket * packet) {
    return stream && (packet->stream_index == stream->index);
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
    av_frame_free(&frame);
}

//--------------------------------------------------------------
bool Decoder::decode(AVPacket *packet, FrameReceiver * receiver) {
	if (send(packet)) {

        AVFrame * frame = NULL;
        while ((frame = receive()) != NULL) {

			receiver->receive(frame, stream->index);

            free(frame);
        }
        return true;
    }
    return false;
}

//--------------------------------------------------------------
bool Decoder::flush(FrameReceiver * receiver) {
	if (!decode(NULL, receiver))
		return false;

	avcodec_flush_buffers(codec_context);

	return true;
}

//--------------------------------------------------------------
void ofxFFmpeg::Decoder::copy(AVFrame * src_frame, uint8_t * dst_data, int dst_size, int align) {
	error = av_image_copy_to_buffer(dst_data, dst_size, src_frame->data, src_frame->linesize, (AVPixelFormat)src_frame->format, src_frame->width, src_frame->height, align);
	if (error < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot copy image to buffer\n");
	}
}

//--------------------------------------------------------------
bool Decoder::start(PacketSupplier * supplier, FrameReceiver * receiver) {
    if (running)
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
        thread_obj->join();
		delete thread_obj;
		thread_obj = NULL;
    }
}

//--------------------------------------------------------------
void Decoder::decodeThread() {

    while (running) {
        auto packet = supplier->supply();
        if (packet && stream->index == packet->stream_index) {

            decode(packet, receiver);

            supplier->free(packet);
        }
    }
    running = false;
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
    return stream ? stream->index : -1;
}

//--------------------------------------------------------------
int Decoder::getBitsPerSample() const {
    return stream ? stream->codecpar->bits_per_coded_sample : 0;
}

//--------------------------------------------------------------
uint64_t Decoder::getBitRate() const {
    return stream ? stream->codecpar->bit_rate : 0;
}

//--------------------------------------------------------------
double Decoder::getTimeBase() const {
    return stream ? av_q2d(stream->time_base) : 0.;
}

//--------------------------------------------------------------
uint64_t ofxFFmpeg::Decoder::getTimeStamp(AVPacket * packet) const {
	return av_rescale_q(packet->pts, stream->time_base, { 1, AV_TIME_BASE });
}

//--------------------------------------------------------------
uint64_t Decoder::getTimeStamp(AVFrame * frame) const {
	return av_rescale_q(frame->pts, stream->time_base, { 1, AV_TIME_BASE });
	//return frame->pts;
}

//--------------------------------------------------------------
uint64_t Decoder::getTimeStamp(int frame_num) const {
	return av_rescale_q(frame_num, { AV_TIME_BASE, 1 }, stream->r_frame_rate);
}

//--------------------------------------------------------------
int ofxFFmpeg::Decoder::getFrameNum(uint64_t pts) const {
	return av_rescale_q(pts, stream->avg_frame_rate, { AV_TIME_BASE, 1 });
}

//--------------------------------------------------------------
bool VideoDecoder::open(Reader & reader) {
	int stream_index = reader.getVideoStreamIndex();
	if (stream_index == -1)
		return false;
	AVStream * stream = reader.getStream(stream_index);
	return Decoder::open(stream);
}

//--------------------------------------------------------------
int VideoDecoder::getWidth() const {
    return stream ? stream->codecpar->width : 0;
}

//--------------------------------------------------------------
int VideoDecoder::getHeight() const {
    return stream ? stream->codecpar->height : 0;
}

//--------------------------------------------------------------
int VideoDecoder::getPixelFormat() const {
    return codec_context ? codec_context->pix_fmt : AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
double VideoDecoder::getFrameRate() {
	return stream ? av_q2d(stream->r_frame_rate) : 0;
}

//--------------------------------------------------------------
bool ofxFFmpeg::AudioDecoder::open(Reader & reader) {
	int stream_index = reader.getAudioStreamIndex();
	if (stream_index < 0)
		return false;
	AVStream * stream = reader.getStream(stream_index);
	return Decoder::open(stream);
}

//--------------------------------------------------------------
int AudioDecoder::getNumChannels() const {
    return stream->codecpar->channels;
}

//--------------------------------------------------------------
uint64_t AudioDecoder::getChannelLayout() const {
    return stream->codecpar->channel_layout;
}

//--------------------------------------------------------------
int AudioDecoder::getSampleRate() const {
    return stream->codecpar->sample_rate;
}

//--------------------------------------------------------------
int AudioDecoder::getSampleFormat() const {
    return stream->codecpar->format;
}

//--------------------------------------------------------------
int AudioDecoder::getFrameSize() const {
    return stream->codecpar->frame_size;
}

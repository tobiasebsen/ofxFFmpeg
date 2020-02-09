#include "Decoder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool Decoder::open(AVStream * stream) {

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
bool Decoder::match(AVPacket * packet) {
    return packet->stream_index == stream->index;
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

            receiver->receiveFrame(frame, stream->index);

            free(frame);
        }
        return true;
    }
    return false;
}

//--------------------------------------------------------------
bool ofxFFmpeg::Decoder::flush(FrameReceiver * receiver) {
	return decode(NULL, receiver);
}

//--------------------------------------------------------------
bool Decoder::start(PacketSupplier * supplier, FrameReceiver * receiver) {
    if (running)
        return false;
    
    running = true;
    threadObj = new std::thread(&Decoder::decodeThread, this, supplier, receiver);

    return true;
}

//--------------------------------------------------------------
void Decoder::stop() {
    if (running) {
        running = false;
        threadObj->join();
    }
}

//--------------------------------------------------------------
void Decoder::decodeThread(PacketSupplier * supplier, FrameReceiver * receiver) {

    while (running) {
        auto packet = supplier->supplyPacket();
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
bool VideoDecoder::open(Reader & reader) {
	int stream_index = reader.getVideoStreamIndex();
	if (stream_index == -1)
		return false;
	AVStream * stream = reader.getStream(stream_index);
	return Decoder::open(stream);
}

//--------------------------------------------------------------
int VideoDecoder::getWidth() const {
    return stream->codecpar->width;
}

//--------------------------------------------------------------
int VideoDecoder::getHeight() const {
    return stream->codecpar->height;
}

//--------------------------------------------------------------
int VideoDecoder::getPixelFormat() const {
    return codec_context->pix_fmt;
}

bool ofxFFmpeg::AudioDecoder::open(Reader & reader) {
	int stream_index = reader.getAudioStreamIndex();
	if (stream_index == -1)
		return false;
	AVStream * stream = reader.getStream(stream_index);
	return Decoder::open(stream);
}

//--------------------------------------------------------------
int AudioDecoder::getNumChannels() const {
    return stream->codecpar->channels;
}

//--------------------------------------------------------------
int AudioDecoder::getSampleRate() const {
    return stream->codecpar->sample_rate;
}

//--------------------------------------------------------------
int AudioDecoder::getFrameSize() const {
    return stream->codecpar->frame_size;
}

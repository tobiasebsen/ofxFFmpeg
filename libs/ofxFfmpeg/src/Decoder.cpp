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
/*static enum AVPixelFormat get_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {

	for (auto p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
		av_log(NULL, AV_LOG_INFO, "%d\n", *p);
		if (VideoScaler::supportsInput(*p))
			return *p;
	}
	return AV_PIX_FMT_NONE;
}*/

//--------------------------------------------------------------
bool Decoder::allocate(AVCodec * codec, AVStream * stream) {

	if (!codec || !stream)
		return false;

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

	if (!stream)
		return false;
  
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

		if (codec_context->hw_frames_ctx) {
			av_buffer_unref(&codec_context->hw_frames_ctx);
		}
		if (codec_context->hw_device_ctx) {
			av_buffer_unref(&codec_context->hw_device_ctx);
		}
		//avcodec_close(codec_context);
		avcodec_free_context(&codec_context);
        codec_context = NULL;
    }
	codec = NULL;
	stream = NULL;
	stream_index = -1;
}

//--------------------------------------------------------------
bool Decoder::isOpen() const {
	return codec_context != NULL;
}

//--------------------------------------------------------------
bool Decoder::match(AVPacket * packet) {
    return packet && packet->stream_index == stream_index;
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
	av_frame_free(&frame);
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
void Decoder::flush() {
	if (isOpen()) {
		if (running) {
			flushing = true;
			running = false;
			supplier->terminatePacketSupplier();
		}
		else {
			avcodec_flush_buffers(codec_context);
		}
	}
}

//--------------------------------------------------------------
bool Decoder::start(PacketSupplier * supplier, FrameReceiver * receiver) {
    if (!isOpen())
        return false;

	stop();
    
	this->supplier = supplier;
	this->supplier->resumePacketSupplier();
	this->receiver = receiver;
	this->receiver->resumeFrameReceiver();

	flushing = false;
    running = true;
    thread_obj = new std::thread(&Decoder::decodeThread, this);

    return true;
}

//--------------------------------------------------------------
void Decoder::stop() {
	if (running) {
		flushing = false;
		running = false;
	}
	if (thread_obj) {
		receiver->terminateFrameReceiver();
		supplier->terminatePacketSupplier();
		if (thread_obj->joinable())
			thread_obj->join();
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
		else if (flushing) {
			flush(receiver);
			flushing = false;
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
int64_t Decoder::getBitRate() const {
    return codec_context ? codec_context->bit_rate : 0;
}

//--------------------------------------------------------------
double Decoder::getTimeBase() const {
    return stream ? av_q2d(stream->time_base) : 0.;
}

//--------------------------------------------------------------
int64_t Decoder::rescaleTime(int64_t ts) const {
	if (!stream) return -1;
	return av_rescale_q(ts, stream->time_base, { 1, AV_TIME_BASE });
}

//--------------------------------------------------------------
int64_t Decoder::rescaleTimeInv(int64_t ts) const {
	if (!stream) return -1;
	return av_rescale_q(ts, { 1, AV_TIME_BASE }, stream->time_base);
}

//--------------------------------------------------------------
int64_t Decoder::rescaleTime(AVPacket * packet) const {
	return rescaleTime(packet->pts);
}

//--------------------------------------------------------------
int64_t Decoder::rescaleTime(AVFrame * frame) const {
	return rescaleTime(frame->pts);
}

//--------------------------------------------------------------
int64_t Decoder::rescaleDuration(AVFrame * frame) const {
	return rescaleTime(frame->pkt_duration);
}

//--------------------------------------------------------------
int64_t Decoder::rescaleFrameNum(int frame_num) const {
	if (!stream) return -1;
	return av_rescale_q(frame_num, { AV_TIME_BASE, 1 }, stream->avg_frame_rate);
}

//--------------------------------------------------------------
int64_t Decoder::getTimeStamp(AVFrame * frame) const {
	return frame->pts;
}

//--------------------------------------------------------------
int Decoder::getFrameNum(int64_t pts) const {
	return av_rescale_q(pts, stream->avg_frame_rate, { AV_TIME_BASE, 1 });
}

//--------------------------------------------------------------
int Decoder::getFrameNum(AVFrame * frame) const {
	return getFrameNum(rescaleTime(frame->pts));
}

//--------------------------------------------------------------
uint8_t * Decoder::getFrameData(AVFrame * frame, int plane) {
	return frame->data[plane];
}

//--------------------------------------------------------------
bool Decoder::hasHardwareDecoder() {
	return codec_context && (codec_context->hw_device_ctx != NULL);
}

//--------------------------------------------------------------
const Metrics & Decoder::getMetrics() const {
	return metrics;
}

//--------------------------------------------------------------
bool VideoDecoder::open(Reader & reader) {
	
	AVCodec * codec = reader.getVideoCodec();
	AVStream * stream = reader.getVideoStream();

	if (!Decoder::allocate(codec, stream))
		return false;

	if (!Decoder::open(codec))
		return false;

	return true;
}

#define 	FFALIGN(x, a)   (((x)+(a)-1)&~((a)-1))

//--------------------------------------------------------------
static enum AVPixelFormat get_format(struct AVCodecContext *s, const enum AVPixelFormat * fmt) {
	s->hw_frames_ctx = av_hwframe_ctx_alloc(s->hw_device_ctx);
	AVHWDeviceContext * device_ctx = (AVHWDeviceContext*)s->hw_device_ctx->data;
	AVHWFramesContext * frames_ctx = (AVHWFramesContext*)s->hw_frames_ctx->data;
	
	frames_ctx->sw_format = AV_PIX_FMT_NV12; //s->sw_pix_fmt;
	frames_ctx->width = FFALIGN(s->coded_width, 32);
	frames_ctx->height = FFALIGN(s->coded_height, 32);
	frames_ctx->initial_pool_size = 4;

	if (device_ctx->type == AV_HWDEVICE_TYPE_DXVA2) {

		while (fmt[0] != AV_PIX_FMT_NONE) {
			fmt++;
		}

		frames_ctx->format = AV_PIX_FMT_DXVA2_VLD;
		int ret = av_hwframe_ctx_init(s->hw_frames_ctx);
		if (ret < 0) {
			char buf[256];
			av_strerror(ret, buf, sizeof(buf));
			av_log(NULL, AV_LOG_ERROR, buf);
		}
		return AV_PIX_FMT_DXVA2_VLD;
	}

	return AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
bool VideoDecoder::open(Reader & reader, HardwareDevice & hardware) {

	AVCodec * codec = reader.getVideoCodec();
	AVStream * stream = reader.getVideoStream();

	if (!Decoder::allocate(codec, stream))
		return false;

	if (hardware.isOpen()) {
		hw_config = hardware.getConfig(codec);
		if (hw_config) {
			if (hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
				codec_context->hw_device_ctx = av_buffer_ref(hardware.getContextRef());
			/*if (hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX)
				codec_context->get_format = get_format;*/
		}
	}

	if (!Decoder::open(codec))
		return false;

	return true;
}

//--------------------------------------------------------------
int VideoDecoder::getWidth() const {
	return codec_context ? codec_context->width : 0;
}

//--------------------------------------------------------------
int VideoDecoder::getWidth(AVFrame * frame) const {
	return frame->width;
}

//--------------------------------------------------------------
int VideoDecoder::getHeight() const {
	return codec_context ? codec_context->height : 0;
}

//--------------------------------------------------------------
int VideoDecoder::getHeight(AVFrame * frame) const {
	return frame->height;
}

//--------------------------------------------------------------
int VideoDecoder::getPixelFormat() const {
	//if (hw_config) return AV_PIX_FMT_NV12;// hw_config->pix_fmt;
    return codec_context ? codec_context->pix_fmt : AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
int VideoDecoder::getPixelFormat(AVFrame * frame) const {
	return frame->format;
}

//--------------------------------------------------------------
int VideoDecoder::getNumPlanes() const {
	return av_pix_fmt_count_planes((AVPixelFormat)getPixelFormat());
}

//--------------------------------------------------------------
int VideoDecoder::getLineSize(AVFrame * frame, int plane) const {
	return frame->linesize[plane];
}

//--------------------------------------------------------------
bool VideoDecoder::isKeyFrame(AVPacket * packet) {
	return packet->flags & AV_PKT_FLAG_KEY;
}

//--------------------------------------------------------------
bool VideoDecoder::isHardwareFrame(AVFrame * frame) {
	return hw_config && (hw_config->pix_fmt == frame->format);
}

//--------------------------------------------------------------
double VideoDecoder::getFrameRate() {
	return av_q2d(stream->avg_frame_rate);
}

//--------------------------------------------------------------
std::string PixelFormat::getName() {
	return av_get_pix_fmt_name((AVPixelFormat)format);
}

//--------------------------------------------------------------
int PixelFormat::getBitsPerPixel() {
	if (!desc) desc = av_pix_fmt_desc_get((AVPixelFormat)format);
	return av_get_bits_per_pixel(desc);
}

//--------------------------------------------------------------
int PixelFormat::getNumPlanes() {
	return av_pix_fmt_count_planes((AVPixelFormat)format);
}

//--------------------------------------------------------------
bool PixelFormat::hasAlpha() {
	if (!desc) desc = av_pix_fmt_desc_get((AVPixelFormat)format);
	return desc->flags & AV_PIX_FMT_FLAG_ALPHA;
}

//--------------------------------------------------------------
bool AudioDecoder::open(Reader & reader) {
	AVStream * stream = reader.getAudioStream();
	return stream && Decoder::open(stream);
}

//--------------------------------------------------------------
int AudioDecoder::getNumChannels() const {
	return codec_context->channels;
}

//--------------------------------------------------------------
uint64_t AudioDecoder::getChannelLayout() const {
	if (codec_context->channel_layout != 0)
		return codec_context->channel_layout;
	else
		return av_get_default_channel_layout(codec_context->channels);
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

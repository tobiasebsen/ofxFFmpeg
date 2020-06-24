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
bool Decoder::open(AVCodec * codec, AVStream * stream) {

	if (!isAllocated() && !allocate(codec))
		return false;

	avcodec_parameters_to_context(context, stream->codecpar);

	if (!Codec::open(stream)) {
		return false;
	}

	return true;
}

//--------------------------------------------------------------
void Decoder::close() {
    stop();
	Codec::free();
}


//--------------------------------------------------------------
bool Decoder::send(AVPacket *packet) {
    error = avcodec_send_packet(context, packet);
    return error >= 0;
}

//--------------------------------------------------------------
bool Decoder::receive(AVFrame *frame) {
    error = avcodec_receive_frame(context, frame);
    return error >= 0;
}

//--------------------------------------------------------------
AVFrame * Decoder::receive() {
    AVFrame * frame = av_frame_alloc();
    error = avcodec_receive_frame(context, frame);
    if (error < 0) {
        av_frame_free(&frame);
    }
    return frame;
}

//--------------------------------------------------------------
bool Decoder::decode(AVPacket *packet, FrameReceiver * receiver) {
	std::lock_guard<std::mutex> lock(mutex);

	if (!isAllocated())
		return false;

	metrics.begin();

	if (!send(packet))
		return false;

    AVFrame * frame = NULL;
    while ((frame = receive()) != NULL) {

		metrics.end();
		receiver->receive(frame, stream->index);
        av_frame_free(&frame);
    }
    return true;
}

//--------------------------------------------------------------
bool Decoder::flush(FrameReceiver * receiver) {
	if (!decode(NULL, receiver))
		return false;

	avcodec_flush_buffers(context);

	return true;
}

//--------------------------------------------------------------
void Decoder::flush() {
	if (isAllocated()) {
		if (running) {
			flushing = true;
			running = false;
			supplier->terminatePacketSupplier();
		}
		else {
			avcodec_flush_buffers(context);
		}
	}
}

//--------------------------------------------------------------
bool Decoder::start(PacketSupplier * supplier, FrameReceiver * receiver) {
    if (!isAllocated() || !isOpen())
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
        if (packet && stream->index == packet->stream_index) {

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
int Decoder::getFrameNum(int64_t pts) const {
	return av_rescale_q(pts, stream->avg_frame_rate, { AV_TIME_BASE, 1 });
}

//--------------------------------------------------------------
int Decoder::getFrameNum(AVFrame * frame) const {
	return getFrameNum(rescaleTime(frame->pts));
}

//--------------------------------------------------------------
bool Decoder::hasHardwareDecoder() {
	return context && (context->hw_device_ctx != NULL);
}

//--------------------------------------------------------------
const Metrics & Decoder::getMetrics() const {
	return metrics;
}

//--------------------------------------------------------------
bool VideoDecoder::open(Reader & reader) {
	
	AVCodec * codec = reader.getVideoCodec();
	AVStream * stream = reader.getVideoStream();

	if (!Decoder::open(codec, stream))
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

	if (!Codec::allocate(codec))
		return false;

	if (hardware.isOpen()) {
		hw_config = hardware.getConfig(codec);
		if (hw_config) {
			if (hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
				context->hw_device_ctx = av_buffer_ref(hardware.getContextRef());
			/*if (hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX)
				codec_context->get_format = get_format;*/
		}
	}

	if (!Decoder::open(codec, stream))
		return false;

	return true;
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
	AVCodec * codec = reader.getAudioCodec();
	AVStream * stream = reader.getAudioStream();
	return Decoder::open(codec, stream);
}

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
bool Decoder::open(AVStream * stream, bool hardwareAccel) {

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

	hw_format = -1;
	sw_format = -1;

	if (hardwareAccel) {

#if defined _WIN32
		AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_DXVA2;
#elif defined __APPLE__
        AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
#else
		AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;
#endif

		int i = 0;
		const AVCodecHWConfig *config = NULL;
		do {
			config = avcodec_get_hw_config(codec, i);
			if (config && config->device_type == hw_type) {
				hw_format = config->pix_fmt;
			}
			i++;
		} while (config != NULL);

		error = av_hwdevice_ctx_create(&hardware_context, hw_type, NULL, NULL, 0);
		if (error < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot create hardware context\n");
		}
		else {
			codec_context->hw_device_ctx = av_buffer_ref(hardware_context);
		}
	}

    if ((error = avcodec_open2(codec_context, codec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open codec context\n");
        return false;
    }

	/*if (hardwareAccel) {
		AVPixelFormat * formats = NULL;
		codec_context->hw_frames_ctx = av_hwframe_ctx_alloc(hardware_context);
		av_hwframe_ctx_init(codec_context->hw_frames_ctx);
		error = av_hwframe_transfer_get_formats(codec_context->hw_frames_ctx, AV_HWFRAME_TRANSFER_DIRECTION_TO, &formats, 0);
		if (error >= 0) {
			while (*formats != AV_PIX_FMT_NONE) {
				sw_format = *formats;
				formats++;
			}
			av_freep(&formats);
		}
	}*/
    
    return true;
}

//--------------------------------------------------------------
void Decoder::close() {
    stop();
    if (codec_context) {
        avcodec_close(codec_context);
        codec_context = NULL;
    }
	if (hardware_context) {
		av_buffer_unref(&hardware_context);
	}
	stream = NULL;
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

			if (frame->format == hw_format) {
				AVFrame * sw_frame = av_frame_alloc();
				error = av_hwframe_transfer_data(sw_frame, frame, 0);
				receiver->receiveFrame(sw_frame, stream->index);
				av_frame_free(&sw_frame);
			}
			else {
				receiver->receiveFrame(frame, stream->index);
			}

            free(frame);
        }
        return true;
    }
    return false;
}

//--------------------------------------------------------------
bool Decoder::flush(FrameReceiver * receiver) {
	return decode(NULL, receiver);
}

//--------------------------------------------------------------
uint64_t Decoder::rescale(AVFrame * frame) {
    return av_rescale_q(frame->pts, stream->time_base, AV_TIME_BASE_Q);
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
    if (running && threadObj) {
        running = false;
        threadObj->join();
		delete threadObj;
		threadObj = NULL;
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
double Decoder::getTimeBase() const {
    return stream ? av_q2d(stream->time_base) : 0.;
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
	if (hw_format != -1)
		return AV_PIX_FMT_NV12;
    return codec_context ? codec_context->pix_fmt : AV_PIX_FMT_NONE;
}

//--------------------------------------------------------------
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

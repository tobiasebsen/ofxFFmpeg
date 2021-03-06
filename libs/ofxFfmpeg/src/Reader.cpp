#include "ofxFFmpeg/Reader.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool Reader::open(std::string filename) {
	std::lock_guard<std::mutex> lock(mutex);

	close();

    if ((error = avformat_open_input(&format_context, filename.c_str(), NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return false;
    }
    
    if ((error = avformat_find_stream_info(format_context, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return false;
    }
    
    return true;
}

//--------------------------------------------------------------
bool Reader::isOpen() const {
	return format_context != NULL;
}

//--------------------------------------------------------------
void Reader::close() {
    stop();
    if (format_context) {
		for (int i = 0; i < format_context->nb_streams; i++) {
			avcodec_close(format_context->streams[i]->codec);
		}
        avformat_close_input(&format_context);
    }
}

//--------------------------------------------------------------
bool Reader::read(AVPacket * packet) {
	std::lock_guard<std::mutex> lock(mutex);

	metrics.begin();
    error = av_read_frame(format_context, packet);
	metrics.end();

	if (error == AVERROR_EOF && loop)
		seek(0);

	return error == 0;
}

//--------------------------------------------------------------
AVPacket * Reader::read() {
    AVPacket * packet = av_packet_alloc();
    av_init_packet(packet);
    if (!read(packet)) {
        av_packet_free(&packet);
        return NULL;
    }
    return packet;
}

//--------------------------------------------------------------
void Reader::free(AVPacket * packet) {
	av_packet_unref(packet);
	av_packet_free(&packet);
}

//--------------------------------------------------------------
void Reader::setLoop(bool loop) {
	this->loop = loop;
}

//--------------------------------------------------------------
bool Reader::getLoop() const {
	return loop;
}

//--------------------------------------------------------------
bool Reader::read(PacketReceiver * receiver) {
	AVPacket * packet = read();
	if (packet) {
		receiver->receive(packet);
		av_packet_unref(packet);
		av_packet_free(&packet);
		return true;
	}
	return false;
}

//--------------------------------------------------------------
void Reader::seek(uint64_t pts) {
	if (isRunning()) {
		seek_pts = pts;
	}
}

//--------------------------------------------------------------
void Reader::seek(uint64_t pts, PacketReceiver * receiver) {

	error = av_seek_frame(format_context, -1, pts, AVSEEK_FLAG_BACKWARD);
	if (error < 0) {
		return;
	}

	AVPacket * packet = read();
	while (packet) {
		int i = packet->stream_index;
		AVStream * stream = format_context->streams[i];
		int64_t ts = av_rescale_q(packet->pts, stream->time_base, { 1, AV_TIME_BASE });
		if (ts < pts) {
			packet->flags |= AV_PKT_FLAG_DISCARD;
		}
		receiver->receive(packet);
		av_packet_unref(packet);
		av_packet_free(&packet);
		
		if (ts >= pts) {
			break;
		}
		
		packet = read();
	}
}

//--------------------------------------------------------------
bool Reader::start(PacketReceiver * receiver) {

	if (!isOpen())
		return false;
    
	stop();
    
    running = true;
	this->receiver = receiver;
	this->receiver->resumePacketReceiver();

	if (thread_obj) delete thread_obj;
    thread_obj = new std::thread(&Reader::readThread, this);

    return true;
}

//--------------------------------------------------------------
void Reader::stop() {
	if (running) {
		running = false;
	}
	if (thread_obj && thread_obj->joinable() && thread_obj->get_id() != std::this_thread::get_id()) {
		receiver->terminatePacketReceiver();
		thread_obj->join();
		delete thread_obj;
		thread_obj = NULL;
	}
}

//--------------------------------------------------------------
void Reader::readThread() {

	while (running) {
        
        AVPacket * packet = read();
        if (packet) {
			receiver->receive(packet);

			av_packet_unref(packet);
			av_packet_free(&packet);
        }
		else if (error == AVERROR_EOF && !loop) {
			receiver->notifyEndPacket();
			running = false;
		}

		if (seek_pts != -1) {
			error = av_seek_frame(format_context, -1, seek_pts, AVSEEK_FLAG_BACKWARD);
			//seek(seek_pts, receiver);
			seek_pts = -1;
		}
    }
}

//--------------------------------------------------------------
unsigned int Reader::getNumStreams() const {
	return format_context->nb_streams;
}

//--------------------------------------------------------------
AVStream * Reader::getStream(int stream_index) const {
	return format_context->streams[stream_index];
}

//--------------------------------------------------------------
AVStream * Reader::getVideoStream() {
	int stream_index = getVideoStreamIndex();
	return stream_index >= 0 ? getStream(stream_index) : NULL;
}

//--------------------------------------------------------------
AVStream * Reader::getAudioStream() {
	int stream_index = getAudioStreamIndex();
	return stream_index >= 0 ? getStream(stream_index) : NULL;
}

//--------------------------------------------------------------
AVCodec * Reader::getVideoCodec() {
	AVCodec * codec = NULL;
	error = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
	return codec;
}

//--------------------------------------------------------------
AVCodec * Reader::getAudioCodec() {
	AVCodec * codec = NULL;
	error = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
	return codec;
}

//--------------------------------------------------------------
int Reader::getStreamIndex(AVPacket * packet) {
	return packet->stream_index;
}

//--------------------------------------------------------------
int Reader::getVideoStreamIndex() {
	return (error = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0));
}

//--------------------------------------------------------------
int Reader::getAudioStreamIndex() {
	return (error = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0));
}

//--------------------------------------------------------------
std::string Reader::getName() {
	return format_context->iformat->name;
}

//--------------------------------------------------------------
std::string Reader::getLongName() {
	return format_context->iformat->long_name;
}

//--------------------------------------------------------------
int64_t Reader::getDuration() const {
	return format_context ? format_context->duration : 0;
}

//--------------------------------------------------------------
double Reader::getDurationSeconds() const {
	return format_context ? format_context->duration * av_q2d({1, AV_TIME_BASE}) : 0;
}

//--------------------------------------------------------------
uint64_t Reader::getBitRate() const {
    return format_context ? format_context->bit_rate : 0;
}

//--------------------------------------------------------------
double Reader::getTimeBase() const {
    return av_q2d({ 1, AV_TIME_BASE });
}

//--------------------------------------------------------------
std::map<std::string, std::string> Reader::getMetadata() {
	std::map<std::string, std::string> metadata;
	AVDictionaryEntry *tag = NULL;

	while ((tag = av_dict_get(format_context->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
		metadata[tag->key] = tag->value;
	}
	return metadata;
}

//--------------------------------------------------------------
const Metrics & Reader::getMetrics() const {
	return metrics;
}

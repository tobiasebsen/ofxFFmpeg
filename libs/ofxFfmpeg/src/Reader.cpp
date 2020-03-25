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
bool ofxFFmpeg::Reader::isOpen() const {
	return format_context != NULL;
}

//--------------------------------------------------------------
void Reader::close() {
    stop();
    if (format_context) {
        avformat_close_input(&format_context);
    }
}

//--------------------------------------------------------------
bool Reader::read(AVPacket * packet) {
	metrics.begin();
    error = av_read_frame(format_context, packet);
	metrics.end();
    return error == 0;
}

//--------------------------------------------------------------
bool Reader::read(PacketReceiver * receiver) {
	AVPacket * packet = read();
	if (packet) {
		receiver->receive(packet);
		av_packet_unref(packet);
		return true;
	}
	return false;
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
void Reader::seek(uint64_t pts) {
    //int video_stream_index = getVideoStreamIndex();
    av_seek_frame(format_context, -1, pts, 0);
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
AVCodec * ofxFFmpeg::Reader::getVideoCodec() {
	AVCodec * codec = NULL;
	error = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
	return codec;
}

//--------------------------------------------------------------
int Reader::getStreamIndex(AVPacket * packet) {
    return packet->stream_index;
}

//--------------------------------------------------------------
int Reader::getVideoStreamIndex() {
	AVCodec * codec;
	return (error = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0));
}

//--------------------------------------------------------------
int Reader::getAudioStreamIndex() {
	AVCodec * codec;
	return (error = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0));
}

//--------------------------------------------------------------
bool Reader::start(PacketReceiver * receiver) {
    if (running)
        return false;
    
    running = true;
	this->receiver = receiver;
	this->receiver->resumePacketReceiver();

	seek(0);
    thread_obj = new std::thread(&Reader::readThread, this);

    return true;
}

//--------------------------------------------------------------
void Reader::stop() {
    if (running && thread_obj) {
        running = false;
		receiver->terminatePacketReceiver();
		if (thread_obj->joinable() && std::this_thread::get_id() != thread_obj->get_id())
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
        }
		else if (error == AVERROR_EOF) {
			receiver->notifyEndPacket();
		}
    }
}

//--------------------------------------------------------------
std::string Reader::getName() {
	return format_context->iformat->name;
}

//--------------------------------------------------------------
std::string ofxFFmpeg::Reader::getLongName() {
	return format_context->iformat->long_name;
}

//--------------------------------------------------------------
float Reader::getDuration() const {
	return format_context ? (format_context->duration * getTimeBase()) : 0;
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
const Metrics & ofxFFmpeg::Reader::getMetrics() const {
	return metrics;
}

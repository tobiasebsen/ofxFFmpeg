#include "Reader.h"

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
void PacketSupplier::free(AVPacket *packet) {
    av_packet_unref(packet);
	av_packet_free(&packet);
}

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
    error = av_read_frame(format_context, packet);
    return error == 0;
}

//--------------------------------------------------------------
AVPacket * Reader::supplyPacket() {
    AVPacket * packet = av_packet_alloc();
    av_init_packet(packet);
    if (!read(packet)) {
        av_packet_free(&packet);
        return NULL;
    }
    return packet;
}

//--------------------------------------------------------------
unsigned int Reader::getNumStreams() {
    return format_context->nb_streams;
}

//--------------------------------------------------------------
AVStream * Reader::getStream(int stream_index) {
    return format_context->streams[stream_index];
}

//--------------------------------------------------------------
int Reader::getStreamIndex(AVPacket * packet) {
    return packet->stream_index;
}

//--------------------------------------------------------------
int Reader::getVideoStreamIndex() {
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            return i;
        }
    }
    return -1;
}

//--------------------------------------------------------------
int Reader::getAudioStreamIndex() {
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            return i;
        }
    }
    return -1;
}

//--------------------------------------------------------------
bool Reader::start(PacketReceiver * receiver) {
    if (running)
        return false;
    
    running = true;
    threadObj = new std::thread(&Reader::readThread, this, receiver);

    return true;
}

//--------------------------------------------------------------
void Reader::stop() {
    if (running && threadObj) {
        running = false;
		if (threadObj->joinable())
	        threadObj->join();
    }
}

//--------------------------------------------------------------
void Reader::readThread(PacketReceiver * receiver) {
    
	while (running) {
        
        AVPacket * packet = supplyPacket();
        if (packet) {
            receiver->receivePacket(packet);
            av_packet_unref(packet);
        }
		else if (error == AVERROR_EOF) {
			receiver->endRead();
		}
    }
}

//--------------------------------------------------------------
void Reader::notify() {
    std::unique_lock<std::mutex> locker(lock);
    condition.notify_all();
}

//--------------------------------------------------------------
float Reader::getDuration() const {
	return format_context ? (format_context->duration * av_q2d({ 1, AV_TIME_BASE })) : 0;
}

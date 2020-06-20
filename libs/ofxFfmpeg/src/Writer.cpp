#include "ofxFFmpeg/Writer.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool Writer::open(const std::string filename) {

	close();

	if (!(format_context = avformat_alloc_context())) {
		av_log(NULL, AV_LOG_ERROR, "Could not allocate output format context\n");
		return false;
	}

	if ((error = avio_open(&io_context, filename.c_str(), AVIO_FLAG_WRITE)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Could not open output file");
		return false;
	}

	format_context->pb = io_context;

	if (!(format_context->oformat = av_guess_format(NULL, filename.c_str(), NULL))) {
		av_log(NULL, AV_LOG_ERROR, "Could not find output file format\n");
		close();
		return false;
	}

	format_context->url = av_strdup(filename.c_str());

	av_dump_format(format_context, 0, filename.c_str(), 1);

	return true;
}

//--------------------------------------------------------------
void Writer::close() {
	if (format_context) {
		avformat_free_context(format_context);
		format_context = NULL;
	}
	if (io_context) {
		avio_close(io_context);
		io_context = NULL;
	}
}

//--------------------------------------------------------------
AVStream * Writer::addStream() {
	AVStream * stream = avformat_new_stream(format_context, NULL);
	if (stream) {
		stream->id = format_context->nb_streams - 1;
	}
	return stream;
}

//--------------------------------------------------------------
bool Writer::begin() {
	if ((error = avformat_write_header(format_context, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file");
		return false;
	}
	return true;
}

//--------------------------------------------------------------
void Writer::end() {
	if (format_context) {
		av_write_trailer(format_context);
	}
}

//--------------------------------------------------------------
void Writer::write(AVPacket * packet) {
	error = av_interleaved_write_frame(format_context, packet);
}

//--------------------------------------------------------------
std::string Writer::getName() {
	return format_context->oformat->name;
}
//--------------------------------------------------------------
std::string Writer::getLongName() {
	return format_context->oformat->long_name;
}

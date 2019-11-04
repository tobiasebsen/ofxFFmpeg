#include "Recorder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool Recorder::init() {
	int error;

	/** Create a new format context for the output container format. */
	if (!(format_context = avformat_alloc_context())) {
		ofLogError() << "Could not allocate output format context";
		return false;
	}
	return true;
}

//--------------------------------------------------------------
bool Recorder::open(const string filename) {

	int error;

	close();
	init();
    
    string filePath = ofFilePath::getAbsolutePath(filename);

	/** Open the output file to write to it. */
	if ((error = avio_open(&io_context, filePath.c_str(), AVIO_FLAG_WRITE)) < 0) {
		ofLogError() << "Could not open output file";
		return false;
	}

	/** Associate the output file (pointer) with the container format context. */
	format_context->pb = io_context;

	/** Guess the desired container format based on the file extension. */
	if (!(format_context->oformat = av_guess_format(NULL, filePath.c_str(), NULL))) {
		ofLogError() << "Could not find output file format";
		close();
		return false;
	}

	av_strlcpy(format_context->filename, filePath.c_str(), sizeof(format_context->filename));

	av_dump_format(format_context, 0, filePath.c_str(), 1);
}

//--------------------------------------------------------------
bool Recorder::setCodec(int codecId) {
    
    /** Find the encoder to be used by its name. */
    if (!(video_codec = avcodec_find_encoder((enum AVCodecID)codecId))) {
        ofLogError() << "Could not find the encoder";
        close();
        return false;
    }
    
    video_context = avcodec_alloc_context3(video_codec);
    if (!video_context) {
        ofLogError() << "Could not allocate an encoding context";
        close();
        return false;
    }

    if (video_codec->pix_fmts)
        video_context->pix_fmt = video_codec->pix_fmts[0];
    
    return true;
}
//--------------------------------------------------------------
bool Recorder::setCodec(string codecName) {
    
    /** Find the encoder to be used by its name. */
    if (!(video_codec = avcodec_find_encoder_by_name(codecName.c_str()))) {
        ofLogError() << "Could not find the encoder";
        close();
        return false;
    }
    
    video_context = avcodec_alloc_context3(video_codec);
    if (!video_context) {
        ofLogError() << "Could not allocate an encoding context";
        close();
        return false;
    }
    
    if (video_codec->pix_fmts)
        video_context->pix_fmt = video_codec->pix_fmts[0];
    
    return true;
}
//--------------------------------------------------------------
void Recorder::setWidth(int width) {
    video_context->width = width;
}
//--------------------------------------------------------------
void Recorder::setHeight(int height) {
    video_context->height = height;
}
//--------------------------------------------------------------
void Recorder::setFrameRate(float frameRate) {
    video_context->framerate = av_d2q(frameRate, 1000);
}
//--------------------------------------------------------------
void Recorder::setBitRate(int bitRate) {
    video_context->bit_rate = bitRate;
}
//--------------------------------------------------------------
void Recorder::setLevel(int level) {
    video_context->level = level;
}
//--------------------------------------------------------------
void Recorder::setKeyFrame(int key_frame) {
    keyFrameRate = key_frame;
}
//--------------------------------------------------------------
bool Recorder::start() {
    
    video_context->time_base.num = video_context->framerate.den;
    video_context->time_base.den = video_context->framerate.num;
    
    //if (output_format_context->flags & AVFMT_GLOBALHEADER)
    video_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    
    /* Third parameter can be used to pass settings to encoder */
    if ((avcodec_open2(video_context, video_codec, NULL)) < 0) {
        ofLogError() << "Cannot open video encoder for stream";
        close();
        return false;
    }

    /** Create a new video stream in the output file container. */
    if (!(video_stream = avformat_new_stream(format_context, NULL))) {
        ofLog() << "Could not create new stream\n";
        close();
        return false;
    }
    video_stream->id = format_context->nb_streams - 1;
    
    if (avcodec_parameters_from_context(video_stream->codecpar, video_context) < 0) {
        ofLogError() << "Failed to copy encoder parameters to output stream";
        close();
        return false;
    }

    /* init muxer, write output file header */
    if ((avformat_write_header(format_context, NULL)) < 0) {
        ofLogError() << "Error occurred when opening output file";
        return false;
    }
    
    if (!(frame = av_frame_alloc())) {
        ofLogError() << "Could not allocate input frame";
        return false;
    }
    
    frame->width = video_context->width;
    frame->height = video_context->height;
    frame->format = video_context->pix_fmt;
    frame->pts = pts = 0;
    
    n_frame = 0;
    
    /**
     * Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified.
     */
    if ((av_frame_get_buffer(frame, 32)) < 0) {
        ofLogError() << "Could allocate output frame samples";
        close();
        return false;
    }
    
    sws_context = sws_getContext(frame->width, frame->height, AV_PIX_FMT_RGB24, frame->width, frame->height, video_context->pix_fmt, 0, 0, 0, 0);
    
    return true;
}
//--------------------------------------------------------------
void ofxFFmpeg::Recorder::write(AVFrame * f) {

	int error;
	AVPacket packet;

    frame->key_frame = (n_frame % keyFrameRate) == 0 ? 1 : 0;
    n_frame++;

	error = avcodec_send_frame(video_context, f);
	if (error < 0) {
		ofLogError() << "Cound not send frame";
		return;
	}

	av_init_packet(&packet);
	packet.stream_index = video_stream->index;
	packet.flags |= AV_PKT_FLAG_KEY;
	packet.pos = -1;
    
	while (error >= 0) {
		error = avcodec_receive_packet(video_context, &packet);
		if (error >= 0) {
			int ret = av_write_frame(format_context, &packet);

			av_packet_unref(&packet);
		}
	}
}
//--------------------------------------------------------------
void Recorder::write(const ofPixels & pixels) {
    
    frame->pts = (1.0 / 30) * 90000 * pts;
    pts++;
    
    const int in_linesize[1] = { 3 * video_context->width };
    const uint8_t * rgb = pixels.getData();
    sws_scale(sws_context, (const uint8_t * const *)&rgb, in_linesize, 0, video_context->height, frame->data, frame->linesize);
    
    write(frame);
}
//--------------------------------------------------------------
void Recorder::flush() {
	write(NULL);
}
//--------------------------------------------------------------
void Recorder::close() {
	if (format_context) {
		av_write_trailer(format_context);
	}
	if (frame) {
		av_frame_free(&frame);
		frame = NULL;
	}
	if (format_context) {
		avformat_free_context(format_context);
		format_context = NULL;
	}
	if (io_context) {
		avio_close(io_context);
		io_context = NULL;
	}
}

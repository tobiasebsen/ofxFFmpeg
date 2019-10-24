#include "Recorder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
}

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool Recorder::open(const string filename, int width, int height, int frameRate) {

	int error;

	/** Open the output file to write to it. */
	if ((error = avio_open(&output_io_context, filename.c_str(), AVIO_FLAG_WRITE)) < 0) {
		ofLogError() << "Could not open output file";
		return false;
	}

	/** Create a new format context for the output container format. */
	if (!(output_format_context = avformat_alloc_context())) {
		ofLogError() << "Could not allocate output format context";
		return false;
	}

	/** Associate the output file (pointer) with the container format context. */
	output_format_context->pb = output_io_context;

	/** Guess the desired container format based on the file extension. */
	if (!(output_format_context->oformat = av_guess_format(NULL, filename.c_str(), NULL))) {
		ofLogError() << "Could not find output file format";
		close();
		return false;
	}

	av_strlcpy(output_format_context->filename, filename.c_str(), sizeof(output_format_context->filename));

	/** Find the encoder to be used by its name. */
	if (!(output_codec = avcodec_find_encoder(AV_CODEC_ID_H264))) {
		ofLogError() << "Could not find the encoder";
		close();
		return false;
	}

	/** Create a new video stream in the output file container. */
	if (!(stream = avformat_new_stream(output_format_context, NULL))) {
		ofLog() << "Could not create new stream\n";
		close();
		return false;
	}

	avctx = avcodec_alloc_context3(output_codec);
	if (!avctx) {
		ofLogError() << "Could not allocate an encoding context";
		close();
		return false;
	}

	avctx->width = width;
	avctx->height = height;
	if (output_codec->pix_fmts)
		avctx->pix_fmt = output_codec->pix_fmts[0];

	avctx->time_base.num = 1;
	avctx->time_base.den = frameRate;
	avctx->framerate.num = frameRate;
	avctx->framerate.den = 1;

	/* Third parameter can be used to pass settings to encoder */
	error = avcodec_open2(avctx, output_codec, NULL);
	if (error < 0) {
		ofLogError() << "Cannot open video encoder for stream";
		close();
		return false;
	}
	error = avcodec_parameters_from_context(stream->codecpar, avctx);
	if (error < 0) {
		ofLogError() << "Failed to copy encoder parameters to output stream";
		close();
		return false;
	}

	/* init muxer, write output file header */
	error = avformat_write_header(output_format_context, NULL);
	if (error < 0) {
		ofLogError() << "Error occurred when opening output file";
		return false;
	}

	if (!(frame = av_frame_alloc())) {
		ofLogError() << "Could not allocate input frame";
		return false;
	}

	frame->width = width;
	frame->height = height;
	frame->format = avctx->pix_fmt;
	frame->pts = pts = 0;


	/**
	 * Allocate the samples of the created frame. This call will make
	 * sure that the audio frame can hold as many samples as specified.
	 */
	if ((error = av_frame_get_buffer(frame, 32)) < 0) {
		ofLogError() << "Could allocate output frame samples";
		close();
		return false;
	}

	return true;
}

void Recorder::write(const ofPixels & pixels) {
	int error;
	AVPacket packet;
    
    frame->pts = ++pts;
    memcpy(frame->data[0], pixels.getChannel(0).getData(), frame->linesize[0] * frame->height);
    memcpy(frame->data[1], pixels.getChannel(1).getData(), frame->linesize[1] * frame->height);
    memcpy(frame->data[2], pixels.getChannel(2).getData(), frame->linesize[2] * frame->height);

    error = av_frame_make_writable(frame);
    if (error < 0) {
        ofLogError() << "Cound not make frame writable";
        return;
    }

    error = avcodec_send_frame(avctx, frame);
    if (error < 0) {
        ofLogError() << "Cound not send frame";
        return;
    }

    av_init_packet(&packet);
    packet.pts = pts;
    packet.stream_index = stream->index;

    while (error >= 0) {
        error = avcodec_receive_packet(avctx, &packet);
        if (error >= 0) {
            error = av_write_frame(output_format_context, &packet);

            av_packet_unref(&packet);
        }
    }
}

void Recorder::flush() {
    int error;
    AVPacket packet;
    
    av_init_packet(&packet);
    packet.pts = 0;
    packet.stream_index = stream->index;
    
    error = avcodec_send_frame(avctx, NULL);
    
    while (error >= 0) {
        error = avcodec_receive_packet(avctx, &packet);
        if (error >= 0) {
            av_write_frame(output_format_context, &packet);
            av_packet_unref(&packet);
        }
    }
}

void Recorder::close() {
	if (output_format_context) {
		av_write_trailer(output_format_context);
	}
	/*if (frame) {
		av_frame_free(&frame);
		frame = NULL;
	}
	if (stream) {
		av_free(stream);
		stream = NULL;
	}*/
	if (output_format_context) {
		avformat_free_context(output_format_context);
		output_format_context = NULL;
	}
	if (output_io_context) {
		avio_close(output_io_context);
		output_io_context = NULL;
	}
}

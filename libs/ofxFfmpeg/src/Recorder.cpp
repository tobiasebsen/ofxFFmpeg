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
	if (!(output_format_context = avformat_alloc_context())) {
		ofLogError() << "Could not allocate output format context";
		return false;
	}
	return true;
}

//--------------------------------------------------------------
bool Recorder::openCodec(int width, int height, int frameRate) {

	int error;

	/** Find the encoder to be used by its name. */
	if (!(output_codec = avcodec_find_encoder(AV_CODEC_ID_H264))) {
		ofLogError() << "Could not find the encoder";
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

	avctx->color_range = AVCOL_RANGE_MPEG;
	avctx->time_base.num = 1;
	avctx->time_base.den = frameRate;
	//avctx->framerate.num = frameRate;
	//avctx->framerate.den = 1;
	avctx->bit_rate = 800000;
	avctx->gop_size = 12;
	//avctx->pix_fmt = AV_PIX_FMT_YUV420P;
	//if (output_format_context->flags & AVFMT_GLOBALHEADER)
	avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	avctx->profile = FF_PROFILE_H264_HIGH;
	avctx->level = 41;

	/* Third parameter can be used to pass settings to encoder */
	error = avcodec_open2(avctx, output_codec, NULL);
	if (error < 0) {
		ofLogError() << "Cannot open video encoder for stream";
		close();
		return false;
	}
}

//--------------------------------------------------------------
bool Recorder::open(const string filename, int width, int height, int frameRate) {

	int error;

	init();

	/** Open the output file to write to it. */
	if ((error = avio_open(&output_io_context, filename.c_str(), AVIO_FLAG_WRITE)) < 0) {
		ofLogError() << "Could not open output file";
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

	openCodec(width, height, frameRate);

	/** Create a new video stream in the output file container. */
	if (!(stream = avformat_new_stream(output_format_context, NULL))) {
		ofLog() << "Could not create new stream\n";
		close();
		return false;
	}
	stream->id = output_format_context->nb_streams - 1;

	error = avcodec_parameters_from_context(stream->codecpar, avctx);
	if (error < 0) {
		ofLogError() << "Failed to copy encoder parameters to output stream";
		close();
		return false;
	}

	av_dump_format(output_format_context, 0, filename.c_str(), 1);

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

	sws_ctx = sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);

	return true;
}

void Recorder::write(const ofPixels & pixels) {
    
	frame->pts = (1.0 / 30)* 90000 * pts;
	pts++;

	const int in_linesize[1] = { 3 * avctx->width };
	const uint8_t * rgb = pixels.getPixels();
	sws_scale(sws_ctx, (const uint8_t * const *)&rgb, in_linesize, 0, avctx->height, frame->data, frame->linesize);

	write(frame);
}

void ofxFFmpeg::Recorder::write(AVFrame * f) {

	int error;
	AVPacket packet;


	error = avcodec_send_frame(avctx, f);
	if (error < 0) {
		ofLogError() << "Cound not send frame";
		return;
	}

	av_init_packet(&packet);
	packet.stream_index = stream->index;
	packet.flags |= AV_PKT_FLAG_KEY;
	packet.pos = -1;

	while (error >= 0) {
		error = avcodec_receive_packet(avctx, &packet);
		if (error >= 0) {
			int ret = av_write_frame(output_format_context, &packet);

			av_packet_unref(&packet);
		}
	}
}

void Recorder::flush() {
	write(NULL);
}

void Recorder::close() {
	if (output_format_context) {
		av_write_trailer(output_format_context);
	}
	if (frame) {
		av_frame_free(&frame);
		frame = NULL;
	}
	if (output_format_context) {
		avformat_free_context(output_format_context);
		output_format_context = NULL;
	}
	if (output_io_context) {
		avio_close(output_io_context);
		output_io_context = NULL;
	}
}

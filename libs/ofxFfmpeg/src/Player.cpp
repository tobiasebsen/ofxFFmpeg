#include "Player.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;


ofxFFmpeg::Player::Player() {
}

ofxFFmpeg::Player::~Player() {
	close();
}

bool Player::load(string filename) {
    
    int ret;
	string filePath = ofFilePath::getAbsolutePath(filename);

	close();
    
    if ((ret = avformat_open_input(&format_context, filePath.c_str(), NULL, NULL)) < 0) {
        ofLogError() << "Cannot open input file";
        close();
        return false;
    }
    
    //av_dump_format(format_context, 0, filename.c_str(), 1);

    if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) {
        ofLogError() << "Cannot find stream information";
        close();
        return false;
    }
    
    for (int i = 0; i < format_context->nb_streams; i++) {
        AVStream *stream = format_context->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
            video_stream = stream;
            video_codec = avcodec_find_decoder(stream->codecpar->codec_id);
        }
    }
    
    if (video_codec == NULL) {
        ofLogError() << "Video codec not found";
        close();
        return false;
    }
    
    if ((video_context = avcodec_alloc_context3(video_codec)) == NULL) {
        ofLogError() << "Cannot allocate codec context";
        close();
        return false;
    }
    
    avcodec_parameters_to_context(video_context, video_stream->codecpar);
    
    if ((ret = avcodec_open2(video_context, video_codec, NULL)) < 0) {
        ofLogError() << "Cannot open codec context";
        close();
        return false;
    }
    
	if ((frame = av_frame_alloc()) == NULL) {
		ofLogError() << "Cannot allocate frame";
		close();
		return false;
	}

	frame->width = video_context->width;
	frame->height = video_context->height;
	frame->format = video_context->pix_fmt;
	ret = av_frame_get_buffer(frame, 32);
	if (ret < 0) {
		ofLogError() << "Could not get frame buffer";
		close();
		return false;
	}
	
	int width = video_context->width;
    int height = video_context->height;
    
    sws_context = sws_getContext(width, height, video_context->pix_fmt, width, height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);

	startTime = av_gettime_relative();

    return true;
}

void Player::readFrame() {
    
    int ret;
    AVPacket packet;

    ret = av_read_frame(format_context, &packet);
	if (ret == AVERROR_EOF) {
        if (loopState == OF_LOOP_NORMAL) {
            av_seek_frame(format_context, -1, 0, 0);
            ret = av_read_frame(format_context, &packet);
        }
        else
            return;
	}
	if (ret < 0) {
		return;
	}
    
    ret = avcodec_send_packet(video_context, &packet);
    av_packet_unref(&packet);
    if (ret < 0 && ret != AVERROR_EOF) {
        return;
    }

    ret = avcodec_receive_frame(video_context, frame);
    if (ret >= 0) {

		//ofLog() << "Read: " << (frame->pts * av_q2d(video_stream->time_base));

        pixels.allocate(frame->width, frame->height, 3);

		const uint8_t * rgb = pixels.getData();
        const int out_linesize[1] = { 3 * frame->width };
        sws_scale(sws_context, frame->data, frame->linesize, 0, (int)frame->height, (uint8_t * const *)&rgb, out_linesize);

        texture.loadData(pixels);
		frameNew = true;
    }
}

void Player::draw(float x, float y, float w, float h) const {
	if (texture.isAllocated())
		texture.draw(x, y, w, h);
}

void Player::draw(float x, float y) const {
    if (texture.isAllocated())
        texture.draw(x, y);
}

float Player::getWidth() const {
    return video_context ? video_context->width : 0;
}

float Player::getHeight() const {
    return video_context ? video_context->height : 0;
}

bool Player::setPixelFormat(ofPixelFormat pixelFormat) {
	return false;
}

ofPixelFormat Player::getPixelFormat() const {
	return OF_PIXELS_RGB;
}

float Player::getPosition() const {
	return (float)getCurrentFrame() / (float)getTotalNumFrames();
}

int Player::getCurrentFrame() const {
	if (frame && video_stream) {
		uint64_t n = frame->pts * av_q2d(video_stream->time_base) * av_q2d(video_stream->r_frame_rate);
		return n;
	}
	return -1;
}

float Player::getDuration() const {
	return format_context ? (format_context->duration * av_q2d({ 1,AV_TIME_BASE })) : 0;
}

int Player::getTotalNumFrames() const {
    return video_stream ? video_stream->nb_frames : 0;
}

void Player::setFrame(int f) {
	uint64_t ts = ((uint64_t)f) / (av_q2d(video_stream->time_base) * av_q2d(video_stream->r_frame_rate));
	//ts += video_stream->start_time;
	//ts *= 0.3;
	ofLog() << "Seek: " << f << " > " << ts;
	av_seek_frame(format_context, video_stream_index, ts, AVSEEK_FLAG_BACKWARD);
	//avformat_seek_file(format_context, video_stream_index, INT64_MIN, ts, INT64_MAX, 0);
	//avcodec_flush_buffers(video_context);
	readFrame();
}

void Player::setPosition(float pct) {
	setFrame(pct * getTotalNumFrames());
}

void Player::close() {
	if (frame) {
		av_frame_free(&frame);
		frame = NULL;
	}
    if (video_context) {
        avcodec_close(video_context);
        video_context = NULL;
    }
    if (format_context) {
        avformat_close_input(&format_context);
        format_context = NULL;
    }
}

bool Player::isLoaded() const {
	return format_context && video_context && video_stream;
}

bool Player::isInitialized() const {
	return format_context;
}

void Player::update() {

	frameNew = false;

	frameTime = av_gettime_relative();

	if (frame == NULL)
		return;

	uint64_t ptsNow = frameTime - startTime;
	uint64_t ptsFrame = av_rescale_q(frame->pts, video_stream->time_base, { 1, AV_TIME_BASE });

	//if (frame->pts == AV_NOPTS_VALUE || ptsNow > ptsFrame)
		readFrame();
}

bool Player::isFrameNew() const {
	return frameNew;
}

void Player::play() {
}

void Player::stop() {
}

bool Player::isPaused() const {
	return false;
}

bool Player::isPlaying() const {
	return true;
}

void Player::setLoopState(ofLoopType state) {
    loopState = state;
}

ofPixels & Player::getPixels() {
	return pixels;
}

const ofPixels & Player::getPixels() const {
	return pixels;
}

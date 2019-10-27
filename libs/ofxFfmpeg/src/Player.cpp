#include "Player.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;


bool Player::load(string filename) {
    
    int ret;
    
    if ((ret = avformat_open_input(&format_context, filename.c_str(), NULL, NULL)) < 0) {
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
    
    int width = video_context->width;
    int height = video_context->height;
    
    sws_context = sws_getContext(width, height, video_context->pix_fmt, width, height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);

    return true;
}

void Player::readFrame() {
    
    int ret;
    AVPacket packet;

    if ((ret = av_read_frame(format_context, &packet)) < 0)
        return;
    
    ret = avcodec_send_packet(video_context, &packet);
    av_packet_unref(&packet);
    if (ret < 0 && ret != AVERROR_EOF) {
        return;
    }

    AVFrame *frame = av_frame_alloc();
    
    frame->width = video_context->width;
    frame->height = video_context->height;
    frame->format = video_context->pix_fmt;
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        ofLogError() << "Could not get frame buffer";
        av_frame_free(&frame);
        return;
    }
    
    ret = avcodec_receive_frame(video_context, frame);
    if (ret >= 0) {
        pixels.allocate(frame->width, frame->height, 3);
        const uint8_t * rgb = pixels.getData();
        const int out_linesize[1] = { 3 * frame->width };
        sws_scale(sws_context, frame->data, frame->linesize, 0, (int)frame->height, (uint8_t * const *)&rgb, out_linesize);
        texture.loadData(pixels);
    }
    
    av_frame_free(&frame);
}

void Player::draw(float x, float y) const {
    if (texture.isAllocated())
        texture.draw(0, 0);
}

float Player::getWidth() const {
    return video_context ? video_context->width : 0;
}

float Player::getHeight() const {
    return video_context ? video_context->height : 0;
}

float Player::getDuration() const {
    return video_stream ? (video_stream->duration * video_stream->time_base.num) / video_stream->time_base.den : 0;
}

int Player::getTotalNumFrames() const {
    return video_stream ? video_stream->nb_frames : 0;
}

void Player::close() {
    if (video_context) {
        avcodec_close(video_context);
        video_context = NULL;
    }
    if (format_context) {
        avformat_close_input(&format_context);
        format_context = NULL;
    }
}

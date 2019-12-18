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


void Reader::readThread(const char * filename, PacketQueue & videoPackets) {

	int ret;
	AVStream * video_stream = NULL;
	int video_stream_index = -1;
	AVCodec * video_codec = NULL;
	AVCodecContext * video_context = NULL;
	int audio_stream_index = -1;
    
    lastVideoPts == AV_NOPTS_VALUE;

	if ((ret = avformat_open_input(&format_context, filename, NULL, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
		goto cleanup;
	}

	if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		goto cleanup;
	}

	for (int i = 0; i < format_context->nb_streams; i++) {
		AVStream *stream = format_context->streams[i];

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			video_stream = stream;
			video_codec = avcodec_find_decoder(stream->codecpar->codec_id);
		}
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_index = i;
		}
	}

	if (video_codec == NULL) {
		av_log(NULL, AV_LOG_ERROR, "Video codec not found");
		goto cleanup;
	}

	if ((video_context = avcodec_alloc_context3(video_codec)) == NULL) {
		av_log(NULL, AV_LOG_ERROR, "Cannot allocate codec context\n");
		goto cleanup;
	}

	avcodec_parameters_to_context(video_context, video_stream->codecpar);

	if ((ret = avcodec_open2(video_context, video_codec, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open codec context\n");
		goto cleanup;
	}
    
    video = std::shared_ptr<VideoThread>(new VideoThread(video_context, videoPackets));

	AVPacket packet;
	av_init_packet(&packet);
    
	while (running) {

        if (actions.size() > 0) {
            Action & action = actions.front();
            
            switch (action.type) {

                case Action::READ: {
                    
                    ret = av_read_frame(format_context, &packet);
                    if (ret >= 0) {
                        
                        if (packet.stream_index == video_stream_index) {
                            videoPackets.push(&packet);
                            av_log(NULL, AV_LOG_INFO, "%d\n", (int)videoPackets.size());
                            lastVideoPts = av_rescale_q(packet.pts + packet.duration - 1, video_stream->time_base, { 1, AV_TIME_BASE });
                        }
                        if (packet.stream_index == audio_stream_index) {
                        }
                    }
                    av_packet_unref(&packet);
                }
                break;
                    
                case Action::SEEK:
                    break;

                default:
                    break;
            }
        }

		// Lock scope
		{
			std::unique_lock<std::mutex> locker(lock);

			condition.wait(locker);
		}
	}

cleanup:
	running = false;
	if (video_context) {
		avcodec_close(video_context);
		video_context = NULL;
	}
	avformat_close_input(&format_context);
}

void Reader::read(uint64_t pts) {
	std::unique_lock<std::mutex> locker(lock);
    if (pts > lastVideoPts || lastVideoPts == AV_NOPTS_VALUE) {
        actions.emplace(Action::READ, pts);
        condition.notify_one();
    }
}

int Reader::getWidth() {
	return video_stream ? video_stream->codecpar->width : 0;
}

int Reader::getHeight() {
	return video_stream ? video_stream->codecpar->height : 0;
}

int Reader::getTotalNumFrames() const {
	return video_stream ? video_stream->nb_frames : 0;
}

float Reader::getDuration() const {
	return format_context ? (format_context->duration * av_q2d({ 1,AV_TIME_BASE })) : 0;
}

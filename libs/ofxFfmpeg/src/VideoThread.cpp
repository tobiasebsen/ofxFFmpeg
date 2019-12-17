#include "VideoThread.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}

using namespace ofxFFmpeg;

void VideoThread::videoThread(AVCodecContext * video_context) {

	int ret;
    
    int width = video_context->width;
    int height = video_context->height;
    sws_context = sws_getContext(width, height, video_context->pix_fmt, width, height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);

	while (running) {

		AVPacket * packet = videoPackets.pop();
		ret = avcodec_send_packet(video_context, packet);
        av_packet_unref(packet);
		if (ret >= 0) {

            while (ret >= 0) {
                AVFrame * frame = av_frame_alloc();
                ret = avcodec_receive_frame(video_context, frame);
                if (ret < 0) {
                    av_frame_free(&frame);
                    break;
                }
                
                const uint8_t * rgb = NULL;
                const int out_linesize[1] = { 3 * frame->width };
                sws_scale(sws_context, frame->data, frame->linesize, 0, (int)frame->height, (uint8_t * const *)&rgb, out_linesize);

                //ofLog() << frame->pts;
                //frameQueue.emplace(frame->pts, frame);
                //frameQueueDuration += frame->pkt_duration;
            }
		}
	}
	running = false;
}

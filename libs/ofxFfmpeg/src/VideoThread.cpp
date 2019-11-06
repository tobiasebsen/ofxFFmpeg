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

void VideoThread::videoThread(PacketQueue & videoPackets) {

	int ret;

	while (running) {

		videoPackets.wait();

		//AVPacket * packet = videoPackets.pop(0);
		//ret = avcodec_send_packet(video_context, &packet);
		if (ret >= 0) {

		}
	}
	running = false;
}
#include "HapDecoder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

extern "C" {
#include <hap.h>
}
#if defined _WIN32
#include <ppl.h>
#else
#include <dispatch/dispatch.h>
#endif

//--------------------------------------------------------------
static int roundUpToMultipleOf4(int n) {
	return ((0 != (n & 3)) ? (n + 3) & ~3 : n);
}

//--------------------------------------------------------------
static void doDecode(HapDecodeWorkFunction function, void *p, unsigned int count, void *info) {
#if defined(__APPLE__)
	dispatch_apply(count, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^ (size_t i) {
		function(p, i);
	});
#elif defined(_WIN32)
	concurrency::parallel_for(0U, count, [&](unsigned int i) {
		function(p, i);
	});
#else
	struct Work w = { p, function };
	dispatch_apply_f(count, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), &w, decodeWrapper);
#endif
}

//--------------------------------------------------------------
bool HapDecoder::open(ofxFFmpeg::Reader & reader) {

	AVCodec * codec = reader.getVideoCodec();
	AVStream * stream = reader.getVideoStream();

	if (stream->codecpar->codec_id != AV_CODEC_ID_HAP) {
		av_log(NULL, AV_LOG_ERROR, "Video stream is not HAP\n");
		return false;
	}

	switch (stream->codecpar->codec_tag) {
	case MKTAG('H', 'a', 'p', '1'):
	case MKTAG('H', 'a', 'p', '5'):
	case MKTAG('H', 'a', 'p', 'Y'):
	case MKTAG('H', 'a', 'p', 'M'):
	case MKTAG('H', 'a', 'p', 'A'):
	case MKTAG('H', 'a', 'p', '7'):
		break;
	default:
		av_log(NULL, AV_LOG_ERROR, "Unsupported HAP format\n");
		return false;
	}

	if (!Decoder::open(codec, stream))
		return false;

	return true;
}

//--------------------------------------------------------------
bool HapDecoder::decode(AVPacket * packet, ofxFFmpeg::FrameReceiver * receiver) {

	ofxFFmpeg::Packet p(packet);

	unsigned int textureCount;
	unsigned int hapResult = HapGetFrameTextureCount(p.getData(), p.getSize(), &textureCount);
	if (hapResult != HapResult_No_Error)
		return false;

	for (int index = 0; index < textureCount; index ++) {

		unsigned int textureFormat;
		hapResult = HapGetFrameTextureFormat(p.getData(), p.getSize(), index, &textureFormat);
		if (hapResult != HapResult_No_Error)
			return false;

		size_t length = getCodedWidth() * getCodedHeight();
		if (textureFormat == HapTextureFormat_RGB_DXT1) {
			length /= 2;
		}

		ofxFFmpeg::VideoFrame frame;
		frame.allocate(length);
		frame.setTimeStamp(p.getTimeStamp());
		frame.setDuration(p.getDuration());

		metrics.begin();

		unsigned long bytesUsed;
		hapResult = HapDecode(
			p.getData(),
			p.getSize(),
			index,
			doDecode,
			NULL,
			frame.getData(),
			frame.getSize(),
			&bytesUsed,
			&textureFormat);
		if (hapResult != HapResult_No_Error)
			return false;

		metrics.end();

		receiver->receive(frame, getStreamIndex());
	}

	return true;
}

//--------------------------------------------------------------
const ofxFFmpeg::Metrics & HapDecoder::getMetrics() const {
	return metrics;
}

#include "ofxFfmpeg.h"

#include <stdlib.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include <libavutil/opt.h>
}

using namespace ofxFFmpeg;
using namespace std;

list<int> ofxFFmpeg::getOutputFormats(bool videoFormats, bool audioFormats) {
    list<int> ids;
    
    const AVOutputFormat* format;
    void * i = NULL;
    while ((format = av_muxer_iterate(&i))) {
        if ((videoFormats && format->video_codec != AV_CODEC_ID_NONE) || (audioFormats && format->audio_codec != AV_CODEC_ID_NONE))
            ids.push_back(format->video_codec);
    }
    ids.sort();
    ids.unique();
    return ids;
}

int ofxFFmpeg::getVideoEncoder(string name) {
    const AVOutputFormat* format;
    void * i = NULL;
    while ((format = av_muxer_iterate(&i))) {
        if (name == format->name)
            return format->video_codec;
    }
    return AV_CODEC_ID_NONE;
}

string ofxFFmpeg::getCodecName(int codecId) {
    return avcodec_get_name((AVCodecID)codecId);
}

string ofxFFmpeg::getCodecLongName(int codecId) {
    return avcodec_descriptor_get((AVCodecID)codecId)->long_name;
}

#pragma once

#include "ofxFFmpeg.h"

class HapDecoder : public ofxFFmpeg::Decoder, public ofxFFmpeg::VideoCodec {
public:

	bool open(ofxFFmpeg::Reader & reader);

	bool decode(AVPacket * packet, ofxFFmpeg::FrameReceiver * receiver);

	const ofxFFmpeg::Metrics & getMetrics() const;

protected:
	ofxFFmpeg::Metrics metrics;
};
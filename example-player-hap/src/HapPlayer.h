#pragma once

#include "ofMain.h"
#include "ofxFFmpeg.h"
#include "HapDecoder.h"

class HapPlayer : ofxFFmpeg::PacketReceiver, ofxFFmpeg::FrameReceiver {
public:
	~HapPlayer();

	bool load(string filename);
	void close();

	void play();
	void stop();
		
	void update();
	void draw(float x, float y) const;

	void audioOut(ofSoundBuffer & buffer);

protected:
	void receive(AVPacket * packet);
	void terminatePacketReceiver();
	void resumePacketReceiver();

	void receive(AVFrame * frame, int stream_index);

	void updateFrame(AVFrame * frame);

	int64_t realTime;
	int64_t video_pts_last;

	ofxFFmpeg::Reader reader;
	HapDecoder video;
	ofxFFmpeg::FrameQueue videoCache;
	ofTexture texture;

	ofxFFmpeg::AudioDecoder audio;
	ofxFFmpeg::AudioResampler resampler;
	ofxFFmpeg::AudioBuffer<float> audioBuffer;
	ofSoundStream audioStream;
};
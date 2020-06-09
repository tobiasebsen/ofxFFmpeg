#pragma once

#include "ofMain.h"
#include "ofxFFmpeg.h"

class ofxFFmpegRecorder : public ofxFFmpeg::PacketReceiver {
public:
	bool open(const string filename);
	void close();

	bool setVideoCodec(int codecId);
	bool setVideoCodec(string codecName);

	bool setAudioCodec(int codecId);
	bool setAudioCodec(string codecName);

	ofxFFmpeg::VideoEncoder & getVideoEncoder();
	ofxFFmpeg::AudioEncoder & getAudioEncoder();

    bool start();
    void stop();

    void write(const ofPixels & pixels, int frameNumber = -1);
	void write(const ofPixels & pixels, float timeSeconds);

	void audioIn(ofSoundBuffer & buffer);

	int getError();
	string getErrorString();

protected:

	bool receive(AVPacket * packet);

	int error = 0;

	ofxFFmpeg::Writer writer;
	ofxFFmpeg::VideoEncoder video;
	ofxFFmpeg::VideoScaler scaler;
	ofxFFmpeg::AudioEncoder audio;

	AVFrame *frame = NULL;
    uint64_t frame_count = 0;

	ofSoundStream audioStream;
};

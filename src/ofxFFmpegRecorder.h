#pragma once

#include "ofMain.h"
#include "ofxFFmpeg.h"

class ofxFFmpegRecorder : public ofxFFmpeg::PacketReceiver {
public:
	bool open(const string filename);
	void close();

	bool setVideoCodec(int codecId);
	bool setVideoCodec(string codecName);

	ofxFFmpeg::VideoEncoder & getVideoEncoder();

    bool start();
    void stop();

    void write(const ofPixels & pixels);

	int getError();
	string getErrorString();

protected:

	void receivePacket(AVPacket * packet);

	int error = 0;

	ofxFFmpeg::Writer writer;
	ofxFFmpeg::VideoEncoder video;
	ofxFFmpeg::VideoScaler scaler;

	AVFrame *frame = NULL;
    uint64_t n_frame;
};

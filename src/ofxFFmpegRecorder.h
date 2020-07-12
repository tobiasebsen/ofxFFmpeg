#pragma once

#include "ofMain.h"
#include "ofxFFmpeg.h"

class ofxFFmpegRecorder : public ofxFFmpeg::PacketReceiver {
public:
	ofxFFmpegRecorder();
	~ofxFFmpegRecorder();

	bool open(const string filename);
	void close();

	bool setVideoCodec(int codecId);
	bool setVideoCodec(string codecName);

	bool setAudioCodec(int codecId);
	bool setAudioCodec(string codecName);

	void setAudioInputSettings(const ofSoundStreamSettings & settings);

	ofxFFmpeg::VideoEncoder & getVideoEncoder();
	ofxFFmpeg::AudioEncoder & getAudioEncoder();
	ofxFFmpeg::Writer & getWriter();

    bool start();
    void stop();

    void write(const ofPixels & pixels);

	void writeAudio();
	void writeAudio(int nb_samples);
	void writeAudioFinal();

	void audioIn(ofSoundBuffer & buffer);

	int getError();
	string getErrorString();

protected:

	void receive(AVPacket * packet);

	int error = 0;

	ofxFFmpeg::Writer writer;
	ofxFFmpeg::VideoEncoder video;
	ofxFFmpeg::VideoScaler scaler;
	ofxFFmpeg::AudioEncoder audio;
	ofxFFmpeg::AudioResampler resampler;
	ofxFFmpeg::AudioBuffer<float> audioBuffer;

	AVFrame * vframe = NULL;
    uint64_t frame_count = 0;
	AVFrame * aframe = NULL;

	ofSoundStreamSettings audioSettings;
	ofSoundStream audioStream;
};

#pragma once

#include "ofMain.h"
#include "ofxFFmpeg.h"

class ofxFFmpegPlayer : public ofxFFmpeg::PacketReceiver, public ofxFFmpeg::FrameReceiver, public ofBaseVideoPlayer, public ofBaseVideoDraws {
public:
	ofxFFmpegPlayer();
	~ofxFFmpegPlayer();

	bool load(string filename);
    void close();
	bool isLoaded() const;
	bool isInitialized() const;

	static bool openHardware(int device_type);

	/////////////////////////////////////////////////
	// VIDEO

	void update();
	bool isFrameNew() const;

	void play();
	void stop();
	bool isPlaying() const;

	void setPaused(bool paused);
	bool isPaused() const;

	void setLoopState(ofLoopType state);

	// Position //

	float getPosition() const;
	int getCurrentFrame() const;
	float getDuration() const;
	bool getIsMovieDone() const;
	int getTotalNumFrames() const;

	// Seeking //

	void nextFrame();
	void setFrame(int frame);
	void setPosition(float pct);
	void setTime(int64_t pts);

	// Draw //
	
	void draw(float x, float y, float w, float h) const;
    void draw(float x, float y) const;
	void drawDebug(float x, float y) const;

    float getWidth() const;
    float getHeight() const;

	// Pixels //

	ofPixels & getPixels();
	const ofPixels & getPixels() const;

	bool setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat getPixelFormat() const;
	ofPixelFormat getPixelFormat(int pix_fmt) const;

	// Textures //

	ofTexture & getTexture();
	const ofTexture & getTexture() const;
	void setUseTexture(bool bUseTex);
	bool isUsingTexture() const;
	std::vector<ofTexture> & getTexturePlanes();
	const std::vector<ofTexture> & getTexturePlanes() const;

	/////////////////////////////////////////////////
	// AUDIO

	void setAudioOutputSettings(const ofSoundStreamSettings & settings);
	ofSoundStreamSettings & getAudioOuputSettings();
	const ofSoundStreamSettings & getAudioOuputSettings() const;

	void audioOut(ofSoundBuffer & buffer);

	size_t getAudioAvailable();
	void openAudio();
	void closeAudio();

protected:

	// PacketReceiver

	virtual void receive(AVPacket * packet);
	virtual void notifyEndPacket();
	virtual void terminatePacketReceiver();
	virtual void resumePacketReceiver();

	// FrameReceiver

	virtual void receive(AVFrame * frame, int stream_index);
	virtual void notifyEndFrame(int stream_index);
	virtual void terminateFrameReceiver();
	virtual void resumeFrameReceiver();

	void updateFrame(AVFrame * frame);
	void updateTextures(AVFrame * frame);
	void updateFormat(int av_format, int width, int height);
	void updateFormatGL(int av_format, int width, int height, int planes);

	void drawDebug(string name, const ofxFFmpeg::Metrics & metrics, float x, float y) const;
	void drawDebug(string name, int size, int capacity, float x, float y) const;

	// NV12 shader

    void loadShaderNV12() const;
    void bindShaderNV12(const ofTexture & textureY, const ofTexture & textureUV) const;
    void unbindShaderNV12() const;

	string filePath;

	ofxFFmpeg::Reader reader;
	ofxFFmpeg::Clock clock;
    
	static ofxFFmpeg::HardwareDevice videoHardware;
	static ofxFFmpeg::OpenGLDevice openglDevice;
	mutable ofxFFmpeg::OpenGLRenderer openglRenderer;
    static ofShader shaderNV12;

	ofxFFmpeg::VideoDecoder video;
	ofxFFmpeg::PacketQueue videoPackets;
	ofxFFmpeg::FrameQueue videoFrames;
	mutable ofxFFmpeg::FrameQueue videoCache;
	ofxFFmpeg::VideoScaler scaler;
	ofxFFmpeg::Metrics transferMetrics;
	ofxFFmpeg::Metrics uploadMetrics;

    ofxFFmpeg::AudioDecoder audio;
	ofxFFmpeg::PacketQueue audioPackets;
	ofxFFmpeg::AudioResampler resampler;
	ofxFFmpeg::AudioBuffer<float> audioBuffer;

	ofSoundStream audioStream;
	ofSoundStreamSettings audioSettings;

	bool isBuffering = false;
	bool isLooping = false;
	bool frameNew = false;
	bool isMovieDone = false;
	int frameNum = 0;

	int64_t realTime;
	int64_t videoTime;
	int64_t audioTime;
	int64_t video_ts_last;
	bool paused = false;

    ofPixelFormat pixelFormat = OF_PIXELS_UNKNOWN;

    std::vector<ofPixels> pixelPlanes;
	std::vector<ofTexture> texturePlanes;
};

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

	/////////////////////////////////////////////////
	// VIDEO

	void update();
	bool isFrameNew() const;

	void play();
	void stop();
	void setPaused(bool paused);

	bool isPaused() const;
	bool isPlaying() const;

    ofPixels & getPixels();
    const ofPixels & getPixels() const;

    void draw(float x, float y, float w, float h) const;
    void draw(float x, float y) const;
	void drawDebug(float x, float y) const;

    float getWidth() const;
    float getHeight() const;

	bool setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat getPixelFormat() const;
	ofPixelFormat getPixelFormat(int pix_fmt) const;

	float getPosition() const;
	int getCurrentFrame() const;
    float getDuration() const;
	bool getIsMovieDone() const;
    int getTotalNumFrames() const;

	void nextFrame();
        
    void setLoopState(ofLoopType state);

	void setFrame(int frame);
	void setPosition(float pct);
	void setTime(int64_t pts);

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

	static bool openHardware(int device_type);

protected:

	virtual bool receive(AVPacket * packet);
	virtual void notifyEndPacket();
	virtual void terminatePacketReceiver();
	virtual void resumePacketReceiver();

	virtual bool receive(AVFrame * frame, int stream_index);
	virtual void terminateFrameReceiver();
	virtual void resumeFrameReceiver();

	void updateFrame(AVFrame * frame);
	void updateTextures(AVFrame * frame);
	void updateFormat(int av_format, int width, int height);

	void drawDebug(string name, const ofxFFmpeg::Metrics & metrics, float x, float y) const;
	void drawDebug(string name, int size, int capacity, float x, float y) const;
    
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

	bool _isPlaying = false;
	bool isBuffering = false;
	bool isLooping = false;
	bool isFlushing = false;
	bool isResyncingVideo = false;
	bool frameNew = false;
	bool isMovieDone = false;
	int frameNum = 0;

	double realTimeSeconds;
	double videoTimeSeconds;
	double audioTimeSeconds;
	uint64_t last_frame_pts;
    bool paused = false;
	//uint64_t audio_samples_loop;

    ofPixelFormat pixelFormat = OF_PIXELS_UNKNOWN;
    ofLoopType loopState = OF_LOOP_NORMAL;
	bool loopRequest = false;

    std::vector<ofPixels> pixelPlanes;
	std::vector<ofTexture> texturePlanes;
};

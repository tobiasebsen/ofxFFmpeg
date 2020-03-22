#pragma once

#include "ofMain.h"
#include "ofxFFmpeg.h"

class ofxFFmpegPlayer : public ofxFFmpeg::PacketReceiver, public ofxFFmpeg::FrameReceiver, public ofBaseVideoPlayer {
public:
	ofxFFmpegPlayer();
	~ofxFFmpegPlayer();

    bool load(string filename);
    void close();
	bool isLoaded() const;
	bool isInitialized() const;

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

    float getWidth() const;
    float getHeight() const;

	bool setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat getPixelFormat() const;

	float getPosition() const;
	int getCurrentFrame() const;
    float getDuration() const;
	bool getIsMovieDone() const;
    int getTotalNumFrames() const;

	void nextFrame();
        
    void setLoopState(ofLoopType state);

	void setFrame(int frame);
	void setPosition(float pct);

	void audioOut(ofSoundBuffer & buffer);

protected:

	virtual void receive(AVPacket * packet);
	virtual void notifyEndPacket();
	virtual void terminatePacketReceiver();
	virtual void resumePacketReceiver();

	virtual void receive(AVFrame * frame, int stream_index);
	virtual void terminateFrameReceiver();
	virtual void resumeFrameReceiver();
	virtual void receive(uint64_t pts, uint64_t duration, const std::shared_ptr<uint8_t> imageData);

	string filePath;

	ofxFFmpeg::Reader reader;
	ofxFFmpeg::Clock clock;
    
    ofxFFmpeg::VideoDecoder video;
	ofxFFmpeg::PacketQueue videoPackets;
	ofxFFmpeg::FrameCache videoFrames;
	ofxFFmpeg::VideoScaler scaler;

    ofxFFmpeg::AudioDecoder audio;
	ofxFFmpeg::PacketQueue audioPackets;
	ofxFFmpeg::AudioResampler resampler;
	ofxFFmpeg::AudioBuffer<float> audioBuffer;

	int64_t lastVideoPts;
	int64_t lastAudioPts;
	int64_t lastUpdatePts;
	int64_t seekPts = -1;
        
    std::mutex mutex;
    std::condition_variable frame_receive_cond;
	std::condition_variable frame_ready_cond;
	std::atomic<bool> terminated;

	ofSoundStream audioStream;

	bool pixelsDirty = false;
	bool frameNew = false;
	bool isMovieDone = false;

	double timeSeconds;
	uint64_t pts;
    bool paused = false;

    ofLoopType loopState = OF_LOOP_NORMAL;
	bool loopRequest = false;

    ofPixels pixels;
    ofTexture texture;
};

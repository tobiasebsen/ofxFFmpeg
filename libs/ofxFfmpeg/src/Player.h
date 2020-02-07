#pragma once

#include "ofMain.h"

#include "Reader.h"
#include "Decoder.h"
#include "VideoScaler.h"

struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;

namespace ofxFFmpeg {

    class Player : public PacketReceiver, public FrameReceiver, public ofBaseVideoPlayer {
    public:
		Player();
		~Player();

        bool load(string filename);
        void close();
		bool isLoaded() const;
		bool isInitialized() const;

		void receivePacket(AVPacket * packet);
		void endRead();
		void receiveFrame(AVFrame * frame, int stream_index);

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
        int getTotalNumFrames() const;
        
        void setLoopState(ofLoopType state);

		void setFrame(int frame);
		void setPosition(float pct);

    protected:

		string filePath;

		ofxFFmpeg::Reader reader;
		ofxFFmpeg::VideoDecoder video;
		ofxFFmpeg::VideoScaler scaler;

		bool playing = false;
		bool pixelsDirty = false;
		bool frameNew = false;

		uint64_t pts;

        ofLoopType loopState = OF_LOOP_NORMAL;

        ofPixels pixels;
        ofTexture texture;
    };
}

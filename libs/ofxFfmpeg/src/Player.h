#pragma once

#include "ofMain.h"

struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;

namespace ofxFFmpeg {

    class Player : public ofBaseVideoPlayer {
    public:
		Player();
		~Player();

        bool load(string filename);
        void close();
		bool isLoaded() const;
		bool isInitialized() const;

		void update();
		bool isFrameNew() const;

		void play();
		void stop();

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

        void readFrame();

    protected:

        AVFormatContext * format_context = NULL;
        AVStream *video_stream = NULL;
        AVCodec *video_codec = NULL;
        AVCodecContext *video_context = NULL;
		AVFrame * frame = NULL;
        SwsContext * sws_context = NULL;

		int video_stream_index = -1;

		bool frameNew;
		uint64_t frameTime;
		uint64_t startTime;
        
        ofLoopType loopState = OF_LOOP_NORMAL;

        ofPixels pixels;
        ofTexture texture;
    };
}

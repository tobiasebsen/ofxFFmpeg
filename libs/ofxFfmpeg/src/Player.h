#pragma once

#include "ofMain.h"
#include "Reader.h"
#include "VideoThread.h"

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

        void fillQueue();
        void readFrame();

    protected:

		string filePath;

        /*AVFormatContext * format_context = NULL;
        AVStream *video_stream = NULL;
        AVCodec *video_codec = NULL;
        AVCodecContext *video_context = NULL;
		//AVFrame * frame = NULL;
        SwsContext * sws_context = NULL;

		int video_stream_index = -1;

        std::map<uint64_t, AVFrame*> frameQueue;
        uint64_t frameQueuePts;
        uint64_t frameQueueDuration;*/

		PacketQueue videoPackets;
		shared_ptr<Reader> reader;
		shared_ptr<VideoThread> video;

		bool playing = false;
		bool frameNew;

		uint64_t timeLastFrame;
		uint64_t pts;

        ofLoopType loopState = OF_LOOP_NORMAL;

        ofPixels pixels;
        ofTexture texture;
    };
}

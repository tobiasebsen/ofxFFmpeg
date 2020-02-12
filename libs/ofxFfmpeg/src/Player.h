#pragma once

#include "ofMain.h"

#include "Flow.h"
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

    protected:

		virtual void receivePacket(AVPacket * packet);
		virtual void endPacket();
		virtual void notifyPacket();
		virtual void receiveFrame(AVFrame * frame, int stream_index);
		virtual void receiveImage(uint64_t pts, uint64_t duration, const std::shared_ptr<uint8_t> imageData);

		string filePath;

		ofxFFmpeg::Reader reader;
        ofxFFmpeg::PacketQueue videoPackets;
		ofxFFmpeg::VideoDecoder video;
		ofxFFmpeg::VideoScaler scaler;
        ofxFFmpeg::AudioDecoder audio;

		int64_t lastVideoPts;
		int64_t lastAudioPts;
        
        std::mutex mutex;
        std::condition_variable frame_receive_cond;
		std::condition_variable frame_ready_cond;

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
}

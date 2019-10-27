#pragma once

#include "ofMain.h"

struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct SwsContext;

namespace ofxFFmpeg {

    class Player {
    public:

        bool load(string filename);
        void close();

        ofPixels & getPixels();
        const ofPixels & getPixels() const;

        void draw(float x, float y, float w, float h) const;
        void draw(float x, float y) const;

        float getWidth() const;
        float getHeight() const;

        bool isInitialized() const;
        
        float getDuration() const;
        
        int getTotalNumFrames() const;

        void readFrame();

    protected:

        AVFormatContext * format_context = NULL;
        AVStream *video_stream = NULL;
        AVCodec *video_codec = NULL;
        AVCodecContext *video_context = NULL;
        
        SwsContext * sws_context = NULL;
        
        ofPixels pixels;
        ofTexture texture;
    };
}

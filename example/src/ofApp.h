#pragma once

#include "ofMain.h"
#include "ofxFFmpeg.h"

class ofApp : public ofBaseApp, public ofxFFmpeg::PacketReceiver, public ofxFFmpeg::FrameReceiver {
public:
    void setup();
    void exit();
    void update();
    void draw();
    
    void receivePacket(AVPacket * packet);
    void receiveFrame(AVFrame * frame);
    
    int video_stream_index;
    int audio_stream_index;
	
    ofxFFmpeg::Player player;
    ofxFFmpeg::Reader reader;
    ofxFFmpeg::PacketQueue packetQueue;
    ofxFFmpeg::VideoDecoder video;
    ofxFFmpeg::AudioDecoder audio;
    ofxFFmpeg::VideoScaler scaler;

    ofxFFmpeg::Recorder recorder;

    ofVideoGrabber grabber;
    ofTexture tex;
    ofPixels pix;
    
    ofFpsCounter fps;
};

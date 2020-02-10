#pragma once

#include "ofMain.h"
#include "ofxFFmpeg.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void exit();
    void update();
    void draw();
    
    ofxFFmpegPlayer player;
    
    ofFpsCounter fps;
};

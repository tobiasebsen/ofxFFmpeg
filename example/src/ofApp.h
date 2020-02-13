#pragma once

#include "ofMain.h"
#include "ofxFFmpegPlayer.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void exit();
    void update();
    void draw();
    
    ofxFFmpegPlayer player;
    
    ofFpsCounter fps;
};

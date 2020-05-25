#pragma once

#include "ofMain.h"
#include "ofxFFmpegPlayer.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void exit();
    void update();
    void draw();
	void keyPressed(int key);
	void mousePressed(int x, int y, int button);
	void dragEvent(ofDragInfo dragInfo);
    
    ofxFFmpegPlayer player;
    
    ofFpsCounter fps;
};

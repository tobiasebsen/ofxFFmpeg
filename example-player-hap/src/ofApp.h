#pragma once

#include "ofMain.h"
#include "HapPlayer.h"

class ofApp : public ofBaseApp{
public:

	void setup();
	void update();
	void draw();

	void dragEvent(ofDragInfo dragInfo);
		
	HapPlayer player;
};

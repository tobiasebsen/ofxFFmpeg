#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofLogToConsole();
}

//--------------------------------------------------------------
void ofApp::update(){

	player.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

	player.draw(0, 0);

	ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate(), 1) + " fps", 20, 20);
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

	player.load(dragInfo.files[0]);
	player.play();
}

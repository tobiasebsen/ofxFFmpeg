#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofLogToConsole();
    ofSetLogLevel(OF_LOG_VERBOSE);

    ofSetVerticalSync(false);
	ofSetFrameRate(0);

	ofxFFmpegPlayer::openHardware(ofxFFmpeg::HardwareDevice::getType("dxva2"));

	ofSoundStreamSettings settings = player.getAudioOuputSettings();
	settings.sampleRate = 48000;
	player.setAudioOutputSettings(settings);

	showDebug = false;
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::update(){

	player.update();
    if (player.isFrameNew())
        fps.newFrame();
}

//--------------------------------------------------------------
void ofApp::draw(){
    
	player.draw(0, 0);
    
    ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate(),1) + " fps", 20, 20);
	ofDrawBitmapStringHighlight(ofToString(fps.getFps(),1) + " fps", 20, 40);

	if (showDebug) {
		player.drawDebug(20, 80);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == ' ') player.stop();
	if (key == 'c') player.close();
	if (key == 'd') showDebug = !showDebug;
    if (key == 'f') ofToggleFullscreen();
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	player.setPosition((float)x / (float)ofGetWidth());
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (dragInfo.files.size() > 0) {
		player.load(dragInfo.files[0]);
		//player.setLoopState(OF_LOOP_NONE);
		player.play();
		//player.setPaused(true);
	}
}


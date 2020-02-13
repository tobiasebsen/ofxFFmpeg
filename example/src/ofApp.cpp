#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofLogToConsole();
    ofSetLogLevel(OF_LOG_VERBOSE);

    ofSetVerticalSync(false);
	ofSetFrameRate(60);
    
    player.load("fingers.mov");
    //player.load("SampleHap.mov");
    //player.load("C:/Users/tobias/Downloads/Left_2019_0614_150843.mov");
    //player.load("/Users/tobias/Downloads/Sky Q Brand Reveal 35 Master TV (1080p) (1).mov");
	//player.load("C:/Users/tobias/Downloads/Setup-Public-perception.mp4");
    
    player.play();
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
}


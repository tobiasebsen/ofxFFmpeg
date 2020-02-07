#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofLogToConsole();
	ofSetFrameRate(60);
    
    //reader.open(ofFilePath::getAbsolutePath("fingers.mov"));
    //reader.open(ofFilePath::getAbsolutePath("SampleHap.mov"));
    //player.load("C:/Users/tobias/Downloads/Left_2019_0614_150843.mov");
    //reader.open("/Users/tobias/Downloads/Sky Q Brand Reveal 35 Master TV (1080p) (1).mov");
	player.load("C:/Users/tobias/Downloads/Setup-Public-perception.mp4");
    
    /*ofLog() << reader.getDuration() << " s";
    ofLog() << reader.getNumStreams() << " streams";
    
    if (video.open(reader)) {

        ofLog() << video.getTotalNumFrames() << " frames";
        ofLog() << video.getWidth() << "x" << video.getHeight();

        pix.allocate(video.getWidth(), video.getHeight(), 3);
        
        scaler.setup(video);
    }
    
    if (audio.open(reader)) {

    }

    reader.start(this);*/
    //video.start(&reader, this);
}

//--------------------------------------------------------------
void ofApp::exit(){
    //recorder.flush();
    //recorder.close();
    //player.close();
    //video.close();
    //reader.close();
}

//--------------------------------------------------------------
void ofApp::update(){
    
    //tex.loadData(pix);
	player.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    /*if (tex.isAllocated())
        tex.draw(0, 0);*/

	player.draw(0, 0);
    
    ofDrawBitmapString(ofToString(ofGetFrameRate(),1) + " fps", 20, 20);
    ofDrawBitmapString(ofToString(fps.getFps(),1) + " fps", 20, 40);
}

//--------------------------------------------------------------
void ofApp::receivePacket(AVPacket *pkt) {

    fps.newFrame();
    
    uint64_t after = ofGetElapsedTimeMicros();
    //ofLog() << "Read:   " << (after-before) << " us";
    
    if (video.match(pkt)) {

         uint64_t before = ofGetElapsedTimeMicros();
         video.send(pkt);
         after = ofGetElapsedTimeMicros();
         ofLog() << "Send: " << (after-before) << " us";
        
         before = after;
         auto frm = video.receive();
         if (frm) {
             after = ofGetElapsedTimeMicros();
             ofLog() << "Receive: " << (after-before) << " us";
             
             before = after;
             scaler.scale(frm, pix.getData());
             after = ofGetElapsedTimeMicros();
             ofLog() << "Scale: " << (after-before) << " us";
             
             //tex.loadData(pix);
             
             video.free(frm);
         }
     }
}

//--------------------------------------------------------------
void ofApp::receiveFrame(AVFrame * frame) {

    fps.newFrame();

    scaler.scale(frame, pix.getData());
}

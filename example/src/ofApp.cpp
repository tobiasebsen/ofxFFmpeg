#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofLogToConsole();
	ofSetFrameRate(60);
    
    //reader.open(ofFilePath::getAbsolutePath("fingers.mov"));
    //reader.open(ofFilePath::getAbsolutePath("SampleHap.mov"));
    //reader.open("/Users/tobias/Downloads/Left_2019_0614_150843.mov");
    reader.open("/Users/tobias/Downloads/Sky Q Brand Reveal 35 Master TV (1080p) (1).mov");
    
    ofLog() << reader.getDuration() << " s";
    ofLog() << reader.getNumStreams() << " streams";
    
    video_stream_index = reader.getVideoStreamIndex();
    if (video_stream_index >= 0) {

        auto stream = reader.getStream(video_stream_index);
        video.open(stream);

        ofLog() << video.getTotalNumFrames() << " frames";
        ofLog() << video.getWidth() << "x" << video.getHeight();

        pix.allocate(video.getWidth(), video.getHeight(), 3);
        
        scaler.setup(video);
    }
    
    audio_stream_index = reader.getAudioStreamIndex();
    if (audio_stream_index >= 0) {

        auto stream = reader.getStream(audio_stream_index);
        audio.open(stream);
    }

    //scaler.setup(reader.getVideoContext());

    //player.load("fingers.mov");
	//player.load("C:/Users/tobias/Downloads/Left_2019_0614_150843.mov");
    //player.load("/Users/tobias/Downloads/Left_2019_0614_150843.mov");
    //player.setLoopState(OF_LOOP_NONE);
    //player.play();

    //grabber.setup(1920/2, 1080/2);
	//grabber.setDesiredFrameRate(30);

	/*recorder.open("test.mov");
    recorder.setCodec("libx264");
    recorder.setWidth(player.getWidth());
    recorder.setHeight(player.getHeight());
    recorder.setFrameRate(30);
    recorder.setBitRate(1000);*/
    //recorder.start();
    
    //reader.start(this);
    video.start(&reader, this);
}

//--------------------------------------------------------------
void ofApp::exit(){
    //recorder.flush();
    //recorder.close();
    //player.close();
    video.close();
    reader.close();
}

//--------------------------------------------------------------
void ofApp::update(){
    
    tex.loadData(pix);
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    if (tex.isAllocated())
        tex.draw(0, 0);
    
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

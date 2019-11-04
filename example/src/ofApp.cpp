#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofLogToConsole();
	ofSetFrameRate(0);
    
    player.load("fingers.mov");
    player.setLoopState(OF_LOOP_NONE);

    //grabber.setup(1920/2, 1080/2);
	//grabber.setDesiredFrameRate(30);

	recorder.open("test.mov");
    recorder.setCodec("libx264");
    recorder.setWidth(player.getWidth());
    recorder.setHeight(player.getHeight());
    recorder.setFrameRate(30);
    recorder.setBitRate(1000);
    recorder.start();
}

//--------------------------------------------------------------
void ofApp::exit(){
    recorder.flush();
    recorder.close();
    player.close();
}

//--------------------------------------------------------------
void ofApp::update(){
    
	player.update();
    if (player.isFrameNew()) {
        recorder.write(player.getPixels());
    }
    
    //tex.loadData(pix);
    
    /*grabber.update();
    if (grabber.isFrameNew()) {
		ofPixels & pixels = grabber.getPixels();
		recorder.write(pixels);
	}*/
}

//--------------------------------------------------------------
void ofApp::draw(){
    //grabber.draw(0, 0);
    player.draw(0, 0);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

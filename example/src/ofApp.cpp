#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    grabber.setup(640, 480);
    
	recorder.open(ofFilePath::getAbsolutePath("test.mov"), 640, 480, 30);
}

//--------------------------------------------------------------
void ofApp::exit(){
    recorder.flush();
    recorder.close();
}

//--------------------------------------------------------------
void ofApp::update(){
    
    grabber.update();
    if (grabber.isFrameNew()) {
        ofPixels & pixels = grabber.getPixels();
        recorder.write(pixels);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    grabber.draw(0, 0);
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

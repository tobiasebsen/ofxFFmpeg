#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    //grabber.setup(640, 480);
    ofPixels pixels;
    ofLoadImage(tex, "test.jpg");
    tex.readToPixels(pixels);

	recorder.open(ofFilePath::getAbsolutePath("test.mov"), 1024, 768, 30);
    recorder.write(pixels);
    recorder.flush();
    recorder.close();
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::update(){
    
    grabber.update();
    if (grabber.isFrameNew()) {
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    //grabber.draw(0, 0);
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

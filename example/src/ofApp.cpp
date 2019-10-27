#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofLogToConsole();
	ofSetFrameRate(60);
    
    player.load(ofFilePath::getAbsolutePath("fingers.mov"));

    //grabber.setup(1920/2, 1080/2);
	//grabber.setDesiredFrameRate(30);

	//recorder.open(ofFilePath::getAbsolutePath("test.mov"), grabber.getWidth(), grabber.getHeight(), 30);
}

//--------------------------------------------------------------
void ofApp::exit(){
    player.close();
	//recorder.flush();
	//recorder.close();
}

//--------------------------------------------------------------
void ofApp::update(){
    
    player.readFrame();
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

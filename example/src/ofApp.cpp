#include "ofApp.h"

#include "pixfmt.h"

//--------------------------------------------------------------
void ofApp::setup(){

    auto codecs = ofxFFmpeg::getOutputFormats(true, false);
    for (auto codec : codecs) {
        cout << ofxFFmpeg::getCodecLongName(codec) << endl;
    }

    int codecId = 27;//ofxFfmpeg::getVideoEncoder("h264rgb");
    ofxFFmpeg::AvCodecPtr codec = ofxFFmpeg::getEncoder(codecId);
    if (codec) {
        codec->setWidth(640);
        codec->setHeight(480);
        codec->setFrameRate(30);
        codec->setPixelFormat(AV_PIX_FMT_YUV420P);
        codec->open();

        ofxFFmpeg::AvFramePtr frame = codec->allocFrame();
        if (frame) {
            frame->getBuffer(32);
            
            ofFile file;
            file.open("test.mov", ofFile::Mode::WriteOnly);

            ofxFFmpeg::AvPacket pkt;
            ofBuffer buffer;

            for (int i=0; i<5; i++) {
                frame->makeWritable();
                frame->setPts(i);
                codec->encode(frame);
                
                while (codec->receivePacket(pkt)) {
                    
                    buffer.append((char*)pkt.getData(), pkt.getSize());
                    file.writeFromBuffer(buffer);
                    buffer.clear();
                    
                    pkt.unref();
                }
            }
            codec->encode();
            if (codec->receivePacket(pkt)) {
                buffer.append((char*)pkt.getData(), pkt.getSize());
                file.writeFromBuffer(buffer);
            }

            file.close();
        }
    }
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){

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

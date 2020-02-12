#include "Recorder.h"

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool Recorder::open(const string filename) {

	string filePath = ofFilePath::getAbsolutePath(filename);

	if (!writer.open(filePath)) {
		ofLogError("Could not open output file");
		return false;
	}

	return true;
}

//--------------------------------------------------------------
bool Recorder::setVideoCodec(int codecId) {
  	return video.setup(codecId);
}

//--------------------------------------------------------------
bool Recorder::setVideoCodec(string codecName) {
	return video.setup(codecName);
}

VideoEncoder & ofxFFmpeg::Recorder::getVideoEncoder() {
	return video;
}

//--------------------------------------------------------------
bool Recorder::start() {

	AVStream * stream = writer.addStream();
	video.begin(stream);

	frame = video.allocateFrame();

	writer.begin();

    n_frame = 0;
    
	scaler.setup(video);
    
    return true;
}
//--------------------------------------------------------------
void Recorder::stop() {
	video.flush(this);
	writer.end();
	video.freeFrame(frame);
	frame = NULL;
	close();
}

//--------------------------------------------------------------
void Recorder::receivePacket(AVPacket * packet) {

	writer.write(packet);
}

//--------------------------------------------------------------
void Recorder::write(const ofPixels & pixels) {

	scaler.scale(pixels.getData(), pixels.getBytesStride(), pixels.getHeight(), frame);
    
	video.setTimeStamp(frame, n_frame);
	n_frame++;

	video.encode(frame, this);
}

//--------------------------------------------------------------
int Recorder::getError() {
	return error;
}

//--------------------------------------------------------------
string Recorder::getErrorString() {
	/*char errstr[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(error, errstr, sizeof(errstr));
	return string(errstr);*/
	return string();
}

//--------------------------------------------------------------
void Recorder::close() {
	video.freeFrame(frame);
	frame = NULL;
	video.close();
	writer.close();
}

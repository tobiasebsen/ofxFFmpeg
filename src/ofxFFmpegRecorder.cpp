#include "ofxFFmpegRecorder.h"

using namespace ofxFFmpeg;

//--------------------------------------------------------------
bool ofxFFmpegRecorder::open(const string filename) {

	string filePath = ofFilePath::getAbsolutePath(filename);

	if (!writer.open(filePath)) {
		ofLogError("Could not open output file");
		return false;
	}

	return true;
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::setVideoCodec(int codecId) {
  	return video.setup(codecId);
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::setVideoCodec(string codecName) {
	return video.setup(codecName);
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::setAudioCodec(int codecId) {
	return audio.setup(codecId);
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::setAudioCodec(string codecName) {
	return audio.setup(codecName);
}

//--------------------------------------------------------------
ofxFFmpeg::VideoEncoder & ofxFFmpegRecorder::getVideoEncoder() {
	return video;
}

//--------------------------------------------------------------
ofxFFmpeg::AudioEncoder & ofxFFmpegRecorder::getAudioEncoder() {
	return audio;
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::start() {

	AVStream * stream = writer.addStream();
	video.begin(stream);

	frame = video.allocateFrame();

	writer.begin();

    frame_count = 0;
    
	scaler.setup(video);
    
    return true;
}
//--------------------------------------------------------------
void ofxFFmpegRecorder::stop() {
	video.flush(this);
	writer.end();
	video.freeFrame(frame);
	frame = NULL;
	close();
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::receivePacket(AVPacket * packet) {

	video.setTimeStamp(packet);
	writer.write(packet);
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::write(const ofPixels & pixels, int frameNumber) {

	scaler.scale(pixels.getData(), pixels.getBytesStride(), pixels.getHeight(), frame);
   
	video.setTimeStamp(frame, (int)(frameNumber == -1 ? frame_count : frameNumber));

	video.encode(frame, this);
	frame_count++;
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::write(const ofPixels & pixels, float timeSeconds) {

	scaler.scale(pixels.getData(), pixels.getBytesStride(), pixels.getHeight(), frame);

	video.setTimeStamp(frame, timeSeconds);

	video.encode(frame, this);
	frame_count++;
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::audioIn(ofSoundBuffer & buffer) {
}

//--------------------------------------------------------------
int ofxFFmpegRecorder::getError() {
	return error;
}

//--------------------------------------------------------------
string ofxFFmpegRecorder::getErrorString() {
	/*char errstr[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(error, errstr, sizeof(errstr));
	return string(errstr);*/
	return string();
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::close() {
	video.freeFrame(frame);
	frame = NULL;
	video.close();
	writer.close();
}

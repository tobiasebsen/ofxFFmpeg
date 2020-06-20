#include "ofxFFmpegRecorder.h"

using namespace ofxFFmpeg;

//--------------------------------------------------------------
ofxFFmpegRecorder::ofxFFmpegRecorder() {
	audioSettings.bufferSize = 512;
	audioSettings.numInputChannels = 2;
	audioSettings.setInListener(this);
}

//--------------------------------------------------------------
ofxFFmpegRecorder::~ofxFFmpegRecorder() {
	close();
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::open(const string filename) {

	string filePath = ofFilePath::getAbsolutePath(filename);

	if (!writer.open(filePath)) {
		ofLogError("Could not open output file");
		return false;
	}

	ofLogVerbose() << "== FORMAT ==";
	ofLogVerbose() << writer.getLongName();

	return true;
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::setVideoCodec(int codecId) {
  	return video.allocate(codecId);
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::setVideoCodec(string codecName) {
	return video.allocate(codecName);
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::setAudioCodec(int codecId) {
	return audio.allocate(codecId);
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::setAudioCodec(string codecName) {
	return audio.allocate(codecName);
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::setAudioInputSettings(const ofSoundStreamSettings & settings) {
	audioSettings = settings;
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

	if (video.isAllocated()) {
		AVStream * stream = writer.addStream();
		
		if (video.open(stream)) {
			ofLogVerbose() << "== VIDEO STREAM ==";
			ofLogVerbose() << "  " << video.getLongName();
			ofLogVerbose() << "  " << video.getWidth() << "x" << video.getHeight();

			ofxFFmpeg::PixelFormat format(video.getPixelFormat());
			ofLogVerbose() << "  " << format.getName();
			ofLogVerbose() << "  " << video.getFrameRate() << " fps";
			ofLogVerbose() << "  " << (video.getBitRate() / 1000.f) << " kb/s";

			vframe = VideoFrame::allocate(video.getWidth(), video.getHeight(), video.getPixelFormat());

			scaler.allocate(video);
		}
	}
	if (audio.isAllocated()) {
		AVStream * stream = writer.addStream();
		
		if (audio.open(stream)) {
			ofLogVerbose() << "== AUDIO STREAM ==";
			ofLogVerbose() << "  " << audio.getLongName();
			ofLogVerbose() << "  " << audio.getNumChannels() << " channels";
			ofLogVerbose() << "  " << audio.getSampleRate() << " Hz";
			ofLogVerbose() << "  " << audio.getFrameSize() << " bytes/frame";
			ofLogVerbose() << "  " << (audio.getBitRate() / 1000.f) << " kb/s";

			aframe = AudioFrame::allocate(audio.getFrameSize(), audio.getNumChannels(), audio.getSampleFormat());

			audioBuffer.allocate(audioSettings.sampleRate * audioSettings.numInputChannels * 1.f); // 1 second buffer
			resampler.allocate(44100, 2, AudioResampler::getSampleFormat<float>(), audio);
		}
	}

	writer.begin();

    frame_count = 0;
    
    return true;
}
//--------------------------------------------------------------
void ofxFFmpegRecorder::stop() {

	writeAudio();
	writeAudioFinal();

	video.flush(this);
	audio.flush(this);
	writer.end();

	close();
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::receive(AVPacket * packet) {

	Packet p(packet);

	if (video.match(packet)) {
		int64_t frame_num = p.getTimeStamp();
		int64_t pts = video.rescaleFrameNum(frame_num);
		p.setTimeStamp(pts);
	}
	if (audio.match(packet)) {
		size_t nb_samples = p.getTimeStamp();
		int64_t pts = audio.rescaleSampleCount(nb_samples);
		p.setTimeStamp(pts);
	}

	writer.write(packet);

	return true;
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::write(const ofPixels & pixels, int frameNumber) {

	scaler.scale(pixels.getData(), pixels.getBytesStride(), pixels.getHeight(), vframe);
   
	VideoFrame(vframe).setTimeStamp((int)(frameNumber == -1 ? frame_count : frameNumber));

	video.encode(vframe, this);
	frame_count++;

	writeAudio();
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::write(const ofPixels & pixels, float timeSeconds) {

	scaler.scale(pixels.getData(), pixels.getBytesStride(), pixels.getHeight(), vframe);

	VideoFrame(vframe).setTimeStamp(timeSeconds);

	video.encode(vframe, this);
	frame_count++;

	writeAudio();
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::writeAudio() {

	AudioFrame f(aframe);
	int nb_samples = resampler.getInSamples(f.getNumSamples());

	while (audio.isOpen() && audioBuffer.getAvailableRead() >= nb_samples * audioSettings.numInputChannels) {

		writeAudio(nb_samples);
	}
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::writeAudio(int nb_samples) {

	float * buffer = (float*)resampler.allocateSamplesInput(nb_samples);

	nb_samples = audioBuffer.read(buffer, nb_samples * audioSettings.numInputChannels) / audioSettings.numInputChannels;

	size_t nb_total = audioBuffer.getTotalRead() / audioSettings.numInputChannels;
	AudioFrame(aframe).setTimeStamp(nb_total);

	int samples = resampler.resample(buffer, nb_samples, aframe);
	AudioFrame(aframe).setNumSamples(samples);

	//ofLog() << "Audio samples write: " << nb_total;

	resampler.free(buffer);

	audio.encode(aframe, this);
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::writeAudioFinal() {

	if (audio.isOpen()) {
		int nb_samples = audioBuffer.getAvailableRead() / audioSettings.numInputChannels;
		writeAudio(nb_samples);
	}
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::audioIn(ofSoundBuffer & buffer) {
	if (audioBuffer.getAvailableWrite() >= buffer.size()) {
		audioBuffer.write(buffer.getBuffer().data(), buffer.size());

		writeAudio();
	}
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
	Frame::free(vframe);
	Frame::free(aframe);
	vframe = NULL;
	aframe = NULL;
	video.free();
	audio.free();
	writer.close();
}

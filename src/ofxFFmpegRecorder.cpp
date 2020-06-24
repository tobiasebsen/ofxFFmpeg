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

	close();

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
		if (!stream)
			return false;
		
		if (!video.open(stream))
			return false;

		ofLogVerbose() << "== VIDEO STREAM ==";
		ofLogVerbose() << "  " << video.getLongName();
		ofLogVerbose() << "  " << video.getWidth() << "x" << video.getHeight();

		ofxFFmpeg::PixelFormat format(video.getPixelFormat());
		ofLogVerbose() << "  " << format.getName();
		ofLogVerbose() << "  " << video.getFrameRate() << " fps";
		ofLogVerbose() << "  " << (video.getBitRate() / 1000.f) << " kb/s";

		vframe = VideoFrame::allocate(video.getWidth(), video.getHeight(), video.getPixelFormat());
		if (!vframe)
			return false;

		frame_count = 0;

		if (!scaler.allocate(video))
			return false;
	}
	if (audio.isAllocated()) {
		AVStream * stream = writer.addStream();
		if (!stream)
			return false;

		if (!audio.open(stream))
			return false;

		ofLogVerbose() << "== AUDIO STREAM ==";
		ofLogVerbose() << "  " << audio.getLongName();
		ofLogVerbose() << "  " << audio.getNumChannels() << " channels";
		ofLogVerbose() << "  " << audio.getSampleRate() << " Hz";
		ofLogVerbose() << "  " << audio.getFrameSize() << " bytes/frame";
		ofLogVerbose() << "  " << (audio.getBitRate() / 1000.f) << " kb/s";

		aframe = AudioFrame::allocate(audio.getFrameSize(), audio.getNumChannels(), audio.getSampleFormat());
		if (!aframe)
			return false;

		audioBuffer.allocate(audioSettings.sampleRate * audioSettings.numInputChannels * 1.f); // 1 second buffer
		if (!resampler.allocate(44100, 2, AudioResampler::getSampleFormat<float>(), audio))
			return false;
	}

	if (!writer.begin())
		return false;

    return true;
}
//--------------------------------------------------------------
void ofxFFmpegRecorder::stop() {

	if (audio.isOpen()) {
		writeAudio();
		writeAudioFinal();
		audio.flush(this);
	}

	if (video.isOpen()) {
		video.flush(this);
	}

	writer.end();

	close();
}

//--------------------------------------------------------------
bool ofxFFmpegRecorder::receive(AVPacket * packet) {

	writer.write(packet);

	return true;
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::write(const ofPixels & pixels) {

	scaler.scale(pixels.getData(), pixels.getBytesStride(), pixels.getHeight(), vframe);
   
	VideoFrame(vframe).setTimeStamp(frame_count);

	video.encode(vframe, this);
	frame_count++;

	writeAudio();
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::writeAudio() {

	if (!audio.isOpen() || !aframe || !resampler.isAllocated())
		return;

	AudioFrame f(aframe);
	int nb_samples = resampler.getInSamples(f.getNumSamples());

	while (audio.isOpen() && audioBuffer.getAvailableRead() >= nb_samples * audioSettings.numInputChannels) {

		writeAudio(nb_samples);
	}
}

//--------------------------------------------------------------
void ofxFFmpegRecorder::writeAudio(int nb_samples) {

	if (!audio.isOpen() || !aframe || !resampler.isAllocated())
		return;

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
	writer.close();
	video.close();
	audio.close();
}

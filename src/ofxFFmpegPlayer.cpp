#include "ofxFFmpegPlayer.h"

using namespace ofxFFmpeg;

ofxFFmpeg::HardwareDecoder ofxFFmpegPlayer::videoHardware;

//--------------------------------------------------------------
ofxFFmpegPlayer::ofxFFmpegPlayer() {
}

//--------------------------------------------------------------
ofxFFmpegPlayer::~ofxFFmpegPlayer() {
	close();
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::load(string filename) {

	close();

	if (!reader.open(ofFilePath::getAbsolutePath(filename, true))) {
		return false;
	}

	timeSeconds = 0;
	lastVideoPts = 0;
	lastUpdatePts = 0;
    
    ofLogVerbose() << "== FORMAT ==";
	ofLogVerbose() << reader.getLongName();
	ofLogVerbose() << reader.getDuration() << " seconds";
    ofLogVerbose() << (reader.getBitRate() / 1024.f) << " kb/s";
    ofLogVerbose() << reader.getNumStreams() << " stream(s)";

	if (!videoHardware.isOpen() && videoHardware.open()) {
		ofLogVerbose() << "== HARDWARE ACCELERATION ==";
		ofLogVerbose() << videoHardware.getDeviceName();
	}

	if (video.open(reader, &videoHardware)) {
		ofLogVerbose() << "== VIDEO STREAM ==";
		int num_hw = HardwareDecoder::getNumHardwareConfig(reader.getVideoCodec());
		ofLogVerbose() << "  " << num_hw << " hardware decoder(s) found";
		ofLogVerbose() << "  " << video.getLongName();
		ofLogVerbose() << "  " << video.getWidth() << "x" << video.getHeight();
        ofLogVerbose() << "  " << video.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << video.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << video.getFrameRate() << " fps";
        ofLogVerbose() << "  " << (video.getBitRate() / 1000.f) << " kb/s";

		ofPixelFormat format = getPixelFormat();
		int num_planes = video.getNumPlanes();

		pixelPlanes.resize(num_planes);
		texturePlanes.resize(num_planes);

		if (format == OF_PIXELS_UNKNOWN) {
			scaler.allocate(video);
			pixelPlanes[0].allocate(video.getWidth(), video.getHeight(), OF_PIXELS_RGB);
		}
		else if (format == OF_PIXELS_NV12) {
			pixelPlanes[0].allocate(video.getWidth(), video.getHeight(), OF_PIXELS_NV12);
			pixelPlanes[1].allocate(video.getWidth() / 2, video.getHeight() / 2, OF_PIXELS_RG);
			texturePlanes[1].allocate(pixelPlanes[1]);
			texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_R, GL_RED);
			texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_G, GL_GREEN);
			texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_B, GL_ZERO);
			texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_A, GL_ONE);
		}
		else {
			pixelPlanes[0].allocate(video.getWidth(), video.getHeight(), format);
		}
    }
    if (audio.open(reader)) {
        ofLogVerbose() << "== AUDIO STREAM ==";
		ofLogVerbose() << "  " << audio.getLongName();
		ofLogVerbose() << "  " << audio.getNumChannels() << " channels";
        ofLogVerbose() << "  " << audio.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << audio.getSampleRate() << " Hz";
        ofLogVerbose() << "  " << audio.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << audio.getFrameSize() << " bytes/frame";
        ofLogVerbose() << "  " << (audio.getBitRate() / 1000.f) << " kb/s";

		ofSoundStreamSettings settings;
		settings.sampleRate = 44100;
		settings.numOutputChannels = 2;
		settings.numInputChannels = 0;
		settings.bufferSize = 512;
		settings.setOutListener(this);
		audioStream.setup(settings);

        resampler.allocate(audio, settings.sampleRate, settings.numOutputChannels, AudioResampler::getSampleFormat<float>());
        audioBuffer.allocate(audioStream.getSampleRate() * audioStream.getNumOutputChannels());
    }

	return true;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::close() {
	
	stop();

	video.close();
	audio.close();
	reader.close();

	scaler.free();
	resampler.free();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::play() {
	if (reader.isOpen()) {
		video.start(&videoPackets, this);
		audio.start(&audioPackets, this);
		reader.start(this);
		isMovieDone = false;
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::stop() {
	video.stop();
	audio.stop();
	reader.stop();
	audioStream.stop();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::receive(AVPacket * packet) {

	// VIDEO PACKETS
	if (video.match(packet)) {
		if (video.isRunning()) {
			videoPackets.receive(packet);
		}
		else {
			video.decode(packet, this);
		}
	}

	// AUDIO PACKETS
    if (audio.match(packet)) {
		if (audio.isRunning()) {
			audioPackets.receive(packet);
		}
		else {
			audio.decode(packet, this);
		}
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::notifyEndPacket() {
    if (loopState == OF_LOOP_NONE) {
        video.flush(this);
		audio.flush(this);
        reader.stop();
		video.stop();
		//isMovieDone = true;
    }
	if (loopState == OF_LOOP_NORMAL) {
		reader.seek(0);
		//timeSeconds = 0;
		//lastVideoPts = 0;
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::terminatePacketReceiver() {
	std::lock_guard<std::mutex> lock(mutex);
	frame_receive_cond.notify_all();
	videoPackets.terminatePacketReceiver();
	audioPackets.terminatePacketReceiver();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::resumePacketReceiver() {
	videoPackets.resumePacketReceiver();
	audioPackets.resumePacketReceiver();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::terminateFrameReceiver() {
	std::lock_guard<std::mutex> lock(mutex);
	terminated = true;
	frame_receive_cond.notify_all();
	audioBuffer.terminate();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::resumeFrameReceiver() {
	terminated = false;
	audioBuffer.resume();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::receive(AVFrame * frame, int stream_index) {

	if (stream_index == video.getStreamIndex()) {
		if (!terminated) {
			std::unique_lock<std::mutex> lock(mutex);
			frame_receive_cond.wait(lock);
		}
		if (!terminated) {
			std::lock_guard<std::mutex> lock(mutex);

			if (scaler.isAllocated())
				scaler.scale(frame, pixelPlanes[0].getData(), pixelPlanes[0].getBytesStride());
			else {
				if (video.getPixelFormat() == OFX_FFMPEG_FORMAT_NV12) {
					scaler.getMetrics().begin();
					scaler.copyPlane(frame, 0, pixelPlanes[0].getData(), pixelPlanes[0].getWidth(), pixelPlanes[0].getHeight());
					scaler.copyPlane(frame, 1, pixelPlanes[1].getData(), pixelPlanes[0].getWidth(), pixelPlanes[1].getHeight());
					scaler.getMetrics().end();
				}
				else {
					scaler.getMetrics().begin();
					scaler.copy(frame, pixelPlanes[0].getData(), pixelPlanes[0].getTotalBytes());
					scaler.getMetrics().end();
				}
			}

			lastVideoPts = video.getTimeStamp(frame);
			pixelsDirty = true;
			frame_ready_cond.notify_one();
		}
	}
	if (stream_index == audio.getStreamIndex()) {

        int samples = 0;
        float * buffer = (float*)resampler.resample(frame, &samples);
		samples *= audioStream.getNumOutputChannels();
        
        if (samples > audioBuffer.get_write_available())
            audioBuffer.wait(samples);

        audioBuffer.write(buffer, samples);
        resampler.free(buffer);
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::update() {

	uint64_t duration = 0;
    
    if (isPlaying() && !isPaused()) {
        double dt = ofGetLastFrameTime();
		timeSeconds += dt;
        pts = timeSeconds / reader.getTimeBase();
		duration = dt / reader.getTimeBase();
		//ofLog() << "Clock: " << pts << " " << duration;

	}

	if (pts > lastVideoPts) {
		std::lock_guard<std::mutex> lock(mutex);
		frame_receive_cond.notify_one();
	}

	if (pixelsDirty) {
        std::lock_guard<std::mutex> lock(mutex);

		//ofLog() << "Last: " << lastVideoPts << " " << video.getFrameNum(lastVideoPts);

		for (int i = 0; i < video.getNumPlanes(); i++) {
			ofTexture & tex = texturePlanes[i];
			ofPixels & pix = pixelPlanes[i];
			if (pix.isAllocated()) {
				if (!tex.isAllocated() || tex.getWidth() != pix.getWidth() || tex.getHeight() != pix.getHeight()) {
					tex.allocate(pix);
				}
				tex.loadData(pix);
			}
		}

		timeSeconds = lastVideoPts * reader.getTimeBase();
		lastUpdatePts = lastVideoPts;
		if (lastUpdatePts == seekPts)
			seekPts = -1;

		pixelsDirty = false;
		frameNew = true;
	}
	else {
		frameNew = false;
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::draw(float x, float y, float w, float h) const {
	/*if (texture.isAllocated())
		texture.draw(x, y, w, h);*/
	ofGetCurrentRenderer()->draw(*this, x, y, w, h);
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::draw(float x, float y) const {
	draw(x, y, getWidth(), getHeight());
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::drawDebug(float x, float y) const {
	ofDrawBitmapStringHighlight("Reader:    " + ofToString(reader.getMetrics().getPeriodFiltered(), 1) + " us", x, y);
	ofDrawBitmapStringHighlight("Video:     " + ofToString(video.getMetrics().getPeriodFiltered(), 1) + " us", x, y + 20);
	ofDrawBitmapStringHighlight("Scaler:    " + ofToString(scaler.getMetrics().getPeriodFiltered(), 1) + " us", x, y + 40);
	ofDrawBitmapStringHighlight("Audio:     " + ofToString(audio.getMetrics().getPeriodFiltered(), 1) + " us", x, y + 60);
	ofDrawBitmapStringHighlight("Resampler: " + ofToString(resampler.getMetrics().getPeriodFiltered(), 1) + " us", x, y + 80);

	ofDrawBitmapStringHighlight(ofToString(reader.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y);
	ofDrawBitmapStringHighlight(ofToString(video.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 20);
	ofDrawBitmapStringHighlight(ofToString(scaler.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 40);
	ofDrawBitmapStringHighlight(ofToString(audio.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 60);
	ofDrawBitmapStringHighlight(ofToString(resampler.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 80);

	float videoLoad = video.getMetrics().getDutyCycleFiltered() + scaler.getMetrics().getDutyCycleFiltered();
	ofDrawBitmapStringHighlight(ofToString(videoLoad*100.f, 1) + " %", x + 220, y + 20, videoLoad > 0.9 ? ofColor::red : ofColor::black);
}

//--------------------------------------------------------------
float ofxFFmpegPlayer::getWidth() const {
	return video.getWidth();
}

//--------------------------------------------------------------
float ofxFFmpegPlayer::getHeight() const {
    return video.getHeight();
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::setPixelFormat(ofPixelFormat pixelFormat) {
	return false;
}

//--------------------------------------------------------------
ofPixelFormat ofxFFmpegPlayer::getPixelFormat() const {
	switch (video.getPixelFormat()) {
	case OFX_FFMPEG_FORMAT_GRAY8:
		return OF_PIXELS_GRAY;
	case OFX_FFMPEG_FORMAT_RGB24:
		return OF_PIXELS_RGB;
	case OFX_FFMPEG_FORMAT_RGBA:
		return OF_PIXELS_RGBA;
	case OFX_FFMPEG_FORMAT_NV12:
		return OF_PIXELS_NV12;
	default:
		return OF_PIXELS_UNKNOWN;
	}
}

//--------------------------------------------------------------
float ofxFFmpegPlayer::getPosition() const {
	return (float)getCurrentFrame() / (float)getTotalNumFrames();
}

//--------------------------------------------------------------
int ofxFFmpegPlayer::getCurrentFrame() const {
	return video.getFrameNum(lastUpdatePts);
}

//--------------------------------------------------------------
float ofxFFmpegPlayer::getDuration() const {
	return reader.getDuration();
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::getIsMovieDone() const {
	return isMovieDone;
}

//--------------------------------------------------------------
int ofxFFmpegPlayer::getTotalNumFrames() const {
	return video.getTotalNumFrames();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::nextFrame() {
	std::lock_guard<std::mutex> lock(mutex);
	frame_receive_cond.notify_all();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setFrame(int f) {
	// TODO
	{
		std::lock_guard<std::mutex> lock(mutex);
		pts = video.getTimeStamp(f);
		timeSeconds = pts * reader.getTimeBase();
		seekPts = video.getTimeStamp(f);
		reader.seek(pts);
		frame_receive_cond.notify_all();
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setPosition(float pct) {
	setFrame(pct * getTotalNumFrames());
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::isLoaded() const {
	return reader.isOpen();
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::isInitialized() const {
	return reader.isOpen();
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::isFrameNew() const {
	return frameNew;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setPaused(bool paused) {
	this->paused = paused;
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::isPaused() const {
	return paused;
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::isPlaying() const {
	return reader.isRunning();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setLoopState(ofLoopType state) {
    loopState = state;
}

//--------------------------------------------------------------
ofPixels & ofxFFmpegPlayer::getPixels() {
	return pixelPlanes[0];
}

//--------------------------------------------------------------
const ofPixels & ofxFFmpegPlayer::getPixels() const {
	return pixelPlanes[0];
}

//--------------------------------------------------------------
ofTexture & ofxFFmpegPlayer::getTexture() {
	return texturePlanes[0];
}

//--------------------------------------------------------------
const ofTexture & ofxFFmpegPlayer::getTexture() const {
	return texturePlanes[0];
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setUseTexture(bool bUseTex) {}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::isUsingTexture() const {
	return true;
}

//--------------------------------------------------------------
std::vector<ofTexture>& ofxFFmpegPlayer::getTexturePlanes() {
	return texturePlanes;
}

//--------------------------------------------------------------
const std::vector<ofTexture>& ofxFFmpegPlayer::getTexturePlanes() const {
	return texturePlanes;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::audioOut(ofSoundBuffer & buffer) {

	int samples = audioBuffer.read(buffer.getBuffer().data(), buffer.size());
	if (samples < buffer.size()) {
		//ofLogWarning() << "Audio buffer under-run!";
	}

	uint64_t duration = buffer.getDurationMicros();
	//clock.tick(duration);
}

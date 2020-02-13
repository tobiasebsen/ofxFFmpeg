#include "ofxFFmpegPlayer.h"

using namespace ofxFFmpeg;


ofxFFmpegPlayer::ofxFFmpegPlayer() {
}

ofxFFmpegPlayer::~ofxFFmpegPlayer() {
	close();
}

bool ofxFFmpegPlayer::load(string filename) {

	if (!reader.open(ofFilePath::getAbsolutePath(filename, true))) {
		return false;
	}

	timeSeconds = 0;
	lastVideoPts = 0;
    
    ofLogVerbose() << "== FORMAT ==";
	ofLogVerbose() << reader.getLongName();
	ofLogVerbose() << reader.getDuration() << " seconds";
    ofLogVerbose() << (reader.getBitRate() / 1024.f) << " kb/s";
    ofLogVerbose() << reader.getNumStreams() << " stream(s)";

	if (video.open(reader)) {
        ofLogVerbose() << "== VIDEO STREAM ==";
		ofLogVerbose() << "  " << video.getLongName();
		ofLogVerbose() << "  " << video.getWidth() << "x" << video.getHeight();
        ofLogVerbose() << "  " << video.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << video.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << video.getFrameRate() << " fps";
        ofLogVerbose() << "  " << (video.getBitRate() / 1024.f) << " kb/s";
        scaler.setup(video);
        pixels.allocate(video.getWidth(), video.getHeight(), OF_IMAGE_COLOR);
    }
    if (audio.open(reader)) {
        ofLogVerbose() << "== AUDIO STREAM ==";
		ofLogVerbose() << "  " << audio.getLongName();
		ofLogVerbose() << "  " << audio.getNumChannels() << " channels";
        ofLogVerbose() << "  " << audio.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << audio.getSampleRate() << " Hz";
        ofLogVerbose() << "  " << audio.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << audio.getFrameSize() << " bytes/frame";
        ofLogVerbose() << "  " << (audio.getBitRate() / 1024.f) << " kb/s";
    }

	return true;
}

void ofxFFmpegPlayer::close() {
	reader.stop(this);
	reader.close();
	video.close();
}

void ofxFFmpegPlayer::receivePacket(AVPacket * packet) {

	if (video.match(packet)) {
		video.decode(packet, this);
        //AVPacket * clone = videoPackets.clone(packet);
        //videoPackets.push(clone);
	}
    if (audio.match(packet)) {
        // audio packet
    }
}

void ofxFFmpegPlayer::endPacket() {
    if (loopState == OF_LOOP_NONE) {
        video.flush(this);
        reader.stop(this);
		isMovieDone = true;
    }
	if (loopState == OF_LOOP_NORMAL) {
		reader.seek(0);
		//timeSeconds = 0;
		//lastVideoPts = 0;
	}
}

void ofxFFmpegPlayer::notifyPacket() {
	std::lock_guard<std::mutex> lock(mutex);
	frame_receive_cond.notify_all();
}

void ofxFFmpegPlayer::receiveFrame(AVFrame * frame, int stream_index) {

	if (stream_index == video.getStreamIndex()) {
		{
			std::unique_lock<std::mutex> lock(mutex);
			frame_receive_cond.wait(lock);
		}
		{
			std::lock_guard<std::mutex> lock(mutex);
			scaler.scale(frame, pixels.getData(), pixels.getBytesStride());
			lastVideoPts = video.getTimeStamp(frame);
			pixelsDirty = true;
			frame_ready_cond.notify_one();
		}
	}
}

void ofxFFmpegPlayer::receiveImage(uint64_t pts, uint64_t duration, const std::shared_ptr<uint8_t> imageData) {
	std::unique_lock<std::mutex> lock(mutex);
	frame_receive_cond.wait(lock);
	pixels.allocate(video.getWidth(), video.getHeight(), 3);
	pixels.setFromPixels(imageData.get(), video.getWidth(), video.getHeight(), 3);
	pixelsDirty = true;
}

void ofxFFmpegPlayer::update() {

	uint64_t duration = 0;
    
    if (isPlaying() && !isPaused()) {
        double dt = ofGetLastFrameTime();
		timeSeconds += dt;
        pts = timeSeconds / reader.getTimeBase();
		duration = dt / reader.getTimeBase();
		//ofLog() << "Clock: " << pts << " " << duration;
    }

	if (pts >= lastVideoPts) {
		std::lock_guard<std::mutex> lock(mutex);
		frame_receive_cond.notify_one();
	}

	if (pixelsDirty) {
        std::lock_guard<std::mutex> lock(mutex);

		//ofLog() << "Last: " << lastVideoPts << " " << video.getFrameNum(lastVideoPts);

		if (!texture.isAllocated() || texture.getWidth() != pixels.getWidth() || texture.getHeight() != pixels.getHeight())
			texture.allocate(pixels);

		texture.loadData(pixels);
		timeSeconds = lastVideoPts * reader.getTimeBase();
		pixelsDirty = false;
		frameNew = true;
	}
	else {
		frameNew = false;
	}
}

void ofxFFmpegPlayer::draw(float x, float y, float w, float h) const {
	if (texture.isAllocated())
		texture.draw(x, y, w, h);
}

void ofxFFmpegPlayer::draw(float x, float y) const {
    if (texture.isAllocated())
        texture.draw(x, y);
}

float ofxFFmpegPlayer::getWidth() const {
	return video.getWidth();
}

float ofxFFmpegPlayer::getHeight() const {
    return video.getHeight();
}

bool ofxFFmpegPlayer::setPixelFormat(ofPixelFormat pixelFormat) {
	return false;
}

ofPixelFormat ofxFFmpegPlayer::getPixelFormat() const {
	return OF_PIXELS_RGB;
}

float ofxFFmpegPlayer::getPosition() const {
	return (float)getCurrentFrame() / (float)getTotalNumFrames();
}

int ofxFFmpegPlayer::getCurrentFrame() const {
	return video.getFrameNum(lastVideoPts);
}

float ofxFFmpegPlayer::getDuration() const {
	return reader.getDuration();
}

bool ofxFFmpegPlayer::getIsMovieDone() const {
	return isMovieDone;
}

int ofxFFmpegPlayer::getTotalNumFrames() const {
	return video.getTotalNumFrames();
}

void ofxFFmpegPlayer::nextFrame() {
	std::lock_guard<std::mutex> lock(mutex);
	frame_receive_cond.notify_all();
}

void ofxFFmpegPlayer::setFrame(int f) {
	// TODO
	{
		std::lock_guard<std::mutex> lock(mutex);
		pts = video.getTimeStamp(f);
		timeSeconds = pts * reader.getTimeBase();
		reader.seek(pts);
		frame_receive_cond.notify_all();
	}
}

void ofxFFmpegPlayer::setPosition(float pct) {
	setFrame(pct * getTotalNumFrames());
}

bool ofxFFmpegPlayer::isLoaded() const {
	return reader.isOpen();
}

bool ofxFFmpegPlayer::isInitialized() const {
	return reader.isOpen();
}

bool ofxFFmpegPlayer::isFrameNew() const {
	return frameNew;
}

void ofxFFmpegPlayer::play() {
	if (reader.isOpen()) {
        //video.start(&videoPackets, this);
		reader.start(this);
		isMovieDone = false;
	}
}

void ofxFFmpegPlayer::stop() {
    reader.stop(this);
}

void ofxFFmpegPlayer::setPaused(bool paused) {
	this->paused = paused;
}

bool ofxFFmpegPlayer::isPaused() const {
	return paused;
}

bool ofxFFmpegPlayer::isPlaying() const {
	return reader.isRunning();
}

void ofxFFmpegPlayer::setLoopState(ofLoopType state) {
    loopState = state;
}

ofPixels & ofxFFmpegPlayer::getPixels() {
	return pixels;
}

const ofPixels & ofxFFmpegPlayer::getPixels() const {
	return pixels;
}

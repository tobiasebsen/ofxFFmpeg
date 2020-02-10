#include "Player.h"

using namespace ofxFFmpeg;


ofxFFmpeg::Player::Player() {
}

ofxFFmpeg::Player::~Player() {
	close();
}

bool Player::load(string filename) {

	if (!reader.open(ofFilePath::getAbsolutePath(filename))) {
		return false;
	}
    
    ofLogVerbose() << "== FORMAT ==";
    ofLogVerbose() << reader.getDuration() << " seconds";
    ofLogVerbose() << (reader.getBitRate() / 1024.f) << " kb/s";
    ofLogVerbose() << reader.getNumStreams() << " stream(s)";

	if (video.open(reader)) {
        ofLogVerbose() << "== VIDEO STREAM ==";
        ofLogVerbose() << "  " << video.getWidth() << "x" << video.getHeight();
        ofLogVerbose() << "  " << video.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << video.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << (video.getTotalNumFrames() / reader.getDuration()) << " fps";
        ofLogVerbose() << "  " << (video.getBitRate() / 1024.f) << " kb/s";
        scaler.setup(video);
        pixels.allocate(video.getWidth(), video.getHeight(), OF_IMAGE_COLOR);
    }
    if (audio.open(reader)) {
        ofLogVerbose() << "== AUDIO STREAM ==";
        ofLogVerbose() << "  " << audio.getNumChannels() << " channels";
        ofLogVerbose() << "  " << audio.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << audio.getSampleRate() << " Hz";
        ofLogVerbose() << "  " << audio.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << audio.getFrameSize() << " bytes/frame";
        ofLogVerbose() << "  " << (audio.getBitRate() / 1024.f) << " kb/s";
    }


	return true;
}

void Player::close() {
    frame_cond.notify_all();
	reader.close();
	video.close();
}

void Player::receivePacket(AVPacket * packet) {

	if (video.match(packet)) {
		video.decode(packet, this);
	}
    if (audio.match(packet)) {
        // audio packet
    }
}

void ofxFFmpeg::Player::endRead() {
    if (loopState == OF_LOOP_NONE) {
        video.flush(this);
        reader.stop();
    }
    if (loopState == OF_LOOP_NORMAL)
        reader.seek(0);
}

void Player::receiveFrame(AVFrame * frame, int stream_index) {

	if (stream_index == video.getStreamIndex()) {
		std::unique_lock<std::mutex> lock(mutex);
		frame_cond.wait(lock);
		scaler.scale(frame, pixels.getData());
		pixelsDirty = true;
	}
}

void Player::receiveImage(uint64_t pts, uint64_t duration, const std::shared_ptr<uint8_t> imageData) {
	std::unique_lock<std::mutex> lock(mutex);
	frame_cond.wait(lock);
	pixels.allocate(video.getWidth(), video.getHeight(), 3);
	pixels.setFromPixels(imageData.get(), video.getWidth(), video.getHeight(), 3);
	pixelsDirty = true;
}

void Player::update() {

	if (pixelsDirty) {
        std::lock_guard<std::mutex> lock(mutex);
		texture.loadData(pixels);
		pixelsDirty = false;
		frameNew = true;
        frame_cond.notify_one();
	}
	else {
        std::lock_guard<std::mutex> lock(mutex);
		frameNew = false;
        frame_cond.notify_one();
	}
}

void Player::draw(float x, float y, float w, float h) const {
	if (texture.isAllocated())
		texture.draw(x, y, w, h);
}

void Player::draw(float x, float y) const {
    if (texture.isAllocated())
        texture.draw(x, y);
}

float Player::getWidth() const {
	return video.getWidth();
}

float Player::getHeight() const {
    return video.getHeight();
}

bool Player::setPixelFormat(ofPixelFormat pixelFormat) {
	return false;
}

ofPixelFormat Player::getPixelFormat() const {
	return OF_PIXELS_RGB;
}

float Player::getPosition() const {
	return (float)getCurrentFrame() / (float)getTotalNumFrames();
}

int Player::getCurrentFrame() const {

	return -1;
}

float Player::getDuration() const {
	return reader.getDuration();
}

int Player::getTotalNumFrames() const {
	return 0;
}

void Player::setFrame(int f) {

}

void Player::setPosition(float pct) {
	setFrame(pct * getTotalNumFrames());
}

bool Player::isLoaded() const {
	return reader.isOpen();
}

bool Player::isInitialized() const {
	return false;
}

bool Player::isFrameNew() const {
	return frameNew;
}

void Player::play() {
	if (reader.isOpen()) {
		reader.start(this);
		playing = true;
	}
}

void Player::stop() {
    reader.stop();
	playing = false;
}

void ofxFFmpeg::Player::setPaused(bool paused) {
	playing = !paused;
}

bool Player::isPaused() const {
	return !playing;
}

bool Player::isPlaying() const {
	return reader.isRunning();
}

void Player::setLoopState(ofLoopType state) {
    loopState = state;
}

ofPixels & Player::getPixels() {
	return pixels;
}

const ofPixels & Player::getPixels() const {
	return pixels;
}

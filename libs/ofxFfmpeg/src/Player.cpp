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

	if (!video.open(reader)) {
		return false;
	}

	scaler.setup(video);

	pixels.allocate(video.getWidth(), video.getHeight(), OF_IMAGE_COLOR);

	reader.start(this);

	return true;
}

void Player::close() {
	reader.close();
	video.close();
}

void Player::receivePacket(AVPacket * packet) {

	if (video.match(packet)) {
		video.decode(packet, this);
	}
}

void ofxFFmpeg::Player::endRead() {
	video.flush(this);
}

void Player::receiveFrame(AVFrame * frame, int stream_index) {

	if (stream_index == video.getStreamIndex()) {
		scaler.scale(frame, pixels.getData());
		pixelsDirty = true;
	}
}

void Player::update() {

	if (pixelsDirty) {
		texture.loadData(pixels);
		pixelsDirty = false;
		frameNew = true;
	}
	else {
		frameNew = false;
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

}

void Player::stop() {
	playing = false;
}

void ofxFFmpeg::Player::setPaused(bool paused) {
	playing = !paused;
}

bool Player::isPaused() const {
	return !playing;
}

bool Player::isPlaying() const {
	return playing;
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

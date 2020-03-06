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
	lastUpdatePts = 0;
    
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

		ofPixelFormat format = getPixelFormat();
		if (format == OF_PIXELS_UNKNOWN)
	        scaler.setup(video);
        pixels.allocate(video.getWidth(), video.getHeight(), format == OF_PIXELS_UNKNOWN ? OF_PIXELS_RGB : format);
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

		ofSoundStreamSettings settings;
		settings.sampleRate = audio.getSampleRate();
		settings.numOutputChannels = audio.getNumChannels();
		//settings.numBuffers = 2;
		//settings.bufferSize = 1024;
		settings.setOutListener(this);
		//audioStream.setup(settings);
    }

	return true;
}

void ofxFFmpegPlayer::close() {
	reader.stop(this);
	reader.close();
	video.close();
	audio.close();
}

void ofxFFmpegPlayer::receivePacket(AVPacket * packet) {

	if (video.match(packet)) {
        //ofLog() << "video packet " << video.getTimeStamp(packet);
		video.decode(packet, this);
        //AVPacket * clone = videoPackets.clone(packet);
        //videoPackets.push(clone);
	}
    if (audio.match(packet)) {
        //ofLog() << "audio packet " << audio.getTimeStamp(packet);
        // audio packet
		//audio.decode(packet, this);
    }
}

void ofxFFmpegPlayer::endPacket() {
    if (loopState == OF_LOOP_NONE) {
        video.flush(this);
        reader.stop(this);
		//isMovieDone = true;
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
			

			if (scaler.isSetup())
				scaler.scale(frame, pixels.getData(), pixels.getBytesStride());
			else
				video.copy(frame, pixels.getData(), pixels.getTotalBytes());

			lastVideoPts = video.getTimeStamp(frame);
			pixelsDirty = true;
			frame_ready_cond.notify_one();
		}
	}
	if (stream_index == audio.getStreamIndex()) {

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

	if (pts > lastVideoPts) {
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
	switch (video.getPixelFormat()) {
	case OFX_FFMPEG_FORMAT_GRAY8:
		return OF_PIXELS_GRAY;
	case OFX_FFMPEG_FORMAT_RGB24:
		return OF_PIXELS_RGB;
	case OFX_FFMPEG_FORMAT_RGBA:
		return OF_PIXELS_RGBA;
	default:
		return OF_PIXELS_UNKNOWN;
	}
}

float ofxFFmpegPlayer::getPosition() const {
	return (float)getCurrentFrame() / (float)getTotalNumFrames();
}

int ofxFFmpegPlayer::getCurrentFrame() const {
	return video.getFrameNum(lastUpdatePts);
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
		seekPts = video.getTimeStamp(f);
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

void ofxFFmpegPlayer::audioOut(ofSoundBuffer & buffer) {

	audioBuffer.read(buffer.getBuffer().data(), buffer.getNumFrames(), buffer.getNumChannels(), buffer.getSampleRate());

	uint64_t duration = buffer.getDurationMicros();
	//clock.tick(duration);
}

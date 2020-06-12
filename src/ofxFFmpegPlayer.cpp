#include "ofxFFmpegPlayer.h"

using namespace ofxFFmpeg;

ofxFFmpeg::HardwareDevice ofxFFmpegPlayer::videoHardware;

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

	videoTimeSeconds = 0;
	audioTimeSeconds = 0;
	last_frame_pts = 0;
    
    ofLogVerbose() << "== FORMAT ==";
	ofLogVerbose() << reader.getLongName();
	ofLogVerbose() << reader.getDuration() << " seconds";
    ofLogVerbose() << (reader.getBitRate() / 1024.f) << " kb/s";
    ofLogVerbose() << reader.getNumStreams() << " stream(s)";

	ofLogVerbose() << "== HARDWARE ACCELERATION ==";

	if (videoHardware.isOpen() || videoHardware.open()) {
		ofLogVerbose() << videoHardware.getName();
		vector<int> formats = videoHardware.getFormats();
		if (openglDevice.isOpen() || openglDevice.open(videoHardware)) {
			ofLogVerbose() << "OpenGL compatible";
		}
	}

	if (video.open(reader, videoHardware)) {
		ofLogVerbose() << "== VIDEO STREAM ==";
		int num_hw = HardwareDevice::getNumHardwareConfig(reader.getVideoCodec());
		ofLogVerbose() << "  " << num_hw << " hardware decoder(s) found";
		ofLogVerbose() << "  " << video.getLongName();
		ofLogVerbose() << "  " << video.getWidth() << "x" << video.getHeight();
        ofLogVerbose() << "  " << video.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << video.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << video.getFrameRate() << " fps";
        ofLogVerbose() << "  " << (video.getBitRate() / 1000.f) << " kb/s";

		if (video.hasHardwareDecoder() && openglDevice.isOpen()) {

			openglRenderer.open(openglDevice, video.getWidth(), video.getHeight(), GL_TEXTURE_RECTANGLE);
            int planes = openglRenderer.getNumPlanes();
			texturePlanes.resize(planes);
            
            for (int i=0; i<planes; i++) {
                texturePlanes[i].setUseExternalTextureID(openglRenderer.getTexture(i));

                ofTextureData & texData = texturePlanes[i].texData;
                texData.textureTarget = openglRenderer.getTarget();
                texData.width = openglRenderer.getWidth(i);
                texData.height = openglRenderer.getHeight(i);
                texData.tex_w = texData.width;
                texData.tex_h = texData.height;
                if (texData.textureTarget == GL_TEXTURE_RECTANGLE) {
                    texData.tex_t = texData.width;
                    texData.tex_u = texData.height;
                }
                else {
                    texData.tex_t = texData.tex_w / texData.width;
                    texData.tex_u = texData.tex_h / texData.height;
                }
            }
		}
		else {

			ofPixelFormat format = getPixelFormat(video.getPixelFormat());
			int num_planes = video.getNumPlanes();

			if (video.hasHardwareDecoder()) {
				format = OF_PIXELS_NV12;
				num_planes = 2;
			}

			pixelPlanes.clear();
			texturePlanes.clear();

			updateFormat(format, video.getWidth(), video.getHeight(), num_planes);
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

	openglRenderer.close();

	video.close();
	audio.close();
	reader.close();

	scaler.free();
	resampler.free();

	openglRenderer.close();
	pixelPlanes.clear();
	texturePlanes.clear();

	audioBuffer.free();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::play() {
	if (reader.isOpen()) {
		isBuffering = true;
		isMovieDone = false;

		if (scaler.isAllocated()) {
			scaler.start(&videoFrames, this, video.getStreamIndex());
			video.start(&videoPackets, &videoFrames);
		}
		else {
			video.start(&videoPackets, this);
		}
		audio.start(&audioPackets, this);
		reader.start(this);

		_isPlaying = true;
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::stop() {

	reader.stop();

	video.stop();
	videoPackets.flush();
	video.flush();


	videoPackets.resize(4);
	videoFrames.flush();
	videoCache.flush();

	scaler.stop();

	audio.stop();
	audioPackets.flush();
	audio.flush();

	audioStream.stop();

	_isPlaying = false;
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::receive(AVPacket * packet) {

	// VIDEO PACKETS
	if (video.match(packet)) {

		if (video.isRunning()) {

			// Increment video packet queue
			if (videoPackets.size() == videoPackets.capacity() && audio.isRunning() && audioPackets.size() == 0) {
				videoPackets.resize(videoPackets.capacity() + 1);
			}

			return videoPackets.receive(packet);
		}
		else {
			return video.decode(packet, this);
		}
	}

	// AUDIO PACKETS
    if (audio.match(packet)) {

		// Increment audio packet queue
		if (audioPackets.size() == audioPackets.capacity() && video.isRunning() && videoPackets.size() == 0) {
			audioPackets.resize(audioPackets.capacity() + 1);
		}

		if (audio.isRunning()) {
			return audioPackets.receive(packet);
		}
		else {
			return audio.decode(packet, this);
		}
	}

	return false;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::notifyEndPacket() {
	if (loopState == OF_LOOP_NORMAL) {
		isLooping = true;
		reader.seek(0);
	}
	else { // loopState == OF_LOOP_NONE

		reader.stop();
		video.stop();
		videoPackets.flush(this);
		video.flush(this);

		videoPackets.resize(4);

		scaler.stop();

		audio.stop();
		audioPackets.flush(this);
		audio.flush();
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::terminatePacketReceiver() {
	//if (video.isRunning())
		videoPackets.terminatePacketReceiver();
	/*else
		videoCache.terminateFrameReceiver();*/

	audioPackets.terminatePacketReceiver();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::resumePacketReceiver() {
	videoPackets.resumePacketReceiver();
	audioPackets.resumePacketReceiver();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::terminateFrameReceiver() {
	videoFrames.terminateFrameReceiver();
	videoCache.terminateFrameReceiver();
	audioBuffer.terminate();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::resumeFrameReceiver() {
	videoFrames.resumeFrameReceiver();
	videoCache.resumeFrameReceiver();
	audioBuffer.resume();
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::receive(AVFrame * frame, int stream_index) {

	bool ret = false;

	if (stream_index == video.getStreamIndex()) {

		/*if (video.isHardwareFrame(frame) && !openglRenderer.isOpen()) {
			transferMetrics.begin();
			AVFrame * sw_frame = HardwareDevice::transfer(frame);
			transferMetrics.end();

			if (sw_frame) {
				ret = videoCache.receive(sw_frame, stream_index);
			}
			HardwareDevice::free(sw_frame);
		}
		else*/ {
			ret = videoCache.receive(frame, stream_index);
		}
	}
	if (stream_index == audio.getStreamIndex()) {

        int samples = 0;
        float * buffer = (float*)resampler.resample(frame, &samples);
		if (buffer && samples > 0) {
			samples *= audioStream.getNumOutputChannels();

			if (samples > audioBuffer.getAvailableWrite()) {
				if (video.isRunning() && videoCache.size() == 0)
					audioBuffer.resize(audioBuffer.size() + samples);
				else
					audioBuffer.wait(samples);
			}

			ret = audioBuffer.write(buffer, samples) > 0 ? true : false;
			resampler.free(buffer);
		}
		if (isLooping) {
			audio_samples_loop = audioBuffer.getTotalWrite();
		}
	}

	if (isBuffering) {
		if ((!video.isOpen() || videoCache.size() == videoCache.capacity()) && (!audio.isOpen() || audioBuffer.getAvailableRead() > audioStream.getBufferSize())) {
			isBuffering = false;
		}
	}

	return ret;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::update() {

	int64_t pts = videoTimeSeconds / reader.getTimeBase();
	int64_t frame_pts = video.rescaleTimeInv(pts);
	
	if (isBuffering) {
		if ((!video.isOpen() || videoCache.size() > 0) && (!audio.isOpen() || audioBuffer.getAvailableRead() > audioStream.getBufferSize())) {
			isBuffering = false;
		}
	}

	if (video.isOpen() && isPlaying() && !isBuffering) {

		AVFrame * frame = videoCache.supply(frame_pts);

		if (frame) {

			if (isResyncingVideo)
				isResyncingVideo = false;

			if (openglRenderer.isOpen() && video.isHardwareFrame(frame)) {
				openglRenderer.render(frame);
			}
			else {
				if (video.isHardwareFrame(frame)) {
					transferMetrics.begin();
					AVFrame * sw_frame = HardwareDevice::transfer(frame);
					transferMetrics.end();
					updateFrame(sw_frame);
					HardwareDevice::free(sw_frame);
				}
				else {
					updateFrame(frame);
				}
			}

			frameNew = true;
		}
		else {
			frameNew = false;
		}

		size_t n_flush = videoCache.flush(last_frame_pts, frame_pts);
		if (n_flush > 0) {
			ofLog() << "Flushed: " << n_flush;
		}

		if (frame) {
			last_frame_pts = frame_pts;
			videoCache.free(frame);
		}
	}


	// UPDATE CLOCK
	if (isPlaying() && !isPaused() && !isBuffering) {
		double dt = ofGetLastFrameTime();

		if (!reader.isRunning() && videoCache.size() == 0 && audioBuffer.getAvailableRead() == 0)
			_isPlaying = false;
		else
			videoTimeSeconds += dt;

		// TIME END
		if (videoTimeSeconds >= reader.getDuration()) {
			if (loopState == OF_LOOP_NORMAL) {
				videoTimeSeconds -= reader.getDuration();
				isLooping = false;
			}
			if (loopState == OF_LOOP_NONE) {
				stop();
				_isPlaying = false;
				isMovieDone = true;
			}
		}
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::updateFrame(AVFrame * frame) {

	int num_planes = video.getNumPlanes();
	if (num_planes == 0) num_planes = 1;

	if (pixelPlanes.size() != num_planes || texturePlanes.size() != num_planes) {
		pixelPlanes.resize(num_planes);
		texturePlanes.resize(num_planes);
	}

	int pix_fmt = video.getPixelFormat(frame);
	ofPixelFormat format = getPixelFormat(pix_fmt);
	if (format != pixelPlanes[0].getPixelFormat() || video.getWidth(frame) != pixelPlanes[0].getWidth() || video.getHeight(frame) != pixelPlanes[0].getHeight()) {
		updateFormat(format, video.getWidth(frame), video.getHeight(frame), num_planes);
	}

	for (size_t i = 0; i < pixelPlanes.size(); i++) {
		pixelPlanes[i].setFromAlignedPixels(video.getFrameData(frame, i), pixelPlanes[i].getWidth(), pixelPlanes[i].getHeight(), pixelPlanes[i].getPixelFormat(), video.getLineBytes(frame, i));
		texturePlanes[i].loadData(pixelPlanes[i]);
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::updateFormat(ofPixelFormat format, int width, int height, int planes) {

	if (format == OF_PIXELS_UNKNOWN) {
        pixelPlanes.resize(1);
        texturePlanes.resize(1);
		scaler.allocate(width, height, video.getPixelFormat(), FFMPEG_FORMAT_RGB24);
		pixelPlanes[0].allocate(width, height, OF_PIXELS_RGB);
		texturePlanes[0].allocate(pixelPlanes[0]);
	}
	else if (format == OF_PIXELS_NV12) {
        pixelPlanes.resize(2);
        texturePlanes.resize(2);
		pixelPlanes[0].allocate(width, height, OF_PIXELS_NV12);
		texturePlanes[0].allocate(pixelPlanes[0]);
		pixelPlanes[1].allocate(width / 2, height / 2, OF_PIXELS_RG);
		texturePlanes[1].allocate(pixelPlanes[1]);
		texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_R, GL_RED);
		texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_B, GL_ZERO);
		texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_A, GL_ONE);
	}
	else {
        pixelPlanes.resize(planes);
        texturePlanes.resize(planes);
		pixelPlanes[0].allocate(width, height, format);
		texturePlanes[0].allocate(pixelPlanes[0]);
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::draw(float x, float y, float w, float h) const {
	if (texturePlanes.size() > 0 && texturePlanes[0].isAllocated()) {
		openglRenderer.lock();
		texturePlanes[1].draw(x, y, w, h);
		openglRenderer.unlock();
	}

	//ofGetCurrentRenderer()->draw(*this, x, y, w, h);
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
	ofDrawBitmapStringHighlight("Transfer:  " + ofToString(transferMetrics.getPeriodFiltered(), 1) + " us", x, y + 60);
	ofDrawBitmapStringHighlight("Audio:     " + ofToString(audio.getMetrics().getPeriodFiltered(), 1) + " us", x, y + 80);
	ofDrawBitmapStringHighlight("Resampler: " + ofToString(resampler.getMetrics().getPeriodFiltered(), 1) + " us", x, y + 100);

	ofDrawBitmapStringHighlight(ofToString(reader.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y);
	ofDrawBitmapStringHighlight(ofToString(video.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 20);
	ofDrawBitmapStringHighlight(ofToString(scaler.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 40);
	ofDrawBitmapStringHighlight(ofToString(transferMetrics.getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 60);
	ofDrawBitmapStringHighlight(ofToString(audio.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 80);
	ofDrawBitmapStringHighlight(ofToString(resampler.getMetrics().getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y + 100);

	y -= 15;

	ofPushStyle();
	ofSetColor(255, 50);
	for (int i = 0; i < 11; i++) {
		ofDrawRectangle(x + 240, y + i * 20, 100, 15);
	}
	ofPopStyle();

	ofDrawRectangle(x + 240, y, reader.getMetrics().getDutyCycleFiltered()*100.f, 15);
	ofDrawRectangle(x + 240, y + 20, video.getMetrics().getDutyCycleFiltered()*100.f, 15);
	ofDrawRectangle(x + 240, y + 40, scaler.getMetrics().getDutyCycleFiltered()*100.f, 15);
	ofDrawRectangle(x + 240, y + 60, transferMetrics.getDutyCycleFiltered()*100.f, 15);
	ofDrawRectangle(x + 240, y + 80, audio.getMetrics().getDutyCycleFiltered()*100.f, 15);
	ofDrawRectangle(x + 240, y + 100, resampler.getMetrics().getDutyCycleFiltered()*100.f, 15);

	ofDrawRectangle(x + 240, y + 120, videoPackets.size()*100.f/videoPackets.capacity(), 15);
	ofDrawRectangle(x + 240, y + 180, audioPackets.size()*100.f / audioPackets.capacity(), 15);

	ofSetColor(reader.isRunning() ? ofColor::green : reader.isOpen() ? ofColor::yellow : ofColor::red);
	ofDrawCircle(x + 360, y + 5, 5);
	ofSetColor(video.isRunning() ? ofColor::green : video.isOpen() ? ofColor::yellow : ofColor::red);
	ofDrawCircle(x + 360, y + 25, 5);
	ofSetColor(scaler.isRunning() ? ofColor::green : scaler.isAllocated() ? ofColor::yellow : ofColor::red);
	ofDrawCircle(x + 360, y + 45, 5);
	ofSetColor(audio.isRunning() ? ofColor::green : audio.isOpen() ? ofColor::yellow : ofColor::red);
	ofDrawCircle(x + 360, y + 85, 5);
	ofSetColor(ofColor::white);


	y += 15;

	ofDrawBitmapStringHighlight("Video packets: " + ofToString(videoPackets.size()) + "/" + ofToString(videoPackets.capacity()), x, y + 120);
	ofDrawBitmapStringHighlight("Video frames: " + ofToString(videoFrames.size()) + "/" + ofToString(videoFrames.capacity()), x, y + 140);
	ofDrawBitmapStringHighlight("Video cache: " + ofToString(videoCache.size()) + "/" + ofToString(videoCache.capacity()), x, y + 160);
	ofDrawBitmapStringHighlight("Audio packets: " + ofToString(audioPackets.size()) + "/" + ofToString(audioPackets.capacity()), x, y + 180);
	ofDrawBitmapStringHighlight("Audio buffer:  " + ofToString(audioBuffer.size() ? 100*audioBuffer.getAvailableRead()/audioBuffer.size() : 0) + " %", x, y + 200);

	float scaleX = ofGetWidth() / reader.getDuration();

	if (audio.isOpen()) {
		float aud_pos_x = (audioTimeSeconds / reader.getDuration()) * ofGetWidth();
		ofDrawLine(aud_pos_x, 0, aud_pos_x, ofGetHeight());
		ofDrawBitmapString("Audio: " + ofToString(audioTimeSeconds), aud_pos_x, y + 230);

		float write_x = scaleX * (float)((audioBuffer.getTotalWrite() / audioStream.getNumOutputChannels())) / (float)(audioStream.getSampleRate());
		float read_x = scaleX * (float)((audioBuffer.getTotalRead() / audioStream.getNumOutputChannels())) / (float)(audioStream.getSampleRate());
		ofDrawRectangle(read_x, y + 240, write_x - read_x, 10);
	}

	if (video.isOpen()) {

		float vid_pos_x = getPosition() * ofGetWidth();
		ofDrawLine(vid_pos_x, 0, vid_pos_x, ofGetHeight());
		ofDrawBitmapString("Video: " + ofToString(audioTimeSeconds), vid_pos_x, y + 270);

		videoCache.lock();
		for (size_t i = 0; i < videoCache.size(); i++) {
			AVFrame * frame = videoCache.get(i);
			double t = video.rescaleTime(frame) * reader.getTimeBase();
			double d = video.rescaleDuration(frame) * reader.getTimeBase();
			ofDrawRectangle(t * scaleX, y + 280, d * scaleX, 10);
		}
		videoCache.unlock();
	}
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
	return pixelPlanes.size() > 0 ? pixelPlanes[0].getPixelFormat() : OF_PIXELS_UNKNOWN;
}

//--------------------------------------------------------------
ofPixelFormat ofxFFmpegPlayer::getPixelFormat(int pix_fmt) const {
	switch (pix_fmt) {
	case FFMPEG_FORMAT_GRAY8:
		return OF_PIXELS_GRAY;
	case FFMPEG_FORMAT_RGB24:
		return OF_PIXELS_RGB;
	case FFMPEG_FORMAT_RGBA:
		return OF_PIXELS_RGBA;
	case FFMPEG_FORMAT_NV12:
		return OF_PIXELS_NV12;
	default:
		return OF_PIXELS_UNKNOWN;
	}
}

//--------------------------------------------------------------
float ofxFFmpegPlayer::getPosition() const {
	float duration = getDuration();
	return duration > 0 ? videoTimeSeconds / duration : 0;
}

//--------------------------------------------------------------
int ofxFFmpegPlayer::getCurrentFrame() const {
	return 0;// video.getFrameNum(lastUpdatePts);
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
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setFrame(int f) {
	// TODO
	if (video.isOpen()) {
		int64_t pts = video.rescaleFrameNum(f);
		last_frame_pts = 0;// video.rescaleTimeInv(pts);

		ofLog() << "Seek: " << pts;

		videoTimeSeconds = pts * reader.getTimeBase();
		audioTimeSeconds = videoTimeSeconds;

		reader.stop();
		video.stop();
		audio.stop();

		video.flush();
		audio.flush();

		reader.seek(pts);

		videoPackets.flush();
		videoFrames.flush();
		videoCache.flush();

		audioPackets.flush();
		audioBuffer.reset();

		play();

		isBuffering = true;
		//paused = true;
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
	return _isPlaying;
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

	if (!isPlaying() || isPaused() || isBuffering)
		return;

	int samples = audioBuffer.read(buffer.getBuffer().data(), buffer.size());
	if (samples < buffer.size()) {
		ofLogWarning() << "Audio buffer under-run!";
	}

	uint64_t duration = buffer.getDurationMicros();
	audioTimeSeconds += duration / 1000000.;
	//clock.tick(duration);
}

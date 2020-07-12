#include "ofxFFmpegPlayer.h"

#ifdef OF_SOUNDSTREAM_RTAUDIO
#include "ofRtAudioSoundStream.h"
#define OF_SOUND_STREAM_TYPE ofRtAudioSoundStream
#endif

using namespace ofxFFmpeg;

ofxFFmpeg::HardwareDevice ofxFFmpegPlayer::videoHardware;
ofxFFmpeg::OpenGLDevice ofxFFmpegPlayer::openglDevice;
ofShader ofxFFmpegPlayer::shaderNV12;

//--------------------------------------------------------------
ofxFFmpegPlayer::ofxFFmpegPlayer() {
	audioSettings.numOutputChannels = 2;
	audioSettings.bufferSize = 512;
	audioSettings.setOutListener(this);
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

	if (!videoHardware.isOpen()) {
		openHardware(HardwareDevice::getDefaultType());
	}

    loadShaderNV12();

	frameNum = 0;
	realTime = 0;
	audioTime = 0;
	video_ts_last = 0;

    ofLogVerbose() << "== FORMAT ==";
	ofLogVerbose() << reader.getLongName();
	ofLogVerbose() << reader.getDurationSeconds() << " seconds";
    ofLogVerbose() << (reader.getBitRate() / 1024.f) << " kb/s";
    ofLogVerbose() << reader.getNumStreams() << " stream(s)";

	ofLogVerbose() << "== META DATA ==";
	auto metadata = reader.getMetadata();
	for (auto & m : metadata) {
		ofLogVerbose() << "  " << m.first << " : " << m.second;
	}

	if (video.open(reader, videoHardware)) {
		ofLogVerbose() << "== VIDEO STREAM ==";
		int num_hw = HardwareDevice::getNumConfig(reader.getVideoCodec());
		ofLogVerbose() << "  " << num_hw << " hardware decoder(s) found";
		ofLogVerbose() << "  " << video.getLongName();
		ofLogVerbose() << "  " << video.getTagString();
		ofLogVerbose() << "  " << video.getWidth() << "x" << video.getHeight();

		ofxFFmpeg::PixelFormat format(video.getPixelFormat());
		ofLogVerbose() << "  " << format.getName();
        ofLogVerbose() << "  " << video.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << video.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << video.getFrameRate() << " fps";
		ofLogVerbose() << "  " << video.getDurationSeconds() << " seconds";
        ofLogVerbose() << "  " << (video.getBitRate() / 1000.f) << " kb/s";


		if (video.hasHardwareDecoder() && openglDevice.isOpen()) {

			// OpenGL rendering
			if (openglRenderer.open(openglDevice, video.getWidth(), video.getHeight(), GL_TEXTURE_RECTANGLE)) {
				int format = openglRenderer.getPixelFormat();
                int planes = openglRenderer.getNumPlanes();
				updateFormatGL(format, video.getWidth(), video.getHeight(), planes);
            }
		}
		else {

			// Pixel-upload rendering
			int format = video.hasHardwareDecoder() ? FFMPEG_FORMAT_NV12 : video.getPixelFormat();
			updateFormat(format, video.getWidth(), video.getHeight());
		}
    }
    if (audio.open(reader)) {
        ofLogVerbose() << "== AUDIO STREAM ==";
		ofLogVerbose() << "  " << audio.getLongName();
		ofLogVerbose() << "  " << audio.getNumChannels() << " channels";
        ofLogVerbose() << "  " << audio.getBitsPerSample() << " bits";
        ofLogVerbose() << "  " << audio.getSampleRate() << " Hz";
        ofLogVerbose() << "  " << audio.getTotalNumFrames() << " frames";
        ofLogVerbose() << "  " << audio.getNumSamples() << " samples/frame";
		ofLogVerbose() << "  " << audio.getDurationSeconds() << " seconds";
        ofLogVerbose() << "  " << (audio.getBitRate() / 1000.f) << " kb/s";

		audioStream.setup(audioSettings);

        resampler.allocate(audio, audioSettings.sampleRate, audioSettings.numOutputChannels, AudioResampler::getSampleFormat<float>());
        audioBuffer.allocate(audioSettings.sampleRate * audioSettings.numOutputChannels * 1.f); // 1 second buffer
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

	videoPackets.clear();
	videoCache.clear();
	audioPackets.clear();
	audioBuffer.free();

	scaler.free();
	resampler.free();

	openglRenderer.close();
	pixelPlanes.clear();
	texturePlanes.clear();
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::play() {

	if (reader.isOpen()) {

		isBuffering = true;
		isMovieDone = false;

		audioBuffer.reset();

		if (scaler.isAllocated()) {
			scaler.start(&videoFrames, this, video.getStreamIndex());
			video.start(&videoPackets, &videoFrames);
		}
		else {
			video.start(&videoPackets, this);
		}
		audio.start(&audioPackets, this);
		//reader.seek(0, this);
		reader.start(this);
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::stop() {

	reader.stop();

	video.stop();
	video.flush();
	videoPackets.clear();

	videoPackets.resize(4);
	videoFrames.clear();
	videoCache.clear();

	scaler.stop();

	audio.stop();
	audio.flush();
	audioPackets.clear();

	//audioStream.stop();

	realTime = 0;
	audioTime = 0;
	frameNum = 0;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::receive(AVPacket * packet) {

	// VIDEO PACKETS
	if (video.match(packet)) {

		if (video.isRunning()) {

			// Increment video packet queue
			if (videoPackets.size() == videoPackets.capacity() && audio.isRunning() && audioPackets.size() == 0 && audioBuffer.getAvailableRead() == 0) {
				if (videoPackets.size() < 32) {
					videoPackets.resize(videoPackets.capacity() + 1);
				}
			}

			videoPackets.receive(packet);
		}
		else {
			video.decode(packet, this);
		}
	}

	// AUDIO PACKETS
    if (audio.match(packet)) {

		// Increment audio packet queue
		if (audioPackets.size() == audioPackets.capacity() && video.isRunning() && videoPackets.size() == 0) {
			audioPackets.resize(audioPackets.capacity() + 1);
		}

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
	if (reader.getLoop()) {
	}
	else { // loopState == OF_LOOP_NONE

		videoPackets.terminate();
		audioPackets.terminate();

	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::terminatePacketReceiver() {
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
void ofxFFmpegPlayer::receive(AVFrame * frame, int stream_index) {

	if (stream_index == video.getStreamIndex()) {

		videoCache.receive(frame, stream_index);
	}
	if (stream_index == audio.getStreamIndex()) {

		bool audioIsOpen = audioStream.getSoundStream() != NULL;

		/*ofLog() << AudioFrame(frame).getNumSamples() << " " << AudioFrame(frame).getTimeStamp();
		if (AudioFrame(frame).getNumSamples() < 1024) {
			ofLog();
		}*/

		int samples = 0;
		float * buffer = (float*)resampler.resample(frame, &samples);
		if (buffer && samples > 0) {

			samples *= audioSettings.numOutputChannels;

			if (samples > audioBuffer.getAvailableWrite()) {
				if (video.isRunning() && videoCache.size() < videoCache.capacity() && isBuffering)
					audioBuffer.resize(audioBuffer.size() + samples);
				else
					audioBuffer.wait(samples);
			}

			audioBuffer.write(buffer, samples);
			resampler.free(buffer);
		}
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::notifyEndFrame(int stream_index) {

	if (stream_index == video.getStreamIndex()) {
	}
	if (stream_index == audio.getStreamIndex()) {
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::update() {

	if (isBuffering) {
		if ((!video.isOpen() || videoCache.size() == videoCache.capacity()) && (!audio.isOpen() || audioBuffer.getAvailableRead() > audioSettings.bufferSize * audioSettings.numOutputChannels)) {
			isBuffering = false;
		}
	}

	if (video.isOpen() && isPlaying() && !isBuffering) {

		int64_t video_ts_now = video.rescaleTimeInv(realTime);
		AVFrame * frame = videoCache.supply(video_ts_now);

		updateFrame(frame);

		videoCache.clear(video_ts_last, video_ts_now);

		if (frame) {
			video_ts_last = video_ts_now;
			videoTime = video.rescaleTime(frame) + video.rescaleDuration(frame);
			videoCache.free(frame);
		}
	}

	// UPDATE CLOCK
	if (isPlaying() && !isPaused() && !isBuffering) {
		int64_t delta = ofGetLastFrameTime() / reader.getTimeBase();

		// Only update real-time if it's not ahead of audio-time
		if (!(video.isOpen() && audio.isOpen() && realTime > audioTime))
			realTime += delta;

		// TIME END
		if (realTime >= reader.getDuration() && isPlaying()) {

			if (reader.getLoop()) {
				realTime -= reader.getDuration();
				audioTime -= reader.getDuration();
				isLooping = false;
			}
			else {
				isMovieDone = true;
				realTime = reader.getDuration();
			}
		}

		// RE-SYNC VIDEO TO AUDIO
		if (video.isOpen() && audio.isOpen() && realTime > audioTime - 0.1) {
			realTime = audioTime;
		}
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::updateFrame(AVFrame * frame) {

	frameNew = frame != NULL;

	if (!frame)
		return;

	if (openglRenderer.isOpen() && video.isHardwareFrame(frame)) {
		openglRenderer.render(frame);
	}
	else {
		if (video.isHardwareFrame(frame)) {

			transferMetrics.begin();
			AVFrame * sw_frame = HardwareDevice::transfer(frame);
			transferMetrics.end();

			uploadMetrics.begin();
			updateTextures(sw_frame);
			uploadMetrics.end();

			HardwareDevice::free(sw_frame);
		}
		else {
			uploadMetrics.begin();
			updateTextures(frame);
			uploadMetrics.end();
		}
	}

	frameNum = video.getFrameNum(frame);
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::updateTextures(AVFrame * frame) {

	VideoFrame f(frame);
	int av_format = f.getPixelFormat();
	ofPixelFormat of_format = getPixelFormat(av_format);
	if (pixelPlanes.size() == 0 || of_format != pixelPlanes[0].getPixelFormat() || f.getWidth() != pixelPlanes[0].getWidth() || f.getHeight() != pixelPlanes[0].getHeight()) {
		updateFormat(av_format, f.getWidth(), f.getHeight());
	}

	for (size_t i = 0; i < pixelPlanes.size(); i++) {
		pixelPlanes[i].setFromAlignedPixels(f.getData(i), pixelPlanes[i].getWidth(), pixelPlanes[i].getHeight(), pixelPlanes[i].getPixelFormat(), f.getLineSize(i) * pixelPlanes[i].getBytesPerChannel());
		texturePlanes[i].loadData(pixelPlanes[i]);
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::updateFormat(int av_format, int width, int height) {

	pixelPlanes.clear();
	texturePlanes.clear();
	scaler.free();

	pixelFormat = getPixelFormat(av_format);

	// oF native formats
	if (av_format == FFMPEG_FORMAT_GRAY8 || av_format == FFMPEG_FORMAT_RGB24 || av_format == FFMPEG_FORMAT_RGBA) {
		pixelPlanes.resize(1);
		texturePlanes.resize(1);
		pixelPlanes[0].allocate(width, height, pixelFormat);
		texturePlanes[0].allocate(pixelPlanes[0]);
	}
	// NV12 is a 2-plane 12-bit format
	else if (av_format == FFMPEG_FORMAT_NV12) {
		pixelPlanes.resize(2);
		texturePlanes.resize(2);
		pixelPlanes[0].allocate(width, height, OF_PIXELS_GRAY);
		texturePlanes[0].allocate(pixelPlanes[0]);
		pixelPlanes[1].allocate(width / 2, height / 2, OF_PIXELS_RG);
		texturePlanes[1].allocate(pixelPlanes[1]);

		// NOTE: With OpenGL < 3.0 - OF_PIXELS_RG are loaded as GL_LUMINANCE_ALPHA
		if (!ofIsGLProgrammableRenderer()) {
			texturePlanes[1].setSwizzle(GL_TEXTURE_SWIZZLE_G, GL_ALPHA);
		}
	}
	// Other formats need scaling
	else {
		ofxFFmpeg::PixelFormat format(av_format);
		int dst_fmt = format.hasAlpha() ? FFMPEG_FORMAT_RGBA : FFMPEG_FORMAT_RGB24;
		pixelFormat = getPixelFormat(dst_fmt);

		if (!scaler.allocate(width, height, av_format, dst_fmt)) {
			ofLogError() << "Scaler cannot convert specified format";
			return;
		}
		pixelPlanes.resize(1);
		texturePlanes.resize(1);
		pixelPlanes[0].allocate(width, height, pixelFormat);
		texturePlanes[0].allocate(pixelPlanes[0]);
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::updateFormatGL(int av_format, int width, int height, int planes) {

	pixelFormat = getPixelFormat(av_format);

	texturePlanes.resize(planes);

	for (int i = 0; i < planes; i++) {
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

//--------------------------------------------------------------
void ofxFFmpegPlayer::draw(float x, float y, float w, float h) const {
    
    if (isBuffering) return;
    
    if (openglRenderer.isOpen()) {
        openglRenderer.lock();
        if (openglRenderer.getPixelFormat() == FFMPEG_FORMAT_NV12 && texturePlanes.size() == 2) {
            bindShaderNV12(texturePlanes[0], texturePlanes[1]);
            texturePlanes[0].draw(x, y, w, h);
            unbindShaderNV12();
        }
        else if (texturePlanes.size() == 1) {
            texturePlanes[0].draw(x, y, w, h);
        }
        openglRenderer.unlock();
    }
    else {
		if (pixelFormat == OF_PIXELS_NV12 && texturePlanes.size() == 2) {
			bindShaderNV12(texturePlanes[0], texturePlanes[1]);
			texturePlanes[0].draw(x, y, w, h);
			unbindShaderNV12();
		}
		else {
			ofGetCurrentRenderer()->draw(*this, x, y, w, h);
		}
    }
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::draw(float x, float y) const {
	draw(x, y, getWidth(), getHeight());
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::drawDebug(float x, float y) const {

	drawDebug("Reader:    ", reader.getMetrics(), x, y);
	drawDebug("Video:     ", video.getMetrics(), x, y + 20);
	drawDebug("Scaler:    ", scaler.getMetrics(), x, y + 40);
	drawDebug("Transfer:  ", transferMetrics, x, y + 60);
	drawDebug("Upload:    ", uploadMetrics, x, y + 80);
	drawDebug("Audio:     ", audio.getMetrics(), x, y + 100);
	drawDebug("Resampler: ", resampler.getMetrics(), x, y + 120);

	ofSetColor(reader.isRunning() ? ofColor::green : reader.isOpen() ? ofColor::yellow : ofColor::red);
	ofDrawCircle(x + 360, y - 7, 5);
	ofSetColor(video.isRunning() ? ofColor::green : video.isOpen() ? ofColor::yellow : ofColor::red);
	ofDrawCircle(x + 360, y + 13, 5);
	ofSetColor(scaler.isRunning() ? ofColor::green : scaler.isAllocated() ? ofColor::yellow : ofColor::red);
	ofDrawCircle(x + 360, y + 33, 5);
	ofSetColor(audio.isRunning() ? ofColor::green : audio.isOpen() ? ofColor::yellow : ofColor::red);
	ofDrawCircle(x + 360, y + 93, 5);
	ofSetColor(ofColor::white);

	drawDebug("Video packets: ", videoPackets.size(), videoPackets.capacity(), x, y + 160);
	drawDebug("Video frames:  ", videoFrames.size(), videoFrames.capacity(), x, y + 180);
	drawDebug("Video cache:   ", videoCache.size(), videoCache.capacity(), x, y + 200);
	drawDebug("Audio packets: ", audioPackets.size(), audioPackets.capacity(), x, y + 220);
	drawDebug("Audio buffer:  ", audioBuffer.getAvailableRead(), audioBuffer.size(), x, y + 240);

	ofDrawBitmapStringHighlight(isBuffering ? "Buffering" : "Not bufferting", x, y + 280);

	int64_t duration = reader.getDuration();
	float scaleX = duration != 0 ? ofGetWidth() / reader.getDuration() : 0;

	float pos_x = (realTime / (double)reader.getDuration()) * ofGetWidth();
	ofDrawLine(pos_x, 0, pos_x, ofGetHeight());
	ofDrawBitmapString("Real: " + ofToString(realTime / 1000000., 2), pos_x, y + 300);

	if (audio.isOpen()) {
		float aud_pos_x = (audioTime / (double)reader.getDuration()) * ofGetWidth();
		ofDrawLine(aud_pos_x, 0, aud_pos_x, ofGetHeight());
		ofDrawBitmapString("Audio: " + ofToString(audioTime/1000000., 2), aud_pos_x, y + 340);

		float write_x = scaleX * (float)((audioBuffer.getTotalWrite() / audioStream.getNumOutputChannels())) / (float)(audioStream.getSampleRate());
		float read_x = scaleX * (float)((audioBuffer.getTotalRead() / audioStream.getNumOutputChannels())) / (float)(audioStream.getSampleRate());
		ofDrawRectangle(read_x, y + 350, write_x - read_x, 10);
	}

	if (video.isOpen()) {

		float vid_pos_x = getPosition() * ofGetWidth();
		ofDrawLine(vid_pos_x, 0, vid_pos_x, ofGetHeight());
		ofDrawBitmapString("Video: " + ofToString(realTime/1000000.,2), vid_pos_x, y + 380);

		videoCache.lock();
		for (size_t i = 0; i < videoCache.size(); i++) {
			/*AVFrame * frame = videoCache.get(i);
			double t = video.rescaleTime(frame) * reader.getTimeBase();
			double d = video.rescaleDuration(frame) * reader.getTimeBase();
			ofDrawRectangle(t * scaleX, y + 390, d * scaleX, 10);*/
		}
		videoCache.unlock();
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::drawDebug(string name, const ofxFFmpeg::Metrics & metrics, float x, float y) const {

	ofDrawBitmapStringHighlight(name + ofToString(metrics.getPeriodFiltered(), 1) + " us", x, y);
	ofSetColor(255, 50);
	ofDrawRectangle(x + 240, y - 15, 100, 15);
	ofSetColor(255, 255);
	ofDrawRectangle(x + 240, y - 15, metrics.getDutyCycleFiltered()*100.f, 15);
	ofDrawBitmapStringHighlight(ofToString(metrics.getDutyCycleFiltered()*100.f, 1) + " %", x + 160, y);
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::drawDebug(string name, int size, int capacity, float x, float y) const {
	ofDrawBitmapStringHighlight(name + ofToString(size) + "/" + ofToString(capacity), x, y);
	ofSetColor(255, 50);
	ofDrawRectangle(x + 240, y - 15, 100, 15);
	ofSetColor(255, 255);
	if (capacity != 0) {
		ofDrawRectangle(x + 240, y - 15, size*100.f / capacity, 15);
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
	return pixelFormat;
}

//--------------------------------------------------------------
ofPixelFormat ofxFFmpegPlayer::getPixelFormat(int av_format) const {
	switch (av_format) {
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
	double duration = video.getDurationSeconds();
	return duration > 0 ? videoTime / duration : 0;
}

//--------------------------------------------------------------
int ofxFFmpegPlayer::getCurrentFrame() const {
	return frameNum;
}

//--------------------------------------------------------------
float ofxFFmpegPlayer::getDuration() const {
	return reader.getDurationSeconds();
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
	AVFrame * frame = videoCache.peek();
	if (frame) {
		realTime = video.rescaleTime(frame);
		audioTime = realTime;
		//audioPackets.flush();
	}
	else {
		while (0);
	}
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setFrame(int f) {
	int64_t pts = video.rescaleFrameNum(f);
	setTime(pts);
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setPosition(float pct) {
	int64_t pts = (pct * reader.getDuration()) / reader.getTimeBase();
	setTime(pts);
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setTime(int64_t pts) {

	if (video.isOpen()) {

		video_ts_last = 0;// video.rescaleTimeInv(pts);
		realTime = pts * reader.getTimeBase();
		audioTime = realTime;

		reader.stop();
		video.stop();
		audio.stop();

		video.flush();
		audio.flush();

		videoPackets.clear();
		videoFrames.clear();
		videoCache.clear();

		audioPackets.clear();
		audioBuffer.reset();

		reader.seek(pts, this);

		play();

		//isBuffering = true;
		//paused = true;
	}
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
	return reader.isRunning() || !isMovieDone;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::setLoopState(ofLoopType state) {
	reader.setLoop(state == OF_LOOP_NORMAL);
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
void ofxFFmpegPlayer::setAudioOutputSettings(const ofSoundStreamSettings & settings) {
	audioSettings = settings;
}

//--------------------------------------------------------------
ofSoundStreamSettings & ofxFFmpegPlayer::getAudioOuputSettings() {
	return audioSettings;
}

//--------------------------------------------------------------
const ofSoundStreamSettings & ofxFFmpegPlayer::getAudioOuputSettings() const {
	return audioSettings;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::audioOut(ofSoundBuffer & buffer) {

	bool audioIsOpen = audioStream.getSoundStream() != NULL;

	if (audioIsOpen && (!isPlaying() || isPaused() || isBuffering))
		return;

	if (!audio.isOpen())
		return;

	int samples = audioBuffer.read(buffer.getBuffer().data(), buffer.size());
	if (samples < buffer.size()) {
		//isBuffering = true;
		//ofLogWarning() << "Audio buffer under-run!";
	}

	uint64_t duration = buffer.getDurationMicros();
	audioTime += duration;
	//clock.tick(duration);
}

//--------------------------------------------------------------
size_t ofxFFmpegPlayer::getAudioAvailable() {
	return audioBuffer.getAvailableRead() / audioSettings.numOutputChannels;
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::openAudio() {
	audioStream.setSoundStream(make_shared<OF_SOUND_STREAM_TYPE>());
	audioStream.setup(audioSettings);
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::closeAudio() {
	audioStream.close();
	auto soudStream = audioStream.getSoundStream();
	soudStream.reset();
	audioStream.setSoundStream(shared_ptr<ofBaseSoundStream>(nullptr));
}

//--------------------------------------------------------------
bool ofxFFmpegPlayer::openHardware(int device_type) {
	ofLogVerbose() << "== HARDWARE ACCELERATION ==";

	if (videoHardware.open(device_type)) {
		ofLogVerbose() << videoHardware.getName();

		if (openglDevice.open(videoHardware)) {
			ofLogVerbose() << "OpenGL compatible";
		}

		return true;
	}
	return false;
}

//--------------------------------------------------------------
#define STRINGIFY(x) #x
static string source_nv12_fragment = "#version 120\n" STRINGIFY(
uniform sampler2DRect texY;\n
uniform sampler2DRect texUV;\n
uniform vec2 scaleUV;\n
const vec3 offset = vec3(-0.0625, -0.5, -0.5);\n
const vec3 rcoeff = vec3(1.164, 0.000, 1.596);\n
const vec3 gcoeff = vec3(1.164,-0.391,-0.813);\n
const vec3 bcoeff = vec3(1.164, 2.018, 0.000);\n
void main() {\n
    vec3 yuv;\n
    yuv.x = texture2DRect(texY, gl_TexCoord[0].st).r;\n
    yuv.yz = texture2DRect(texUV, gl_TexCoord[0].st * scaleUV).rg;\n
    yuv += offset;\n
    float r = dot(yuv, rcoeff);\n
    float g = dot(yuv, gcoeff);\n
    float b = dot(yuv, bcoeff);\n
    gl_FragColor = vec4(r, g, b, 1.0);\n
}\n
);

//--------------------------------------------------------------
void ofxFFmpegPlayer::loadShaderNV12() const {
    if (!shaderNV12.isLoaded()) {
        ofLogLevel level = ofGetLogLevel();
        ofSetLogLevel(OF_LOG_WARNING);
        shaderNV12.setupShaderFromSource(GL_FRAGMENT_SHADER, source_nv12_fragment);
        shaderNV12.bindDefaults();
        shaderNV12.linkProgram();
        ofSetLogLevel(level);
    }
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::bindShaderNV12(const ofTexture & textureY, const ofTexture & textureUV) const {
    loadShaderNV12();
    shaderNV12.begin();
    shaderNV12.setUniformTexture("texY", textureY, 0);
    shaderNV12.setUniformTexture("texUV", textureUV, 1);
    shaderNV12.setUniform2f("scaleUV", textureUV.getWidth() / textureY.getWidth(), textureUV.getHeight() / textureY.getHeight());
}

//--------------------------------------------------------------
void ofxFFmpegPlayer::unbindShaderNV12() const {
    shaderNV12.end();
}

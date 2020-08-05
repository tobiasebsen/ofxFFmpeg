#include "HapPlayer.h"

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))

static const string vertexShader = \
	"void main(void)\
    {\
    gl_FrontColor = gl_Color;\
    gl_Position = ftransform();\
    gl_TexCoord[0] = gl_MultiTexCoord0;\
    }";

static const string fragmentShader = \
	"uniform sampler2D cocgsy_src;\
    const vec4 offsets = vec4(-0.50196078431373, -0.50196078431373, 0.0, 0.0);\
    void main()\
    {\
    vec4 CoCgSY = texture2D(cocgsy_src, gl_TexCoord[0].xy);\
    CoCgSY += offsets;\
    float scale = ( CoCgSY.z * ( 255.0 / 8.0 ) ) + 1.0;\
    float Co = CoCgSY.x / scale;\
    float Cg = CoCgSY.y / scale;\
    float Y = CoCgSY.w;\
    vec4 rgba = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0);\
    gl_FragColor = rgba * gl_Color;\
    }";

//--------------------------------------------------------------
HapPlayer::~HapPlayer() {
	close();
}

//--------------------------------------------------------------
bool HapPlayer::load(string filename) {

	close();

	if (!reader.open(ofFilePath::getAbsolutePath(filename, true))) {
		ofLogError("HapPlayer") << "Unable to open file";
		return false;
	}

	if (!video.open(reader)) {
		ofLogError("HapPlayer") << "Unable to open HAP codec";
		return false;
	}

	if (audio.open(reader)) {

		ofSoundStreamSettings audioSettings;
		audioSettings.sampleRate = 44100;
		audioSettings.bufferSize = 512;
		audioSettings.numOutputChannels = 2;
		audioSettings.setOutListener(this);

		audioStream.setup(audioSettings);

		resampler.allocate(audio, audioSettings.sampleRate, audioSettings.numOutputChannels, resampler.getSampleFormat<float>());
		audioBuffer.allocate(audioSettings.sampleRate * audioSettings.numOutputChannels * 2);
	}

	realTime = 0;
	video_pts_last = 0;

	return true;
}

//--------------------------------------------------------------
void HapPlayer::close() {
	reader.close();
	video.close();
	videoCache.clear();
	audio.close();
	texture.clear();
}

//--------------------------------------------------------------
void HapPlayer::play() {

	if (reader.isOpen()) {
		reader.start(this);
	}
}

//--------------------------------------------------------------
void HapPlayer::stop() {
	reader.stop();
}

//--------------------------------------------------------------
void HapPlayer::update() {

	if (video.isOpen()) {
		int64_t video_pts_now = video.rescaleTimeInv(realTime);
		AVFrame * frame = videoCache.supply(video_pts_now);
		videoCache.clear(video_pts_last, video_pts_now);
		if (frame) {
			updateFrame(frame);
			video_pts_last = video_pts_now;
			ofxFFmpeg::Frame::free(frame);
		}
	}

	int64_t delta = ofGetLastFrameTime() / reader.getTimeBase();
	if (reader.isRunning()) {
		realTime += delta;
		if (realTime > reader.getDuration() && reader.getLoop()) {
			realTime -= reader.getDuration();
		}
	}
}

//--------------------------------------------------------------
void HapPlayer::draw(float x, float y) const {
	if (texture.isAllocated()) {
		texture.draw(x, y);

		ofDrawBitmapStringHighlight("Reader: " + ofToString(reader.getMetrics().getPeriodFiltered()) + " us", 20, 40);
		ofDrawBitmapStringHighlight("Video:  " + ofToString(video.getMetrics().getPeriodFiltered()) + " us", 20, 60);
		ofDrawBitmapStringHighlight("Audio:  " + ofToString(audio.getMetrics().getPeriodFiltered()) + " us", 20, 80);
	}
}

//--------------------------------------------------------------
void HapPlayer::receive(AVPacket * packet) {

	if (video.match(packet)) {
		video.decode(packet, this);
	}
	if (audio.match(packet)) {
		audio.decode(packet, this);
	}
}

//--------------------------------------------------------------
void HapPlayer::terminatePacketReceiver() {
	videoCache.terminate();
}

//--------------------------------------------------------------
void HapPlayer::resumePacketReceiver() {
	videoCache.resume();
}

//--------------------------------------------------------------
void HapPlayer::receive(AVFrame * frame, int stream_index) {

	if (stream_index == video.getStreamIndex()) {
		videoCache.receive(frame, stream_index);
	}
	if (stream_index == audio.getStreamIndex()) {

		int samples = 0;
		float * buffer = (float*)resampler.resample(frame, &samples);
		if (buffer && samples > 0) {
			audioBuffer.wait_and_write(buffer, samples * audioStream.getNumOutputChannels());
			resampler.free(buffer);
		}
	}
}

//--------------------------------------------------------------
void HapPlayer::updateFrame(AVFrame * frame) {

	ofxFFmpeg::VideoFrame f(frame);

	if (!texture.isAllocated()) {
		ofTextureData texData;
		texData.width = video.getCodedWidth();
		texData.height = video.getCodedHeight();
		texData.textureTarget = GL_TEXTURE_2D;
		switch (video.getTag()) {
		case MKTAG('H', 'a', 'p', '1'):
			texData.glInternalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; break;
		case MKTAG('H', 'a', 'p', '5'):
		case MKTAG('H', 'a', 'p', 'Y'):
			texData.glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
		default:
			texData.glInternalFormat = GL_RGBA;break;
		}
		texture.allocate(texData, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
		texture.texData.width = video.getWidth();
		texture.texData.height = video.getHeight();
		texture.texData.tex_t = texture.texData.width / texture.texData.tex_w;
		texture.texData.tex_u = texture.texData.height / texture.texData.tex_h;

	}

	texture.bind();

	glCompressedTexSubImage2D(GL_TEXTURE_2D,
		0,
		0,
		0,
		video.getCodedWidth(),
		video.getCodedHeight(),
		texture.texData.glInternalFormat,
		(GLsizei)f.getSize(),
		f.getData());

	texture.unbind();
}

//--------------------------------------------------------------
void HapPlayer::audioOut(ofSoundBuffer & buffer) {

	audioBuffer.read(buffer.getBuffer().data(), buffer.size());
}

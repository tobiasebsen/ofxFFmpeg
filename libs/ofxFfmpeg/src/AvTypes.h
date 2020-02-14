#pragma once

#define OFX_FFMPEG_FORMAT_GRAY8	8
#define OFX_FFMPEG_FORMAT_RGB24	2
#define OFX_FFMPEG_FORMAT_RGBA	26

struct AVIOContext; 
struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct AVBufferRef;
struct SwsContext;
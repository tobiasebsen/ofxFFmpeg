#pragma once

#define FFMPEG_NOPTS_VALUE          ((int64_t)UINT64_C(0x8000000000000000))

#define FFMPEG_FORMAT_GRAY8	8
#define FFMPEG_FORMAT_RGB24	2
#define FFMPEG_FORMAT_RGBA	26
#define FFMPEG_FORMAT_NV12	23

struct AVIOContext; 
struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVCodecParameters;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct AVBufferRef;
struct SwsContext;
struct SwrContext;
struct AVCodecHWConfig;
struct AVHWDeviceContext;
struct AVPixFmtDescriptor;
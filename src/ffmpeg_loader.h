#ifndef FFMPEG_LOADER_H
#define FFMPEG_LOADER_H

#include <string>
#include <cstdint>

// Forward declarations for FFmpeg types
extern "C" {
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVPacket;
struct AVFrame;
struct AVCodec;
struct AVCodecParameters;
struct AVRational;
struct AVDictionary;
struct AVOutputFormat;
struct AVIOContext;
struct AVBSFContext;
struct AVBitStreamFilter;
struct SwsContext;
struct SwrContext;
}

namespace FFmpegLib {
    // avformat functions
    extern int (*avformat_open_input)(AVFormatContext**, const char*, const AVOutputFormat*, AVDictionary**);
    extern void (*avformat_close_input)(AVFormatContext**);
    extern int (*avformat_find_stream_info)(AVFormatContext*, AVDictionary**);
    extern int (*avformat_alloc_output_context2)(AVFormatContext**, const AVOutputFormat*, const char*, const char*);
    extern void (*avformat_free_context)(AVFormatContext*);
    extern AVStream* (*avformat_new_stream)(AVFormatContext*, const AVCodec*);
    extern int (*avformat_write_header)(AVFormatContext*, AVDictionary**);
    extern int (*av_write_trailer)(AVFormatContext*);
    extern int (*av_interleaved_write_frame)(AVFormatContext*, AVPacket*);
    extern int (*av_read_frame)(AVFormatContext*, AVPacket*);
    extern int (*avio_open)(AVIOContext**, const char*, int);
    extern int (*avio_closep)(AVIOContext**);

    // avcodec functions
    extern const AVCodec* (*avcodec_find_decoder)(int);
    extern const AVCodec* (*avcodec_find_encoder)(int);
    extern const AVCodec* (*avcodec_find_encoder_by_name)(const char*);
    extern AVCodecContext* (*avcodec_alloc_context3)(const AVCodec*);
    extern void (*avcodec_free_context)(AVCodecContext**);
    extern void (*avcodec_flush_buffers)(AVCodecContext*);
    extern int (*avcodec_open2)(AVCodecContext*, const AVCodec*, AVDictionary**);
    extern int (*avcodec_parameters_to_context)(AVCodecContext*, const AVCodecParameters*);
    extern int (*avcodec_parameters_from_context)(AVCodecParameters*, const AVCodecContext*);
    extern int (*avcodec_parameters_copy)(AVCodecParameters*, const AVCodecParameters*);
    extern int (*avcodec_send_packet)(AVCodecContext*, const AVPacket*);
    extern int (*avcodec_receive_frame)(AVCodecContext*, AVFrame*);
    extern int (*avcodec_send_frame)(AVCodecContext*, const AVFrame*);
    extern int (*avcodec_receive_packet)(AVCodecContext*, AVPacket*);

    // Bitstream filter functions
    extern const AVBitStreamFilter* (*av_bsf_get_by_name)(const char*);
    extern int (*av_bsf_alloc)(const AVBitStreamFilter*, AVBSFContext**);
    extern int (*av_bsf_init)(AVBSFContext*);
    extern void (*av_bsf_free)(AVBSFContext**);
    extern int (*av_bsf_send_packet)(AVBSFContext*, AVPacket*);
    extern int (*av_bsf_receive_packet)(AVBSFContext*, AVPacket*);

    // avutil functions
    extern const char* (*av_version_info)();
    extern AVFrame* (*av_frame_alloc)();
    extern void (*av_frame_free)(AVFrame**);
    extern void (*av_frame_unref)(AVFrame*);
    extern int (*av_frame_get_buffer)(AVFrame*, int);
    extern AVPacket* (*av_packet_alloc)();
    extern void (*av_packet_free)(AVPacket**);
    extern void (*av_packet_unref)(AVPacket*);
    extern AVPacket* (*av_packet_clone)(const AVPacket*);
    extern void (*av_packet_rescale_ts)(AVPacket*, AVRational, AVRational);
    extern int64_t (*av_rescale_q)(int64_t, AVRational, AVRational);
    extern int (*av_opt_set)(void*, const char*, const char*, int);
    extern void* (*av_malloc)(size_t);
    extern void (*av_free)(void*);

    // swscale functions
    extern SwsContext* (*sws_getContext)(int, int, int, int, int, int, int, void*, void*, const double*);
    extern SwsContext* (*sws_getCachedContext)(SwsContext*, int, int, int, int, int, int, int, void*, void*, const double*);
    extern void (*sws_freeContext)(SwsContext*);
    extern int (*sws_scale)(SwsContext*, const uint8_t* const*, const int*, int, int, uint8_t* const*, const int*);

    // swresample functions
    extern SwrContext* (*swr_alloc)();
    extern int (*swr_alloc_set_opts2)(SwrContext**, const void*, int, int, const void*, int, int, int, void*);
    extern int (*swr_init)(SwrContext*);
    extern int (*swr_convert_frame)(SwrContext*, AVFrame*, const AVFrame*);
    extern void (*swr_free)(SwrContext**);
}

bool loadFFmpegLibraries(const std::string& libPath);

#endif // FFMPEG_LOADER_H

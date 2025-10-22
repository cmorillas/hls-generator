
#ifndef FFMPEG_DELETERS_H
#define FFMPEG_DELETERS_H

#include "ffmpeg_loader.h"

struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (ctx) {
            FFmpegLib::avformat_free_context(ctx);
        }
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx) {
            FFmpegLib::avcodec_free_context(&ctx);
        }
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* frame) const {
        if (frame) {
            FFmpegLib::av_frame_free(&frame);
        }
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ctx) const {
        if (ctx) {
            FFmpegLib::sws_freeContext(ctx);
        }
    }
};

struct AVBSFContextDeleter {
    void operator()(AVBSFContext* ctx) const {
        if (ctx) {
            FFmpegLib::av_bsf_free(&ctx);
        }
    }
};

#endif // FFMPEG_DELETERS_H

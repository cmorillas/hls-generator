#include "ffmpeg_deleters.h"
#include "ffmpeg_context.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavcodec/bsf.h>
}

void AVFormatContextDeleter::operator()(AVFormatContext* ctx) const {
    if (ctx && ffmpeg) {
        ffmpeg->avformat_free_context(ctx);
    }
}

void AVCodecContextDeleter::operator()(AVCodecContext* ctx) const {
    if (ctx && ffmpeg) {
        ffmpeg->avcodec_free_context(&ctx);
    }
}

void AVFrameDeleter::operator()(AVFrame* frame) const {
    if (frame && ffmpeg) {
        ffmpeg->av_frame_free(&frame);
    }
}

void SwsContextDeleter::operator()(SwsContext* ctx) const {
    if (ctx && ffmpeg) {
        ffmpeg->sws_freeContext(ctx);
    }
}

void AVBSFContextDeleter::operator()(AVBSFContext* ctx) const {
    if (ctx && ffmpeg) {
        ffmpeg->av_bsf_free(&ctx);
    }
}

void SwrContextDeleter::operator()(SwrContext* ctx) const {
    if (ctx && ffmpeg) {
        ffmpeg->swr_free(&ctx);
    }
}


#ifndef FFMPEG_DELETERS_H
#define FFMPEG_DELETERS_H

#include <memory>

// Forward declaration
class FFmpegContext;

// Forward declarations for FFmpeg types
extern "C" {
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
struct AVBSFContext;
struct SwrContext;
}

// Deleters that use FFmpegContext instead of FFmpegLib
// These are used by components that have been migrated to FFmpegContext

struct AVFormatContextDeleter {
    std::shared_ptr<FFmpegContext> ffmpeg;

    AVFormatContextDeleter() = default;
    explicit AVFormatContextDeleter(std::shared_ptr<FFmpegContext> ctx) : ffmpeg(ctx) {}

    void operator()(AVFormatContext* ctx) const;
};

struct AVCodecContextDeleter {
    std::shared_ptr<FFmpegContext> ffmpeg;

    AVCodecContextDeleter() = default;
    explicit AVCodecContextDeleter(std::shared_ptr<FFmpegContext> ctx) : ffmpeg(ctx) {}

    void operator()(AVCodecContext* ctx) const;
};

struct AVFrameDeleter {
    std::shared_ptr<FFmpegContext> ffmpeg;

    AVFrameDeleter() = default;
    explicit AVFrameDeleter(std::shared_ptr<FFmpegContext> ctx) : ffmpeg(ctx) {}

    void operator()(AVFrame* frame) const;
};

struct SwsContextDeleter {
    std::shared_ptr<FFmpegContext> ffmpeg;

    SwsContextDeleter() = default;
    explicit SwsContextDeleter(std::shared_ptr<FFmpegContext> ctx) : ffmpeg(ctx) {}

    void operator()(SwsContext* ctx) const;
};

struct AVBSFContextDeleter {
    std::shared_ptr<FFmpegContext> ffmpeg;

    AVBSFContextDeleter() = default;
    explicit AVBSFContextDeleter(std::shared_ptr<FFmpegContext> ctx) : ffmpeg(ctx) {}

    void operator()(AVBSFContext* ctx) const;
};

struct SwrContextDeleter {
    std::shared_ptr<FFmpegContext> ffmpeg;

    SwrContextDeleter() = default;
    explicit SwrContextDeleter(std::shared_ptr<FFmpegContext> ctx) : ffmpeg(ctx) {}

    void operator()(SwrContext* ctx) const;
};

#endif // FFMPEG_DELETERS_H

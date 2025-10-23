#ifndef FFMPEG_CONTEXT_H
#define FFMPEG_CONTEXT_H

#include <string>
#include <memory>
#include <cstdint>

class DynamicLibrary;

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

/**
 * FFmpegContext - Shared context for FFmpeg dynamic loading
 *
 * Centralizes FFmpeg library loading and provides shared function pointers
 * to all pipelines. Ensures single library load, no duplicates, and proper
 * cleanup on destruction.
 *
 * Lifecycle:
 *   - Constructor: Prepares for loading (doesn't load yet)
 *   - initialize(libPath): Loads all FFmpeg libraries dynamically
 *   - Destructor: Automatically unloads all libraries (dlclose)
 *
 * Thread-safety: Immutable after initialize(), safe for multiple readers
 *
 * Usage:
 *   auto ctx = std::make_shared<FFmpegContext>();
 *   if (!ctx->initialize(libPath)) { error }
 *   // Share ctx with pipelines via shared_ptr
 */
class FFmpegContext {
public:
    FFmpegContext();
    ~FFmpegContext();

    // No copy, movable
    FFmpegContext(const FFmpegContext&) = delete;
    FFmpegContext& operator=(const FFmpegContext&) = delete;
    FFmpegContext(FFmpegContext&&) = default;
    FFmpegContext& operator=(FFmpegContext&&) = default;

    /**
     * Load all FFmpeg libraries from the specified path
     * @param libPath Path to directory containing FFmpeg libraries
     * @return true on success, false on failure
     */
    bool initialize(const std::string& libPath);

    /**
     * Check if libraries are loaded
     */
    bool isInitialized() const { return initialized_; }

    // ===== avformat functions =====
    int (*avformat_open_input)(AVFormatContext**, const char*, const AVOutputFormat*, AVDictionary**) = nullptr;
    void (*avformat_close_input)(AVFormatContext**) = nullptr;
    int (*avformat_find_stream_info)(AVFormatContext*, AVDictionary**) = nullptr;
    int (*avformat_alloc_output_context2)(AVFormatContext**, const AVOutputFormat*, const char*, const char*) = nullptr;
    void (*avformat_free_context)(AVFormatContext*) = nullptr;
    AVStream* (*avformat_new_stream)(AVFormatContext*, const AVCodec*) = nullptr;
    int (*avformat_write_header)(AVFormatContext*, AVDictionary**) = nullptr;
    int (*av_write_trailer)(AVFormatContext*) = nullptr;
    int (*av_interleaved_write_frame)(AVFormatContext*, AVPacket*) = nullptr;
    int (*av_read_frame)(AVFormatContext*, AVPacket*) = nullptr;
    int (*avio_open)(AVIOContext**, const char*, int) = nullptr;
    int (*avio_closep)(AVIOContext**) = nullptr;

    // ===== avcodec functions =====
    const AVCodec* (*avcodec_find_decoder)(int) = nullptr;
    const AVCodec* (*avcodec_find_encoder)(int) = nullptr;
    const AVCodec* (*avcodec_find_encoder_by_name)(const char*) = nullptr;
    AVCodecContext* (*avcodec_alloc_context3)(const AVCodec*) = nullptr;
    void (*avcodec_free_context)(AVCodecContext**) = nullptr;
    void (*avcodec_flush_buffers)(AVCodecContext*) = nullptr;
    int (*avcodec_open2)(AVCodecContext*, const AVCodec*, AVDictionary**) = nullptr;
    int (*avcodec_parameters_to_context)(AVCodecContext*, const AVCodecParameters*) = nullptr;
    int (*avcodec_parameters_from_context)(AVCodecParameters*, const AVCodecContext*) = nullptr;
    int (*avcodec_parameters_copy)(AVCodecParameters*, const AVCodecParameters*) = nullptr;
    int (*avcodec_send_packet)(AVCodecContext*, const AVPacket*) = nullptr;
    int (*avcodec_receive_frame)(AVCodecContext*, AVFrame*) = nullptr;
    int (*avcodec_send_frame)(AVCodecContext*, const AVFrame*) = nullptr;
    int (*avcodec_receive_packet)(AVCodecContext*, AVPacket*) = nullptr;

    // ===== Bitstream filter functions =====
    const AVBitStreamFilter* (*av_bsf_get_by_name)(const char*) = nullptr;
    int (*av_bsf_alloc)(const AVBitStreamFilter*, AVBSFContext**) = nullptr;
    int (*av_bsf_init)(AVBSFContext*) = nullptr;
    void (*av_bsf_free)(AVBSFContext**) = nullptr;
    int (*av_bsf_send_packet)(AVBSFContext*, AVPacket*) = nullptr;
    int (*av_bsf_receive_packet)(AVBSFContext*, AVPacket*) = nullptr;

    // ===== avutil functions =====
    const char* (*av_version_info)() = nullptr;
    AVFrame* (*av_frame_alloc)() = nullptr;
    void (*av_frame_free)(AVFrame**) = nullptr;
    void (*av_frame_unref)(AVFrame*) = nullptr;
    int (*av_frame_get_buffer)(AVFrame*, int) = nullptr;
    AVPacket* (*av_packet_alloc)() = nullptr;
    void (*av_packet_free)(AVPacket**) = nullptr;
    void (*av_packet_unref)(AVPacket*) = nullptr;
    AVPacket* (*av_packet_clone)(const AVPacket*) = nullptr;
    void (*av_packet_rescale_ts)(AVPacket*, AVRational, AVRational) = nullptr;
    int64_t (*av_rescale_q)(int64_t, AVRational, AVRational) = nullptr;
    int (*av_opt_set)(void*, const char*, const char*, int) = nullptr;
    void* (*av_malloc)(size_t) = nullptr;
    void (*av_free)(void*) = nullptr;

    // ===== swscale functions =====
    SwsContext* (*sws_getContext)(int, int, int, int, int, int, int, void*, void*, const double*) = nullptr;
    SwsContext* (*sws_getCachedContext)(SwsContext*, int, int, int, int, int, int, int, void*, void*, const double*) = nullptr;
    void (*sws_freeContext)(SwsContext*) = nullptr;
    int (*sws_scale)(SwsContext*, const uint8_t* const*, const int*, int, int, uint8_t* const*, const int*) = nullptr;

    // ===== swresample functions =====
    SwrContext* (*swr_alloc)() = nullptr;
    int (*swr_alloc_set_opts2)(SwrContext**, const void*, int, int, const void*, int, int, int, void*) = nullptr;
    int (*swr_init)(SwrContext*) = nullptr;
    int (*swr_convert_frame)(SwrContext*, AVFrame*, const AVFrame*) = nullptr;
    void (*swr_free)(SwrContext**) = nullptr;

private:
    bool initialized_ = false;

    // Library handles (owned by FFmpegContext, destroyed in destructor)
    std::unique_ptr<DynamicLibrary> avformatLib_;
    std::unique_ptr<DynamicLibrary> avcodecLib_;
    std::unique_ptr<DynamicLibrary> avutilLib_;
    std::unique_ptr<DynamicLibrary> swscaleLib_;
    std::unique_ptr<DynamicLibrary> swresampleLib_;

    // Helper methods
    std::unique_ptr<DynamicLibrary> tryLoadLibrary(const std::string& libPath, const std::string& baseName);
    bool loadSymbols();
};

#endif // FFMPEG_CONTEXT_H

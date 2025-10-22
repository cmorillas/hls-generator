
#include "ffmpeg_loader.h"
#include "dynamic_library.h"
#include "logger.h"

#include <memory>

namespace FFmpegLib {
    // avformat functions
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

    // avcodec functions
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

    // Bitstream filter functions
    const AVBitStreamFilter* (*av_bsf_get_by_name)(const char*) = nullptr;
    int (*av_bsf_alloc)(const AVBitStreamFilter*, AVBSFContext**) = nullptr;
    int (*av_bsf_init)(AVBSFContext*) = nullptr;
    void (*av_bsf_free)(AVBSFContext**) = nullptr;
    int (*av_bsf_send_packet)(AVBSFContext*, AVPacket*) = nullptr;
    int (*av_bsf_receive_packet)(AVBSFContext*, AVPacket*) = nullptr;

    // avutil functions
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
    int (*av_opt_set)(void*, const char*, const char*, int) = nullptr;
    void* (*av_malloc)(size_t) = nullptr;
    void (*av_free)(void*) = nullptr;

    // swscale functions
    SwsContext* (*sws_getContext)(int, int, int, int, int, int, int, void*, void*, const double*) = nullptr;
    void (*sws_freeContext)(SwsContext*) = nullptr;
    int (*sws_scale)(SwsContext*, const uint8_t* const*, const int*, int, int, uint8_t* const*, const int*) = nullptr;
}

bool loadFFmpegLibraries(const std::string& libPath) {
    static std::unique_ptr<DynamicLibrary> avformat_lib;
    static std::unique_ptr<DynamicLibrary> avcodec_lib;
    static std::unique_ptr<DynamicLibrary> avutil_lib;
    static std::unique_ptr<DynamicLibrary> swscale_lib;

#ifdef PLATFORM_WINDOWS
    SetDllDirectoryA(libPath.c_str());
    avformat_lib = std::make_unique<DynamicLibrary>(libPath + "\\avformat-61.dll");
    avcodec_lib = std::make_unique<DynamicLibrary>(libPath + "\\avcodec-61.dll");
    avutil_lib = std::make_unique<DynamicLibrary>(libPath + "\\avutil-59.dll");
    swscale_lib = std::make_unique<DynamicLibrary>(libPath + "\\swscale-8.dll");
#else
    avformat_lib = std::make_unique<DynamicLibrary>(libPath + "/libavformat.so.61");
    avcodec_lib = std::make_unique<DynamicLibrary>(libPath + "/libavcodec.so.61");
    avutil_lib = std::make_unique<DynamicLibrary>(libPath + "/libavutil.so.59");
    swscale_lib = std::make_unique<DynamicLibrary>(libPath + "/libswscale.so.8");
#endif

    if (!avformat_lib->load() || !avcodec_lib->load() || !avutil_lib->load() || !swscale_lib->load()) {
        return false;
    }

#define LOAD_FUNC(lib, name) \
    FFmpegLib::name = lib->getFunction<decltype(FFmpegLib::name)>(#name); \
    if (!FFmpegLib::name) { Logger::error("Failed to load function: " #name); return false; }

    LOAD_FUNC(avformat_lib, avformat_open_input);
    LOAD_FUNC(avformat_lib, avformat_close_input);
    LOAD_FUNC(avformat_lib, avformat_find_stream_info);
    LOAD_FUNC(avformat_lib, avformat_alloc_output_context2);
    LOAD_FUNC(avformat_lib, avformat_free_context);
    LOAD_FUNC(avformat_lib, avformat_new_stream);
    LOAD_FUNC(avformat_lib, avformat_write_header);
    LOAD_FUNC(avformat_lib, av_write_trailer);
    LOAD_FUNC(avformat_lib, av_interleaved_write_frame);
    LOAD_FUNC(avformat_lib, av_read_frame);
    LOAD_FUNC(avformat_lib, avio_open);
    LOAD_FUNC(avformat_lib, avio_closep);

    LOAD_FUNC(avcodec_lib, avcodec_find_decoder);
    LOAD_FUNC(avcodec_lib, avcodec_find_encoder);
    LOAD_FUNC(avcodec_lib, avcodec_find_encoder_by_name);
    LOAD_FUNC(avcodec_lib, avcodec_alloc_context3);
    LOAD_FUNC(avcodec_lib, avcodec_free_context);
    LOAD_FUNC(avcodec_lib, avcodec_flush_buffers);
    LOAD_FUNC(avcodec_lib, avcodec_open2);
    LOAD_FUNC(avcodec_lib, avcodec_parameters_to_context);
    LOAD_FUNC(avcodec_lib, avcodec_parameters_from_context);
    LOAD_FUNC(avcodec_lib, avcodec_parameters_copy);
    LOAD_FUNC(avcodec_lib, avcodec_send_packet);
    LOAD_FUNC(avcodec_lib, avcodec_receive_frame);
    LOAD_FUNC(avcodec_lib, avcodec_send_frame);
    LOAD_FUNC(avcodec_lib, avcodec_receive_packet);
    LOAD_FUNC(avcodec_lib, av_bsf_get_by_name);
    LOAD_FUNC(avcodec_lib, av_bsf_alloc);
    LOAD_FUNC(avcodec_lib, av_bsf_init);
    LOAD_FUNC(avcodec_lib, av_bsf_free);
    LOAD_FUNC(avcodec_lib, av_bsf_send_packet);
    LOAD_FUNC(avcodec_lib, av_bsf_receive_packet);
    LOAD_FUNC(avcodec_lib, av_packet_alloc);
    LOAD_FUNC(avcodec_lib, av_packet_free);
    LOAD_FUNC(avcodec_lib, av_packet_unref);
    LOAD_FUNC(avcodec_lib, av_packet_clone);
    LOAD_FUNC(avcodec_lib, av_packet_rescale_ts);

    LOAD_FUNC(avutil_lib, av_version_info);
    LOAD_FUNC(avutil_lib, av_frame_alloc);
    LOAD_FUNC(avutil_lib, av_frame_free);
    LOAD_FUNC(avutil_lib, av_frame_unref);
    LOAD_FUNC(avutil_lib, av_frame_get_buffer);
    LOAD_FUNC(avutil_lib, av_opt_set);
    LOAD_FUNC(avutil_lib, av_malloc);
    LOAD_FUNC(avutil_lib, av_free);

    LOAD_FUNC(swscale_lib, sws_getContext);
    LOAD_FUNC(swscale_lib, sws_freeContext);
    LOAD_FUNC(swscale_lib, sws_scale);

#undef LOAD_FUNC

    Logger::info("FFmpeg libraries loaded successfully");
    return true;
}

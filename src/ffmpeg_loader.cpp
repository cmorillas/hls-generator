
#include "ffmpeg_loader.h"
#include "dynamic_library.h"
#include "logger.h"

#include <memory>
#include <vector>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#endif

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
    int64_t (*av_rescale_q)(int64_t, AVRational, AVRational) = nullptr;
    int (*av_opt_set)(void*, const char*, const char*, int) = nullptr;
    void* (*av_malloc)(size_t) = nullptr;
    void (*av_free)(void*) = nullptr;

    // swscale functions
    SwsContext* (*sws_getContext)(int, int, int, int, int, int, int, void*, void*, const double*) = nullptr;
    SwsContext* (*sws_getCachedContext)(SwsContext*, int, int, int, int, int, int, int, void*, void*, const double*) = nullptr;
    void (*sws_freeContext)(SwsContext*) = nullptr;
    int (*sws_scale)(SwsContext*, const uint8_t* const*, const int*, int, int, uint8_t* const*, const int*) = nullptr;

    // swresample functions
    SwrContext* (*swr_alloc)() = nullptr;
    int (*swr_alloc_set_opts2)(SwrContext**, const void*, int, int, const void*, int, int, int, void*) = nullptr;
    int (*swr_init)(SwrContext*) = nullptr;
    int (*swr_convert_frame)(SwrContext*, AVFrame*, const AVFrame*) = nullptr;
    void (*swr_free)(SwrContext**) = nullptr;
}

// Helper: Check if file exists
static bool fileExists(const std::string& path) {
#ifdef PLATFORM_WINDOWS
    DWORD attrib = GetFileAttributesA(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
#endif
}

// Helper: Try to load a library with dynamic version scanning
static std::unique_ptr<DynamicLibrary> tryLoadLibrary(const std::string& libPath, const std::string& baseName) {
    std::vector<std::string> candidates;

#ifdef PLATFORM_WINDOWS
    // Windows: Try patterns like avformat-61.dll, avformat-60.dll, avformat.dll
    const std::vector<int> knownVersions = {62, 61, 60, 59};

    // First, try unversioned (e.g., avformat.dll)
    candidates.push_back(libPath + "\\" + baseName + ".dll");

    // Then try known versions
    for (int ver : knownVersions) {
        candidates.push_back(libPath + "\\" + baseName + "-" + std::to_string(ver) + ".dll");
    }

    // Finally, scan directory for any version
    WIN32_FIND_DATAA findData;
    std::string pattern = libPath + "\\" + baseName + "-*.dll";
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string fileName = findData.cFileName;
            std::string fullPath = libPath + "\\" + fileName;
            // Avoid duplicates
            if (std::find(candidates.begin(), candidates.end(), fullPath) == candidates.end()) {
                candidates.push_back(fullPath);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    // Linux: Try patterns like libavformat.so.61, libavformat.so
    const std::vector<int> knownVersions = {62, 61, 60, 59};

    // First, try unversioned (e.g., libavformat.so)
    candidates.push_back(libPath + "/lib" + baseName + ".so");

    // Then try known versions
    for (int ver : knownVersions) {
        candidates.push_back(libPath + "/lib" + baseName + ".so." + std::to_string(ver));
    }

    // Finally, scan directory for any version
    DIR* d = opendir(libPath.c_str());
    if (d) {
        struct dirent* entry;
        std::string prefix = "lib" + baseName + ".so.";
        while ((entry = readdir(d)) != nullptr) {
            std::string name = entry->d_name;
            if (name.find(prefix) == 0) {
                std::string fullPath = libPath + "/" + name;
                // Avoid duplicates
                if (std::find(candidates.begin(), candidates.end(), fullPath) == candidates.end()) {
                    candidates.push_back(fullPath);
                }
            }
        }
        closedir(d);
    }
#endif

    // Try each candidate in order
    for (const auto& candidate : candidates) {
        if (fileExists(candidate)) {
            Logger::info("Trying to load: " + candidate);
            auto lib = std::make_unique<DynamicLibrary>(candidate);
            if (lib->load()) {
                Logger::info("Successfully loaded: " + candidate);
                return lib;
            } else {
                Logger::warn("Failed to load: " + candidate);
            }
        }
    }

    Logger::error("Could not find any valid library for: " + baseName);
    return nullptr;
}

bool loadFFmpegLibraries(const std::string& libPath) {
    static std::unique_ptr<DynamicLibrary> avformat_lib;
    static std::unique_ptr<DynamicLibrary> avcodec_lib;
    static std::unique_ptr<DynamicLibrary> avutil_lib;
    static std::unique_ptr<DynamicLibrary> swscale_lib;
    static std::unique_ptr<DynamicLibrary> swresample_lib;

#ifdef PLATFORM_WINDOWS
    SetDllDirectoryA(libPath.c_str());
#endif

    // Try to load each library with dynamic version detection
    Logger::info("Loading FFmpeg libraries from: " + libPath);

    avformat_lib = tryLoadLibrary(libPath, "avformat");
    if (!avformat_lib) {
        Logger::error("Failed to load avformat library");
        return false;
    }

    avcodec_lib = tryLoadLibrary(libPath, "avcodec");
    if (!avcodec_lib) {
        Logger::error("Failed to load avcodec library");
        return false;
    }

    avutil_lib = tryLoadLibrary(libPath, "avutil");
    if (!avutil_lib) {
        Logger::error("Failed to load avutil library");
        return false;
    }

    swscale_lib = tryLoadLibrary(libPath, "swscale");
    if (!swscale_lib) {
        Logger::error("Failed to load swscale library");
        return false;
    }

    swresample_lib = tryLoadLibrary(libPath, "swresample");
    if (!swresample_lib) {
        Logger::error("Failed to load swresample library");
        return false;
    }

    Logger::info("All FFmpeg libraries loaded successfully");

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
    LOAD_FUNC(avutil_lib, av_rescale_q);
    LOAD_FUNC(avutil_lib, av_opt_set);
    LOAD_FUNC(avutil_lib, av_malloc);
    LOAD_FUNC(avutil_lib, av_free);

    LOAD_FUNC(swscale_lib, sws_getContext);
    LOAD_FUNC(swscale_lib, sws_getCachedContext);
    LOAD_FUNC(swscale_lib, sws_freeContext);
    LOAD_FUNC(swscale_lib, sws_scale);

    LOAD_FUNC(swresample_lib, swr_alloc);
    LOAD_FUNC(swresample_lib, swr_alloc_set_opts2);
    LOAD_FUNC(swresample_lib, swr_init);
    LOAD_FUNC(swresample_lib, swr_convert_frame);
    LOAD_FUNC(swresample_lib, swr_free);

#undef LOAD_FUNC

    Logger::info("FFmpeg libraries loaded successfully");
    return true;
}

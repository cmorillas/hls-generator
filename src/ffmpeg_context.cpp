#include "ffmpeg_context.h"
#include "dynamic_library.h"
#include "logger.h"

#include <vector>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#endif

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

FFmpegContext::FFmpegContext() = default;

FFmpegContext::~FFmpegContext() {
    // Cleanup is automatic via unique_ptr destructors
    // Libraries will be unloaded (dlclose) when unique_ptrs are destroyed
    if (initialized_) {
        Logger::info("FFmpegContext destroyed - unloading FFmpeg libraries");
    }
}

std::unique_ptr<DynamicLibrary> FFmpegContext::tryLoadLibrary(const std::string& libPath, const std::string& baseName) {
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

bool FFmpegContext::loadSymbols() {
#define LOAD_FUNC(lib, name) \
    name = lib->getFunction<decltype(name)>(#name); \
    if (!name) { Logger::error("Failed to load function: " #name); return false; }

    // avformat functions
    LOAD_FUNC(avformatLib_, avformat_open_input);
    LOAD_FUNC(avformatLib_, avformat_close_input);
    LOAD_FUNC(avformatLib_, avformat_find_stream_info);
    LOAD_FUNC(avformatLib_, avformat_alloc_output_context2);
    LOAD_FUNC(avformatLib_, avformat_free_context);
    LOAD_FUNC(avformatLib_, avformat_new_stream);
    LOAD_FUNC(avformatLib_, avformat_write_header);
    LOAD_FUNC(avformatLib_, av_write_trailer);
    LOAD_FUNC(avformatLib_, av_interleaved_write_frame);
    LOAD_FUNC(avformatLib_, av_read_frame);
    LOAD_FUNC(avformatLib_, avio_open);
    LOAD_FUNC(avformatLib_, avio_closep);

    // avcodec functions
    LOAD_FUNC(avcodecLib_, avcodec_find_decoder);
    LOAD_FUNC(avcodecLib_, avcodec_find_encoder);
    LOAD_FUNC(avcodecLib_, avcodec_find_encoder_by_name);
    LOAD_FUNC(avcodecLib_, avcodec_alloc_context3);
    LOAD_FUNC(avcodecLib_, avcodec_free_context);
    LOAD_FUNC(avcodecLib_, avcodec_flush_buffers);
    LOAD_FUNC(avcodecLib_, avcodec_open2);
    LOAD_FUNC(avcodecLib_, avcodec_parameters_to_context);
    LOAD_FUNC(avcodecLib_, avcodec_parameters_from_context);
    LOAD_FUNC(avcodecLib_, avcodec_parameters_copy);
    LOAD_FUNC(avcodecLib_, avcodec_send_packet);
    LOAD_FUNC(avcodecLib_, avcodec_receive_frame);
    LOAD_FUNC(avcodecLib_, avcodec_send_frame);
    LOAD_FUNC(avcodecLib_, avcodec_receive_packet);
    LOAD_FUNC(avcodecLib_, av_bsf_get_by_name);
    LOAD_FUNC(avcodecLib_, av_bsf_alloc);
    LOAD_FUNC(avcodecLib_, av_bsf_init);
    LOAD_FUNC(avcodecLib_, av_bsf_free);
    LOAD_FUNC(avcodecLib_, av_bsf_send_packet);
    LOAD_FUNC(avcodecLib_, av_bsf_receive_packet);
    LOAD_FUNC(avcodecLib_, av_packet_alloc);
    LOAD_FUNC(avcodecLib_, av_packet_free);
    LOAD_FUNC(avcodecLib_, av_packet_unref);
    LOAD_FUNC(avcodecLib_, av_packet_clone);
    LOAD_FUNC(avcodecLib_, av_packet_rescale_ts);

    // avutil functions
    LOAD_FUNC(avutilLib_, av_version_info);
    LOAD_FUNC(avutilLib_, av_frame_alloc);
    LOAD_FUNC(avutilLib_, av_frame_free);
    LOAD_FUNC(avutilLib_, av_frame_unref);
    LOAD_FUNC(avutilLib_, av_frame_get_buffer);
    LOAD_FUNC(avutilLib_, av_rescale_q);
    LOAD_FUNC(avutilLib_, av_opt_set);
    LOAD_FUNC(avutilLib_, av_malloc);
    LOAD_FUNC(avutilLib_, av_free);

    // swscale functions
    LOAD_FUNC(swscaleLib_, sws_getContext);
    LOAD_FUNC(swscaleLib_, sws_getCachedContext);
    LOAD_FUNC(swscaleLib_, sws_freeContext);
    LOAD_FUNC(swscaleLib_, sws_scale);

    // swresample functions
    LOAD_FUNC(swresampleLib_, swr_alloc);
    LOAD_FUNC(swresampleLib_, swr_alloc_set_opts2);
    LOAD_FUNC(swresampleLib_, swr_init);
    LOAD_FUNC(swresampleLib_, swr_convert_frame);
    LOAD_FUNC(swresampleLib_, swr_free);

#undef LOAD_FUNC

    return true;
}

bool FFmpegContext::initialize(const std::string& libPath) {
    if (initialized_) {
        Logger::warn("FFmpegContext already initialized");
        return true;
    }

#ifdef PLATFORM_WINDOWS
    SetDllDirectoryA(libPath.c_str());
#endif

    Logger::info("Initializing FFmpegContext - loading FFmpeg libraries from: " + libPath);

    // Try to load each library with dynamic version detection
    avformatLib_ = tryLoadLibrary(libPath, "avformat");
    if (!avformatLib_) {
        Logger::error("Failed to load avformat library");
        return false;
    }

    avcodecLib_ = tryLoadLibrary(libPath, "avcodec");
    if (!avcodecLib_) {
        Logger::error("Failed to load avcodec library");
        return false;
    }

    avutilLib_ = tryLoadLibrary(libPath, "avutil");
    if (!avutilLib_) {
        Logger::error("Failed to load avutil library");
        return false;
    }

    swscaleLib_ = tryLoadLibrary(libPath, "swscale");
    if (!swscaleLib_) {
        Logger::error("Failed to load swscale library");
        return false;
    }

    swresampleLib_ = tryLoadLibrary(libPath, "swresample");
    if (!swresampleLib_) {
        Logger::error("Failed to load swresample library");
        return false;
    }

    Logger::info("All FFmpeg libraries loaded successfully");

    // Load all function symbols
    if (!loadSymbols()) {
        Logger::error("Failed to load FFmpeg function symbols");
        return false;
    }

    initialized_ = true;
    Logger::info("FFmpegContext initialized successfully");
    return true;
}

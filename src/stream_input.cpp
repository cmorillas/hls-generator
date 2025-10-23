
#include "stream_input.h"
#include "ffmpeg_input.h"
#include "browser_input.h"
#include "logger.h"

#include <algorithm>
#include <cctype>

std::string StreamInputFactory::detectInputType(const std::string& uri) {
    std::string lowerUri = uri;
    std::transform(lowerUri.begin(), lowerUri.end(), lowerUri.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lowerUri.find("srt://") == 0) {
        return "srt";
    }
    if (lowerUri.find("rtmp://") == 0 || lowerUri.find("rtmps://") == 0) {
        return "rtmp";
    }
    if (lowerUri.find("ndi://") == 0) {
        return "ndi";
    }
    if (lowerUri.find("rtsp://") == 0 || lowerUri.find("rtsps://") == 0) {
        return "rtsp";
    }
    if (lowerUri.find("udp://") == 0) {
        return "udp";
    }
    if (lowerUri.find("http://") == 0 || lowerUri.find("https://") == 0) {
        return "browser";
    }

    return "file";
}

std::unique_ptr<StreamInput> StreamInputFactory::create(const std::string& uri, const AppConfig& config, std::shared_ptr<FFmpegContext> ffmpegCtx) {
    std::string inputType = detectInputType(uri);

    Logger::info("Detected input type: " + inputType);

    if (inputType == "browser") {
        return std::make_unique<BrowserInput>(config, ffmpegCtx);
    } else if (inputType == "file" || inputType == "srt" || inputType == "rtmp" || inputType == "ndi" || inputType == "rtsp" || inputType == "udp") {
        return std::make_unique<FFmpegInput>(inputType, ffmpegCtx);
    } else {
        Logger::error("Unsupported input type: " + inputType);
        return nullptr;
    }
}


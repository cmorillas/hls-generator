#include <iostream>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <atomic>
#include "obs_detector.h"
#include "logger.h"
#include "hls_generator.h"
#include "stream_input.h"
#include "config.h"

// Note: CEF subprocess handling is done by OBS's obs-browser-page
// We don't need CEF includes or subprocess handling in main.cpp

// Global flag for signal handling
static std::atomic<bool> g_interrupted(false);

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    Logger::info("");
    Logger::info("Interrupt signal received (Ctrl+C), shutting down gracefully...");
    g_interrupted.store(true);
}

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <input_source> <output_directory>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --no-js           Disable JavaScript injection (no cookie auto-accept)" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  input_source      Video file path or stream URI" << std::endl;
    std::cout << "  output_directory  Directory where HLS files will be generated" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported input sources:" << std::endl;
    std::cout << "  Files:      video.mp4, video.mkv, video.webm, etc." << std::endl;
    std::cout << "  SRT:        srt://host:port" << std::endl;
    std::cout << "  RTMP:       rtmp://server/app/stream" << std::endl;
    std::cout << "  NDI:        ndi://source_name" << std::endl;
    std::cout << "  RTSP:       rtsp://camera_ip/stream" << std::endl;
    std::cout << "  Browser:    http://url or https://url (OBS CEF browser)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << progName << " video.mp4 /path/to/output" << std::endl;
    std::cout << "  " << progName << " srt://192.168.1.100:9000 /path/to/output" << std::endl;
    std::cout << "  " << progName << " rtmp://live.twitch.tv/app/stream_key /path/to/output" << std::endl;
    std::cout << "  " << progName << " https://example.com /path/to/output" << std::endl;
    std::cout << "  " << progName << " --no-js https://example.com /path/to/output" << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool enable_js_injection = true;  // Enabled by default
    int arg_offset = 1;

    // Check for --no-js flag
    if (argc > 1 && strcmp(argv[1], "--no-js") == 0) {
        enable_js_injection = false;
        arg_offset = 2;  // Skip the flag
    }

    // Validate remaining arguments
    if (argc - arg_offset + 1 != 3) {
        printUsage(argv[0]);
        return 1;
    }

    AppConfig config;
    config.hls.inputFile = argv[arg_offset];
    config.hls.outputDir = argv[arg_offset + 1];
    config.browser.enableJsInjection = enable_js_injection;

    Logger::info("=== HLS Generator ===");
    Logger::info("Input: " + config.hls.inputFile);
    Logger::info("Output: " + config.hls.outputDir);
    Logger::info("");

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    OBSPaths obsPaths = OBSDetector::detect();

    if (!obsPaths.found) {
        Logger::error("Neither OBS Studio nor FFmpeg found in the system");
        Logger::error("Please install OBS Studio (recommended) or FFmpeg");
        return 1;
    }

    Logger::info("FFmpeg libraries detected from: " + obsPaths.source);
    Logger::info("");

    HLSGenerator generator(config);

    if (!generator.initialize(obsPaths.ffmpegLibDir)) {
        Logger::error("Failed to initialize HLS generator");
        return 1;
    }

    generator.setInterruptCallback([]() -> bool {
        return g_interrupted.load();
    });

    if (!generator.generate()) {
        if (g_interrupted.load()) {
            Logger::info("Stream interrupted by user");
        } else {
            Logger::error("Failed to generate HLS stream");
            return 1;
        }
    }

    Logger::info("");
    Logger::info("Process completed successfully");

    return 0;
}

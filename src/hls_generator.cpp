#include "hls_generator.h"
#include "ffmpeg_wrapper.h"
#include "logger.h"
#include <thread>
#include <atomic>

HLSGenerator::HLSGenerator(const AppConfig& config)
    : config_(config), ffmpegWrapper_(std::make_unique<FFmpegWrapper>(config)) {
}

HLSGenerator::~HLSGenerator() = default;

bool HLSGenerator::initialize(const std::string& ffmpegLibPath) {
    Logger::info("Initializing HLS generator...");
    Logger::info("  Input: " + config_.hls.inputFile);
    Logger::info("  Output: " + config_.hls.outputDir);
    Logger::info("  FFmpeg libs: " + ffmpegLibPath);

    // Parallel initialization: FFmpeg libs + setupOutput, then openInput
    std::atomic<bool> ffmpegReady(false);
    std::atomic<bool> inputOpenSuccess(false);

    // Thread 1: Load FFmpeg libraries + setup output (fast, ~1-2s)
    std::thread ffmpegThread([&]() {
        Logger::info(">>> Starting FFmpeg initialization in parallel...");

        if (!ffmpegWrapper_->loadLibraries(ffmpegLibPath)) {
            Logger::error("Failed to load FFmpeg libraries");
            ffmpegReady = true; // Signal completion (even on failure)
            return;
        }

        if (!ffmpegWrapper_->setupOutput()) {
            Logger::error("Failed to setup HLS output");
            ffmpegReady = true; // Signal completion (even on failure)
            return;
        }

        Logger::info(">>> FFmpeg setup complete");
        ffmpegReady = true; // Signal FFmpeg is ready
    });

    // Thread 2: Wait for FFmpeg, then open CEF input (slow, ~10-15s)
    std::thread cefThread([&]() {
        Logger::info(">>> Waiting for FFmpeg to be ready before starting CEF...");

        // Wait for FFmpeg to complete
        while (!ffmpegReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        Logger::info(">>> Starting CEF initialization...");
        inputOpenSuccess = ffmpegWrapper_->openInput(config_.hls.inputFile);
        Logger::info(">>> CEF initialization completed");
    });

    // Wait for both threads to complete
    ffmpegThread.join();
    cefThread.join();

    if (!inputOpenSuccess) {
        Logger::error("Failed to open input file");
        return false;
    }

    Logger::info(">>> Parallel initialization complete");
    initialized_ = true;
    return true;
}

bool HLSGenerator::generate() {
    if (!initialized_ || !ffmpegWrapper_) {
        Logger::error("Generator not initialized");
        return false;
    }

    if (interruptCallback_) {
        ffmpegWrapper_->setInterruptCallback(interruptCallback_);
    }

    if (!ffmpegWrapper_->processVideo()) {
        Logger::error("Failed to process video");
        return false;
    }

    Logger::info("HLS generated successfully");
    return true;
}

void HLSGenerator::setInterruptCallback(std::function<bool()> callback) {
    interruptCallback_ = callback;
}

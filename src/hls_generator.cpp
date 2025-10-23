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

    // Parallel initialization: CEF in thread, FFmpeg in main
    std::atomic<bool> inputOpenSuccess(false);
    std::atomic<bool> inputOpenComplete(false);

    std::thread cefThread([&]() {
        Logger::info(">>> Starting CEF initialization in parallel...");
        inputOpenSuccess = ffmpegWrapper_->openInput(config_.hls.inputFile);
        inputOpenComplete = true;
        Logger::info(">>> CEF initialization thread completed");
    });

    // Meanwhile, initialize FFmpeg in main thread
    Logger::info(">>> Starting FFmpeg initialization in parallel...");

    if (!ffmpegWrapper_->loadLibraries(ffmpegLibPath)) {
        Logger::error("Failed to load FFmpeg libraries");
        cefThread.join(); // Wait for CEF thread before returning
        return false;
    }

    if (!ffmpegWrapper_->setupOutput()) {
        Logger::error("Failed to setup HLS output");
        cefThread.join(); // Wait for CEF thread before returning
        return false;
    }

    Logger::info(">>> FFmpeg setup complete, waiting for CEF...");

    // Wait for CEF initialization to complete
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

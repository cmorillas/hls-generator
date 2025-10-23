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

    // STEP 1: Load FFmpeg libraries (required by both openInput and setupOutput)
    if (!ffmpegWrapper_->loadLibraries(ffmpegLibPath)) {
        Logger::error("Failed to load FFmpeg libraries");
        return false;
    }

    // STEP 2: Parallelize setupOutput() in background thread
    // NOTE: openInput() with CEF MUST stay on main thread (CEF requires UI thread)
    std::atomic<bool> outputSetupSuccess(false);
    std::atomic<bool> outputThreadException(false);

    // Background thread: Setup output (fast, ~100ms)
    std::thread outputThread([&]() {
        try {
            Logger::info(">>> Setting up HLS output in background...");
            outputSetupSuccess = ffmpegWrapper_->setupOutput();
            Logger::info(">>> HLS output setup completed");
        } catch (const std::exception& e) {
            Logger::error("Exception in setupOutput thread: " + std::string(e.what()));
            outputThreadException = true;
        } catch (...) {
            Logger::error("Unknown exception in setupOutput thread");
            outputThreadException = true;
        }
    });

    // STEP 3: Open input on MAIN thread (CEF requires this)
    Logger::info(">>> Opening input on main thread (CEF requirement)...");
    bool inputOpenSuccess = ffmpegWrapper_->openInput(config_.hls.inputFile);
    Logger::info(">>> Input open completed");

    // STEP 4: Wait for background thread to finish
    outputThread.join();

    // STEP 5: Check results
    if (outputThreadException) {
        Logger::error("Exception occurred in setupOutput thread");
        return false;
    }

    if (!outputSetupSuccess) {
        Logger::error("Failed to setup HLS output");
        return false;
    }

    if (!inputOpenSuccess) {
        Logger::error("Failed to open input file");
        return false;
    }

    Logger::info(">>> Initialization successful (setupOutput parallelized)");
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

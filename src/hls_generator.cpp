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

    // STEP 2: Parallelize openInput() and setupOutput() (they are independent)
    std::atomic<bool> outputSetupSuccess(false);
    std::atomic<bool> inputOpenSuccess(false);
    std::atomic<bool> outputThreadException(false);
    std::atomic<bool> inputThreadException(false);

    // Thread 1: Setup output (fast, ~100ms)
    std::thread outputThread([&]() {
        try {
            Logger::info(">>> Setting up HLS output in parallel...");
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

    // Thread 2: Open input (slow, ~10-15s with CEF)
    std::thread inputThread([&]() {
        try {
            Logger::info(">>> Opening input in parallel...");
            inputOpenSuccess = ffmpegWrapper_->openInput(config_.hls.inputFile);
            Logger::info(">>> Input open completed");
        } catch (const std::exception& e) {
            Logger::error("Exception in openInput thread: " + std::string(e.what()));
            inputThreadException = true;
        } catch (...) {
            Logger::error("Unknown exception in openInput thread");
            inputThreadException = true;
        }
    });

    // STEP 3: Wait for both threads
    outputThread.join();
    inputThread.join();

    // STEP 4: Check results
    if (outputThreadException) {
        Logger::error("Exception occurred in setupOutput thread");
        return false;
    }

    if (inputThreadException) {
        Logger::error("Exception occurred in openInput thread");
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

    Logger::info(">>> Parallel initialization successful");
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

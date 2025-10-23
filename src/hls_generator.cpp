#include "hls_generator.h"
#include "ffmpeg_wrapper.h"
#include "logger.h"

HLSGenerator::HLSGenerator(const AppConfig& config)
    : config_(config), ffmpegWrapper_(std::make_unique<FFmpegWrapper>(config)) {
}

HLSGenerator::~HLSGenerator() = default;

bool HLSGenerator::initialize(const std::string& ffmpegLibPath) {
    Logger::info("Initializing HLS generator...");
    Logger::info("  Input: " + config_.hls.inputFile);
    Logger::info("  Output: " + config_.hls.outputDir);
    Logger::info("  FFmpeg libs: " + ffmpegLibPath);

    if (!ffmpegWrapper_->loadLibraries(ffmpegLibPath)) {
        Logger::error("Failed to load FFmpeg libraries");
        return false;
    }

    if (!ffmpegWrapper_->openInput(config_.hls.inputFile)) {
        Logger::error("Failed to open input file");
        return false;
    }

    if (!ffmpegWrapper_->setupOutput()) {
        Logger::error("Failed to setup HLS output");
        return false;
    }

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

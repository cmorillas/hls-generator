#ifndef HLS_GENERATOR_H
#define HLS_GENERATOR_H

#include <string>
#include <functional>
#include <memory>
#include "ffmpeg_wrapper.h"
#include "config.h"

class HLSGenerator {
public:
    HLSGenerator(const AppConfig& config);
    ~HLSGenerator();

    bool initialize(const std::string& ffmpegLibPath);
    bool generate();

    void setInterruptCallback(std::function<bool()> callback);

private:
    AppConfig config_;
    bool initialized_ = false;
    std::function<bool()> interruptCallback_;
    std::unique_ptr<FFmpegWrapper> ffmpegWrapper_;
};

#endif // HLS_GENERATOR_H

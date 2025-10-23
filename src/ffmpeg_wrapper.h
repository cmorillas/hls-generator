#ifndef FFMPEG_WRAPPER_H
#define FFMPEG_WRAPPER_H

#include <string>
#include <memory>
#include <functional>
#include "config.h"
#include "ffmpeg_deleters.h"

class StreamInput;
class FFmpegContext;
class VideoPipeline;
class AudioPipeline;

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVPacket;
struct AVFrame;
struct AVBSFContext;
struct SwsContext;
struct SwrContext;

class FFmpegWrapper {
public:
    explicit FFmpegWrapper(const AppConfig& config);
    ~FFmpegWrapper();

    bool loadLibraries(const std::string& libPath);
    bool openInput(const std::string& uri);
    bool setupOutput();
    bool resetOutput();

    bool processVideo();

    void setInterruptCallback(std::function<bool()> callback) { interruptCallback_ = callback; }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    double getFPS() const { return fps_; }
    double getDuration() const { return duration_; }

private:
    bool initialized_ = false;

    // Shared FFmpeg context (loaded once, shared among all pipelines)
    std::shared_ptr<FFmpegContext> ffmpegCtx_;

    // Specialized pipelines
    std::unique_ptr<VideoPipeline> videoPipeline_;
    std::unique_ptr<AudioPipeline> audioPipeline_;

    std::unique_ptr<StreamInput> streamInput_;
    AVFormatContext* inputFormatCtx_ = nullptr;
    int videoStreamIndex_ = -1;
    int audioStreamIndex_ = -1;

    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> outputFormatCtx_;
    int outputVideoStreamIndex_ = -1;
    int outputAudioStreamIndex_ = -1;

    enum class ProcessingMode {
        REMUX,
        TRANSCODE,
        PROGRAMMATIC
    };
    ProcessingMode processingMode_ = ProcessingMode::REMUX;

    int width_ = 0;
    int height_ = 0;
    double fps_ = 0.0;
    double duration_ = 0.0;

    const AppConfig config_;  // Immutable configuration
    int reload_count_ = 0;
    std::string input_uri_;

    std::function<bool()> interruptCallback_;

    bool openInputCodec();
    bool detectAndDecideProcessingMode();
    bool processVideoRemux();
    bool processVideoTranscode();
    bool processVideoProgrammatic();
};

#endif // FFMPEG_WRAPPER_H

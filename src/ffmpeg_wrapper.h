#ifndef FFMPEG_WRAPPER_H
#define FFMPEG_WRAPPER_H

#include <string>
#include <memory>
#include <functional>
#include "config.h"
#include "ffmpeg_deleters.h"

class StreamInput;

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
    FFmpegWrapper();
    ~FFmpegWrapper();

    bool loadLibraries(const std::string& libPath);
    void setConfig(const AppConfig& config) { config_ = config; }
    bool openInput(const std::string& uri);
    bool setupOutput(const AppConfig& config);
    bool resetOutput();

    bool processVideo();

    void setInterruptCallback(std::function<bool()> callback) { interruptCallback_ = callback; }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    double getFPS() const { return fps_; }
    double getDuration() const { return duration_; }

private:
    bool initialized_ = false;

    std::unique_ptr<StreamInput> streamInput_;
    AVFormatContext* inputFormatCtx_ = nullptr;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> inputCodecCtx_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> inputAudioCodecCtx_;
    int videoStreamIndex_ = -1;
    int audioStreamIndex_ = -1;

    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> outputFormatCtx_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> outputCodecCtx_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> outputAudioCodecCtx_;
    int outputVideoStreamIndex_ = -1;
    int outputAudioStreamIndex_ = -1;

    std::unique_ptr<AVBSFContext, AVBSFContextDeleter> bsfCtx_;
    std::unique_ptr<SwsContext, SwsContextDeleter> swsCtx_;
    std::unique_ptr<SwrContext, SwrContextDeleter> swrCtx_;
    std::unique_ptr<AVFrame, AVFrameDeleter> convertedFrame_;  // Cached frame for audio conversion

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
    int inputCodecId_ = 0;
    std::string inputCodecName_;

    // Audio transcoding state
    bool audioNeedsTranscoding_ = false;
    int inputAudioCodecId_ = 0;
    int64_t lastAudioPts_ = 0;  // Last valid audio PTS (for synthetic PTS generation)
    bool audioPtsInitialized_ = false;  // Track if we've received first valid PTS
    bool audioPtsWarningShown_ = false;  // Track if we've warned about missing PTS (avoid log spam)

    AppConfig config_;
    int reload_count_ = 0;
    std::string input_uri_;

    std::function<bool()> interruptCallback_;

    bool openInputCodec();
    bool setupEncoder();
    bool setupAudioEncoder(AVStream* outAudioStream);
    bool detectAndDecideProcessingMode();
    bool setupBitstreamFilter();
    bool processVideoRemux();
    bool processVideoTranscode();
    bool processVideoProgrammatic();

    // Helper function to convert and encode a frame with SwsContext if needed
    bool convertAndEncodeFrame(AVFrame* inputFrame, int64_t pts);
};

#endif // FFMPEG_WRAPPER_H

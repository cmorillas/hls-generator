
#ifndef FFMPEG_INPUT_H
#define FFMPEG_INPUT_H

#include "stream_input.h"
#include <memory>

// Forward declarations
class FFmpegContext;

class FFmpegInput : public StreamInput {
public:
    FFmpegInput(const std::string& protocol, std::shared_ptr<FFmpegContext> ffmpegCtx);
    ~FFmpegInput() override;

    bool open(const std::string& uri) override;
    bool readPacket(AVPacket* packet) override;
    void close() override;

    AVFormatContext* getFormatContext() override;
    int getVideoStreamIndex() const override;
    int getAudioStreamIndex() const override;
    bool isLiveStream() const override;
    std::string getTypeName() const override;

private:
    std::shared_ptr<FFmpegContext> ffmpeg_;
    std::string protocol_;
    AVFormatContext* formatContext_ = nullptr;
    int videoStreamIndex_ = -1;
    int audioStreamIndex_ = -1;
};

#endif // FFMPEG_INPUT_H

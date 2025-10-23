#ifndef BROWSER_INPUT_H
#define BROWSER_INPUT_H

#include "stream_input.h"
#include "browser_backend.h"
#include "ffmpeg_deleters.h"
#include "config.h"

#include <string>
#include <mutex>
#include <atomic>
#include <vector>
#include <memory>

// Forward declarations
class FFmpegContext;

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class BrowserInput : public StreamInput {
public:
    BrowserInput(const AppConfig& config, std::shared_ptr<FFmpegContext> ffmpegCtx);
    ~BrowserInput() override;

    bool open(const std::string& uri) override;
    bool readPacket(AVPacket* packet) override;
    void close() override;
    AVFormatContext* getFormatContext() override;
    int getVideoStreamIndex() const override;
    int getAudioStreamIndex() const override;
    bool isLiveStream() const override;
    std::string getTypeName() const override;
    bool isProgrammatic() const override { return true; }

    void setPageReloadCallback(std::function<bool()> callback) { pageReloadCallback_ = callback; }
    bool resetEncoders();

private:
    std::shared_ptr<FFmpegContext> ffmpeg_;
    std::unique_ptr<BrowserBackend> backend_;

    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> format_ctx_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> codec_ctx_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> audio_codec_ctx_;
    std::unique_ptr<SwsContext, SwsContextDeleter> sws_ctx_;
    std::unique_ptr<AVFrame, AVFrameDeleter> yuv_frame_;
    std::unique_ptr<AVFrame, AVFrameDeleter> audio_frame_;

    // Order matches initialization order in constructor to avoid -Wreorder warnings
    AppConfig config_;
    int video_stream_index_;
    int audio_stream_index_;

    std::mutex frame_mutex_;
    std::mutex encoder_mutex_;
    std::vector<uint8_t> current_frame_;
    bool frame_ready_;
    std::atomic<bool> resetting_encoders_;
    int64_t frame_count_;
    int64_t start_time_ms_;
    int snapshot_width_;
    int snapshot_height_;

    std::vector<float> audio_buffer_;
    int audio_channels_;
    int audio_sample_rate_;
    int64_t audio_samples_written_;
    int64_t audio_start_pts_;
    bool audio_stream_started_;
    int64_t last_audio_packet_count_;

    bool initialized_;
    std::atomic<bool> running_;

    std::function<bool()> pageReloadCallback_;

    // SMPTE test bars placeholder
    std::unique_ptr<AVFrame, AVFrameDeleter> smpte_frame_;
    bool createSMPTEFrame();
    void fillSMPTEBars(AVFrame* frame);

    bool setupEncoder(bool is_reset = false);
    bool setupAudioEncoder(int sample_rate, int channels, bool is_reset = false);
    bool convertBGRAtoYUV(const uint8_t* bgra_data, AVFrame* yuv_frame);
    bool convertBGRAtoYUVWithCrop(const uint8_t* bgra_data, int src_width, int src_height, AVFrame* yuv_frame);
    bool encodeFrame(AVFrame* frame, AVPacket* packet);
    bool encodeAudio(AVPacket* packet);
    bool hasAudioData() const;

    void onFrameReceived(const uint8_t* bgra_data, int width, int height);
    void pullAudioFromBackend();
};

#endif // BROWSER_INPUT_H

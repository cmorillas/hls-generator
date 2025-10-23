#ifndef MUXER_MANAGER_H
#define MUXER_MANAGER_H

#include <memory>
#include <string>
#include "ffmpeg_deleters.h"
#include "config.h"

class FFmpegContext;
struct AVFormatContext;
struct AVStream;

/**
 * MuxerManager - Manages HLS output muxer
 *
 * Responsibilities:
 *   - Setup HLS output format context
 *   - Create output streams (video + audio)
 *   - Configure HLS parameters (segment duration, playlist, etc.)
 *   - Write header and trailer
 *   - Create preliminary playlist (prevents 404 race condition)
 *   - Reset logic for stream reload
 */
class MuxerManager {
public:
    explicit MuxerManager(std::shared_ptr<FFmpegContext> ctx);
    ~MuxerManager();

    // No copy, movable
    MuxerManager(const MuxerManager&) = delete;
    MuxerManager& operator=(const MuxerManager&) = delete;
    MuxerManager(MuxerManager&&) = default;
    MuxerManager& operator=(MuxerManager&&) = default;

    /**
     * Setup HLS output muxer
     * @param config Application configuration
     * @param outVideoStream Pointer to receive created video stream
     * @param outAudioStream Pointer to receive created audio stream (may be null if no audio)
     * @param hasAudio Whether to create audio stream
     * @return true on success
     */
    bool setupOutput(const AppConfig& config,
                     AVStream** outVideoStream,
                     AVStream** outAudioStream,
                     bool hasAudio);

    /**
     * Write output header (call after all streams are configured)
     * @return true on success
     */
    bool writeHeader();

    /**
     * Write output trailer (call at end of stream)
     * @return true on success
     */
    bool writeTrailer();

    /**
     * Reset output for new stream
     * @return true on success
     */
    bool reset();

    /**
     * Get output format context
     */
    AVFormatContext* getFormatContext() { return outputFormatCtx_.get(); }

    /**
     * Get video stream index
     */
    int getVideoStreamIndex() const { return outputVideoStreamIndex_; }

    /**
     * Get audio stream index
     */
    int getAudioStreamIndex() const { return outputAudioStreamIndex_; }

private:
    // Shared FFmpeg context
    std::shared_ptr<FFmpegContext> ffmpeg_;

    // Output format context
    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> outputFormatCtx_;

    // Stream indices
    int outputVideoStreamIndex_ = -1;
    int outputAudioStreamIndex_ = -1;

    // Configuration
    AppConfig config_;
    std::string playlistPath_;
};

#endif // MUXER_MANAGER_H

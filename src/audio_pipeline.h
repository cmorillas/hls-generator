#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <memory>
#include <cstdint>
#include "ffmpeg_deleters.h"

class FFmpegContext;
struct AVStream;
struct AVPacket;
struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;

/**
 * AudioPipeline - Handles audio processing (REMUX or TRANSCODE to AAC)
 *
 * Responsibilities:
 *   - Detect if audio needs transcoding (non-AAC → AAC)
 *   - Setup audio encoder (AAC) and decoder (for transcode)
 *   - Setup SwrContext for audio resampling
 *   - Process audio packets (remux or transcode path)
 *   - PTS tracking and synthetic generation
 *   - Flush buffered audio at end-of-stream
 *
 * Lifecycle:
 *   1. Constructor: Receives shared FFmpegContext
 *   2. setupEncoder(): Configures encoder/decoder based on input
 *   3. processPacket(): Called for each audio packet
 *   4. flush(): Drains buffered frames at end
 *   5. reset(): Cleans state for new stream
 */
class AudioPipeline {
public:
    explicit AudioPipeline(std::shared_ptr<FFmpegContext> ctx);
    ~AudioPipeline();

    // No copy, movable
    AudioPipeline(const AudioPipeline&) = delete;
    AudioPipeline& operator=(const AudioPipeline&) = delete;
    AudioPipeline(AudioPipeline&&) = default;
    AudioPipeline& operator=(AudioPipeline&&) = default;

    /**
     * Setup audio encoder and decoder (if transcode needed)
     * @param inStream Input audio stream
     * @param outStream Output audio stream
     * @param audioStreamIndex Index of audio stream in input format context
     * @param inputFormatCtx Input format context (for timebase info)
     * @return true on success
     */
    bool setupEncoder(AVStream* inStream, AVStream* outStream,
                      int audioStreamIndex, AVFormatContext* inputFormatCtx);

    /**
     * Process a single audio packet (remux or transcode)
     * @param packet Audio packet to process
     * @param inputFormatCtx Input format context
     * @param outputFormatCtx Output format context
     * @param audioStreamIndex Input audio stream index
     * @param outputAudioStreamIndex Output audio stream index
     * @return true on success
     */
    bool processPacket(AVPacket* packet,
                       AVFormatContext* inputFormatCtx,
                       AVFormatContext* outputFormatCtx,
                       int audioStreamIndex,
                       int outputAudioStreamIndex);

    /**
     * Flush buffered audio frames (call at end of stream)
     * @param outputFormatCtx Output format context
     * @param outputAudioStreamIndex Output audio stream index
     * @return true on success
     */
    bool flush(AVFormatContext* outputFormatCtx, int outputAudioStreamIndex);

    /**
     * Reset state for new stream
     */
    void reset();

    /**
     * Check if audio needs transcoding
     */
    bool needsTranscoding() const { return needsTranscoding_; }

    /**
     * Get input audio codec ID
     */
    int getInputCodecId() const { return inputCodecId_; }

private:
    // Shared FFmpeg context
    std::shared_ptr<FFmpegContext> ffmpeg_;

    // Codec contexts
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> inputCodecCtx_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> outputCodecCtx_;

    // Audio resampler
    std::unique_ptr<SwrContext, SwrContextDeleter> swrCtx_;

    // Cached frame for conversion
    std::unique_ptr<AVFrame, AVFrameDeleter> convertedFrame_;

    // State
    bool needsTranscoding_ = false;
    int inputCodecId_ = 0;
    int audioStreamIndex_ = -1;  // Cached for PTS calculation
    AVFormatContext* inputFormatCtx_ = nullptr;  // Cached for PTS calculation

    // PTS tracking state
    struct PTSState {
        int64_t lastPts = 0;
        bool initialized = false;
        bool warningShown = false;

        void reset() {
            lastPts = 0;
            initialized = false;
            warningShown = false;
        }
    } ptsState_;

    // Helper: Process a decoded audio frame (transcode path)
    bool processDecodedFrame(AVFrame* audioFrame,
                             AVFormatContext* outputFormatCtx,
                             int outputAudioStreamIndex);

    // Helper: Calculate and assign PTS to converted frame
    void assignPTS(AVFrame* inputFrame, AVFrame* convertedFrame);
};

#endif // AUDIO_PIPELINE_H

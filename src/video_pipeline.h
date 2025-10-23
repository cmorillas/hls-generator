#ifndef VIDEO_PIPELINE_H
#define VIDEO_PIPELINE_H

#include <memory>
#include <string>
#include "ffmpeg_deleters.h"
#include "config.h"

class FFmpegContext;
struct AVStream;
struct AVPacket;
struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;
struct AVBSFContext;
struct SwsContext;
struct AVRational;

/**
 * VideoPipeline - Handles video processing (REMUX, TRANSCODE, or PROGRAMMATIC)
 *
 * Responsibilities:
 *   - Detect processing mode based on codec compatibility
 *   - Setup video encoder (H.264) and decoder (for transcode)
 *   - Setup SwsContext for video scaling/conversion
 *   - Setup bitstream filter (h264_mp4toannexb)
 *   - Process video packets/frames in all modes
 *   - Flush buffered video at end-of-stream
 *
 * Processing Modes:
 *   - REMUX: Copy H.264 packets directly (fast, no quality loss)
 *   - TRANSCODE: Decode and re-encode to H.264 (compatible but slow)
 *   - PROGRAMMATIC: Process packets from browser/CEF sources
 */
class VideoPipeline {
public:
    enum class Mode {
        REMUX,
        TRANSCODE,
        PROGRAMMATIC
    };

    explicit VideoPipeline(std::shared_ptr<FFmpegContext> ctx);
    ~VideoPipeline();

    // No copy, movable
    VideoPipeline(const VideoPipeline&) = delete;
    VideoPipeline& operator=(const VideoPipeline&) = delete;
    VideoPipeline(VideoPipeline&&) = default;
    VideoPipeline& operator=(VideoPipeline&&) = default;

    /**
     * Detect processing mode based on input codec
     * @param inStream Input video stream
     * @return Detected mode
     */
    Mode detectMode(AVStream* inStream);

    /**
     * Setup video encoder for TRANSCODE mode
     * @param outStream Output video stream
     * @param config Video configuration
     * @return true on success
     */
    bool setupEncoder(AVStream* outStream, const AppConfig& config);

    /**
     * Setup video decoder for TRANSCODE mode
     * @param inStream Input video stream
     * @return true on success
     */
    bool setupDecoder(AVStream* inStream);

    /**
     * Setup bitstream filter (h264_mp4toannexb) for REMUX/PROGRAMMATIC
     * @param inStream Input video stream (for REMUX)
     * @param outStream Output video stream (for TRANSCODE)
     * @param mode Current processing mode
     * @param outputCodecTimeBase Output codec time base (for TRANSCODE)
     * @return true on success
     */
    bool setupBitstreamFilter(AVStream* inStream, AVStream* outStream, Mode mode, AVRational outputCodecTimeBase);

    /**
     * Convert and encode a video frame (TRANSCODE mode)
     * @param inputFrame Input frame to convert/encode
     * @param pts Presentation timestamp
     * @return true on success
     */
    bool convertAndEncodeFrame(AVFrame* inputFrame, int64_t pts);

    /**
     * Decode a packet to frames (TRANSCODE mode)
     * @param packet Input packet to decode
     * @param frame Output frame (caller must allocate)
     * @param frameAvailable Set to true if a frame was decoded
     * @return true on success
     */
    bool decodePacket(AVPacket* packet, AVFrame* frame, bool& frameAvailable);

    /**
     * Receive encoded packet from encoder (TRANSCODE mode)
     * @param packet Output packet (caller must allocate)
     * @param packetAvailable Set to true if a packet was received
     * @return true on success
     */
    bool receiveEncodedPacket(AVPacket* packet, bool& packetAvailable);

    /**
     * Process bitstream filter for a packet (REMUX/PROGRAMMATIC modes)
     * @param packet Packet to filter
     * @param outputFormatCtx Output format context
     * @param outputVideoStreamIndex Output video stream index
     * @param inputTimeBase Input time base
     * @param outputTimeBase Output time base
     * @return true on success
     */
    bool processBitstreamFilter(AVPacket* packet,
                                 AVFormatContext* outputFormatCtx,
                                 int outputVideoStreamIndex,
                                 AVRational inputTimeBase,
                                 AVRational outputTimeBase);

    /**
     * Flush encoder (TRANSCODE mode)
     * @param outputFormatCtx Output format context
     * @param outputVideoStreamIndex Output video stream index
     * @return true on success
     */
    bool flushEncoder(AVFormatContext* outputFormatCtx, int outputVideoStreamIndex);

    /**
     * Flush bitstream filter (REMUX/PROGRAMMATIC modes)
     * @param outputFormatCtx Output format context
     * @param outputVideoStreamIndex Output video stream index
     * @param inputTimeBase Input time base
     * @param outputTimeBase Output time base
     * @return true on success
     */
    bool flushBitstreamFilter(AVFormatContext* outputFormatCtx,
                               int outputVideoStreamIndex,
                               AVRational inputTimeBase,
                               AVRational outputTimeBase);

    /**
     * Reset state for new stream
     */
    void reset();

    /**
     * Get current processing mode
     */
    Mode getMode() const { return mode_; }

    /**
     * Get input codec ID
     */
    int getInputCodecId() const { return inputCodecId_; }

    /**
     * Get input codec name
     */
    std::string getInputCodecName() const { return inputCodecName_; }

    /**
     * Get input codec context (for TRANSCODE mode)
     */
    AVCodecContext* getInputCodecContext() { return inputCodecCtx_.get(); }

    /**
     * Get output codec context (for TRANSCODE mode)
     */
    AVCodecContext* getOutputCodecContext() { return outputCodecCtx_.get(); }

private:
    // Shared FFmpeg context
    std::shared_ptr<FFmpegContext> ffmpeg_;

    // Codec contexts
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> inputCodecCtx_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> outputCodecCtx_;

    // Bitstream filter
    std::unique_ptr<AVBSFContext, AVBSFContextDeleter> bsfCtx_;

    // Video scaler
    std::unique_ptr<SwsContext, SwsContextDeleter> swsCtx_;

    // State
    Mode mode_ = Mode::REMUX;
    int inputCodecId_ = 0;
    std::string inputCodecName_;
    AppConfig config_;
};

#endif // VIDEO_PIPELINE_H

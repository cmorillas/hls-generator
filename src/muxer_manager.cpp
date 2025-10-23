#include "muxer_manager.h"
#include "ffmpeg_context.h"
#include "logger.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include <fstream>
#include <sys/stat.h>

#ifdef PLATFORM_WINDOWS
#include <direct.h>
#else
#include <sys/types.h>
#endif

MuxerManager::MuxerManager(std::shared_ptr<FFmpegContext> ctx)
    : ffmpeg_(std::move(ctx)) {
}

MuxerManager::~MuxerManager() = default;

bool MuxerManager::setupOutput(const AppConfig& config,
                                 AVStream** outVideoStream,
                                 AVStream** outAudioStream,
                                 bool hasAudio) {
    config_ = config;

    Logger::info("Setting up HLS output:");
    Logger::info("  Output dir: " + config_.hls.outputDir);
    Logger::info("  Segment duration: " + std::to_string(config_.hls.segmentDuration) + " seconds");

    // Create output directory if it doesn't exist
    struct stat st;
    if (stat(config_.hls.outputDir.c_str(), &st) != 0) {
        Logger::info("Creating output directory...");
#ifdef PLATFORM_WINDOWS
        _mkdir(config_.hls.outputDir.c_str());
#else
        mkdir(config_.hls.outputDir.c_str(), 0755);
#endif
    }

    playlistPath_ = config_.hls.outputDir + "/playlist.m3u8";

    // Create preliminary playlist to avoid 404 errors from players
    Logger::info("Creating preliminary HLS playlist (prevents 404 race condition)");
    std::ofstream prelimPlaylist(playlistPath_);
    if (prelimPlaylist.is_open()) {
        prelimPlaylist << "#EXTM3U\n";
        prelimPlaylist << "#EXT-X-VERSION:6\n";
        prelimPlaylist << "#EXT-X-TARGETDURATION:" << config_.hls.segmentDuration << "\n";
        prelimPlaylist << "#EXT-X-MEDIA-SEQUENCE:0\n";
        prelimPlaylist << "#EXT-X-PLAYLIST-TYPE:EVENT\n";
        prelimPlaylist.close();
        Logger::info("Preliminary playlist created: " + playlistPath_);
    } else {
        Logger::warn("Could not create preliminary playlist (non-fatal)");
    }

    Logger::info("Creating HLS output: " + playlistPath_);

    // Allocate output format context
    AVFormatContext* temp_format_ctx = nullptr;
    if (ffmpeg_->avformat_alloc_output_context2(&temp_format_ctx, nullptr, "hls", playlistPath_.c_str()) < 0) {
        Logger::error("Failed to allocate output context");
        return false;
    }
    outputFormatCtx_.reset(temp_format_ctx);

    // Create video stream
    AVStream* videoStream = ffmpeg_->avformat_new_stream(outputFormatCtx_.get(), nullptr);
    if (!videoStream) {
        Logger::error("Failed to create output video stream");
        return false;
    }
    outputVideoStreamIndex_ = videoStream->index;
    *outVideoStream = videoStream;

    Logger::info("Created video output stream at index " + std::to_string(outputVideoStreamIndex_));

    // Create audio stream if needed
    if (hasAudio) {
        AVStream* audioStream = ffmpeg_->avformat_new_stream(outputFormatCtx_.get(), nullptr);
        if (!audioStream) {
            Logger::error("Failed to create output audio stream");
            return false;
        }
        outputAudioStreamIndex_ = audioStream->index;
        *outAudioStream = audioStream;

        Logger::info("Created audio output stream at index " + std::to_string(outputAudioStreamIndex_));
    } else {
        *outAudioStream = nullptr;
        outputAudioStreamIndex_ = -1;
    }

    // Configure HLS parameters
    ffmpeg_->av_opt_set(outputFormatCtx_->priv_data, "hls_time", std::to_string(config_.hls.segmentDuration).c_str(), 0);
    ffmpeg_->av_opt_set(outputFormatCtx_->priv_data, "hls_list_size", "0", 0);  // Keep all segments
    ffmpeg_->av_opt_set(outputFormatCtx_->priv_data, "hls_segment_filename",
               (config_.hls.outputDir + "/segment_%03d.ts").c_str(), 0);
    ffmpeg_->av_opt_set(outputFormatCtx_->priv_data, "hls_flags", "delete_segments+omit_endlist", 0);

    Logger::info("HLS output configured successfully");
    return true;
}

bool MuxerManager::writeHeader() {
    if (!outputFormatCtx_) {
        Logger::error("Output format context not initialized");
        return false;
    }

    Logger::info("Opening output file: " + playlistPath_);

    if (ffmpeg_->avio_open(&outputFormatCtx_->pb, playlistPath_.c_str(), AVIO_FLAG_WRITE) < 0) {
        Logger::error("Failed to open output file");
        return false;
    }

    Logger::info("Writing output header");

    if (ffmpeg_->avformat_write_header(outputFormatCtx_.get(), nullptr) < 0) {
        Logger::error("Failed to write output header");
        return false;
    }

    Logger::info("Output header written successfully");
    return true;
}

bool MuxerManager::writeTrailer() {
    if (!outputFormatCtx_) {
        return true;
    }

    Logger::info("Writing output trailer");

    if (ffmpeg_->av_write_trailer(outputFormatCtx_.get()) < 0) {
        Logger::error("Failed to write trailer");
        return false;
    }

    Logger::info("Output trailer written successfully");
    return true;
}

bool MuxerManager::reset() {
    Logger::info(">>> RESETTING OUTPUT MUXER");

    if (outputFormatCtx_) {
        if (outputFormatCtx_->pb) {
            Logger::info(">>> Closing output I/O context");
            ffmpeg_->avio_closep(&outputFormatCtx_->pb);
        }

        Logger::info(">>> Freeing output format context");
        outputFormatCtx_.reset();
    }

    outputVideoStreamIndex_ = -1;
    outputAudioStreamIndex_ = -1;

    Logger::info(">>> OUTPUT MUXER RESET COMPLETE");
    return true;
}

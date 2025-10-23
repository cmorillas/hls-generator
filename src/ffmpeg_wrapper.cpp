#include "ffmpeg_wrapper.h"
#include "ffmpeg_context.h"
#include "video_pipeline.h"
#include "audio_pipeline.h"
#include "logger.h"
#include "stream_input.h"
#include "browser_input.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>

#ifdef PLATFORM_WINDOWS
#include <direct.h>
#endif

// Constants for logging and control flow
namespace {
    constexpr int PACKET_LOG_INTERVAL = 100;           // Log every N packets
    constexpr int FRAME_LOG_INTERVAL = 100;            // Log every N frames
    constexpr int MAX_EMPTY_READ_ATTEMPTS = 1000;      // Max empty reads before EOF
}

FFmpegWrapper::FFmpegWrapper(const AppConfig& config)
    : config_(config) {
}

FFmpegWrapper::~FFmpegWrapper() = default;

bool FFmpegWrapper::loadLibraries(const std::string& libPath) {
    // Create shared FFmpeg context (loaded once, shared among all components)
    ffmpegCtx_ = std::make_shared<FFmpegContext>();
    if (!ffmpegCtx_->initialize(libPath)) {
        Logger::error("Failed to initialize FFmpeg context");
        return false;
    }

    // Create specialized pipelines (all share the same FFmpegContext)
    videoPipeline_ = std::make_unique<VideoPipeline>(ffmpegCtx_);
    audioPipeline_ = std::make_unique<AudioPipeline>(ffmpegCtx_);

    Logger::info("FFmpeg context and pipelines initialized successfully");

    initialized_ = true;
    return true;
}

bool FFmpegWrapper::openInput(const std::string& uri) {
    if (!initialized_) {
        Logger::error("FFmpeg not initialized");
        return false;
    }

    input_uri_ = uri;
    Logger::info("Opening input: " + uri);

    streamInput_ = StreamInputFactory::create(uri, config_, ffmpegCtx_);
    if (!streamInput_) {
        Logger::error("Failed to create input source for: " + uri);
        return false;
    }

    Logger::info("Input type: " + streamInput_->getTypeName());

    BrowserInput* browserInput = dynamic_cast<BrowserInput*>(streamInput_.get());
    if (browserInput) {
        browserInput->setPageReloadCallback([this]() {
            return this->resetOutput();
        });
        Logger::info("Page reload callback configured for browser input");
    }

    if (!streamInput_->open(uri)) {
        Logger::error("Failed to open input source");
        return false;
    }

    inputFormatCtx_ = streamInput_->getFormatContext();
    videoStreamIndex_ = streamInput_->getVideoStreamIndex();
    audioStreamIndex_ = streamInput_->getAudioStreamIndex();

    if (!inputFormatCtx_ || videoStreamIndex_ < 0) {
        Logger::error("Invalid input format or no video stream found");
        return false;
    }

    if (audioStreamIndex_ >= 0) {
        Logger::info("Audio stream detected at index " + std::to_string(audioStreamIndex_));
    } else {
        Logger::info("No audio stream (video-only source)");
    }

    AVStream* videoStream = inputFormatCtx_->streams[videoStreamIndex_];
    AVCodecParameters* codecpar = videoStream->codecpar;

    width_ = codecpar->width;
    height_ = codecpar->height;

    if (videoStream->avg_frame_rate.den != 0) {
        fps_ = (double)videoStream->avg_frame_rate.num / videoStream->avg_frame_rate.den;
    } else {
        fps_ = 25.0;
    }

    if (inputFormatCtx_->duration != AV_NOPTS_VALUE) {
        duration_ = (double)inputFormatCtx_->duration / AV_TIME_BASE;
    }

    // Detect processing mode (VideoPipeline will also detect and store codec info)
    if (!detectAndDecideProcessingMode()) {
        return false;
    }

    // Get codec info from VideoPipeline after mode detection
    Logger::info("Video info:");
    Logger::info("  Codec: " + videoPipeline_->getInputCodecName());
    Logger::info("  Resolution: " + std::to_string(width_) + "x" + std::to_string(height_));
    Logger::info("  FPS: " + std::to_string(fps_));
    Logger::info("  Duration: " + std::to_string(duration_) + " seconds");

    if (!openInputCodec()) {
        return false;
    }

    return true;
}

bool FFmpegWrapper::detectAndDecideProcessingMode() {
    Logger::info("Analyzing input codec for HLS compatibility...");

    // Check if input is programmatic (browser/CEF sources)
    if (streamInput_ && streamInput_->isProgrammatic()) {
        Logger::info("Input is programmatic: Using PROGRAMMATIC mode (generated packets)");
        processingMode_ = ProcessingMode::PROGRAMMATIC;
        return true;
    }

    // Delegate mode detection to VideoPipeline
    VideoPipeline::Mode videoMode = videoPipeline_->detectMode(inputFormatCtx_->streams[videoStreamIndex_]);

    // Map VideoPipeline::Mode to FFmpegWrapper::ProcessingMode
    switch (videoMode) {
        case VideoPipeline::Mode::REMUX:
            processingMode_ = ProcessingMode::REMUX;
            break;
        case VideoPipeline::Mode::TRANSCODE:
            processingMode_ = ProcessingMode::TRANSCODE;
            break;
        case VideoPipeline::Mode::PROGRAMMATIC:
            processingMode_ = ProcessingMode::PROGRAMMATIC;
            break;
    }

    return true;
}

bool FFmpegWrapper::openInputCodec() {
    // Only open decoder if we need to transcode
    if (processingMode_ == ProcessingMode::TRANSCODE) {
        return videoPipeline_->setupDecoder(inputFormatCtx_->streams[videoStreamIndex_]);
    }
    return true;
}

bool FFmpegWrapper::setupOutput() {
    // config_ is already initialized in constructor (immutable)

    Logger::info("Setting up HLS output:");
    Logger::info("  Output dir: " + config_.hls.outputDir);
    Logger::info("  Segment duration: " + std::to_string(config_.hls.segmentDuration) + " seconds");

    struct stat st;
    if (stat(config_.hls.outputDir.c_str(), &st) != 0) {
        Logger::info("Creating output directory...");
#ifdef PLATFORM_WINDOWS
        _mkdir(config_.hls.outputDir.c_str());
#else
        mkdir(config_.hls.outputDir.c_str(), 0755);
#endif
    }

    std::string playlistPath = config_.hls.outputDir + "/playlist.m3u8";

    // Create preliminary playlist immediately to avoid 404 errors from players
    // This empty playlist tells the player the stream is starting soon
    Logger::info("Creating preliminary HLS playlist (prevents 404 race condition)");
    std::ofstream prelimPlaylist(playlistPath);
    if (prelimPlaylist.is_open()) {
        prelimPlaylist << "#EXTM3U\n";
        prelimPlaylist << "#EXT-X-VERSION:6\n";
        prelimPlaylist << "#EXT-X-TARGETDURATION:" << config_.hls.segmentDuration << "\n";
        prelimPlaylist << "#EXT-X-MEDIA-SEQUENCE:0\n";
        prelimPlaylist << "#EXT-X-PLAYLIST-TYPE:EVENT\n";
        prelimPlaylist.close();
        Logger::info("Preliminary playlist created: " + playlistPath);
    } else {
        Logger::warn("Could not create preliminary playlist (non-fatal)");
    }

    Logger::info("Creating HLS output: " + playlistPath);

    AVFormatContext* temp_format_ctx = nullptr;
    if (ffmpegCtx_->avformat_alloc_output_context2(&temp_format_ctx, nullptr, "hls", playlistPath.c_str()) < 0) {
        Logger::error("Failed to allocate output context");
        return false;
    }
    outputFormatCtx_.reset(temp_format_ctx);

    AVStream* outVideoStream = ffmpegCtx_->avformat_new_stream(outputFormatCtx_.get(), nullptr);
    if (!outVideoStream) {
        Logger::error("Failed to create output video stream");
        return false;
    }
    outputVideoStreamIndex_ = outVideoStream->index;

    AVStream* outAudioStream = nullptr;
    bool createAudioStream = (audioStreamIndex_ >= 0);

    if (!createAudioStream && streamInput_ && streamInput_->isProgrammatic()) {
        AVFormatContext* inputCtx = streamInput_->getFormatContext();
        if (inputCtx) {
            for (unsigned int i = 0; i < inputCtx->nb_streams; i++) {
                if (inputCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    audioStreamIndex_ = i;
                    createAudioStream = true;
                    Logger::info("Found dynamic audio stream at index " + std::to_string(i));
                    break;
                }
            }
        }
    }

    if (createAudioStream) {
        outAudioStream = ffmpegCtx_->avformat_new_stream(outputFormatCtx_.get(), nullptr);
        if (!outAudioStream) {
            Logger::error("Failed to create output audio stream");
            return false;
        }
        outputAudioStreamIndex_ = outAudioStream->index;
        Logger::info("Created audio output stream at index " + std::to_string(outputAudioStreamIndex_));
    }

    if (processingMode_ == ProcessingMode::REMUX) {
        Logger::info("Configuring output for REMUX mode");
        AVStream* inVideoStream = inputFormatCtx_->streams[videoStreamIndex_];

        if (ffmpegCtx_->avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar) < 0) {
            Logger::error("Failed to copy video codec parameters");
            return false;
        }

        outVideoStream->time_base = inVideoStream->time_base;

        if (audioStreamIndex_ >= 0 && outAudioStream) {
            AVStream* inAudioStream = inputFormatCtx_->streams[audioStreamIndex_];
            if (ffmpegCtx_->avcodec_parameters_copy(outAudioStream->codecpar, inAudioStream->codecpar) < 0) {
                Logger::error("Failed to copy audio codec parameters");
                return false;
            }
            outAudioStream->time_base = inAudioStream->time_base;
            Logger::info("Audio stream configured (codec ID: " +
                        std::to_string(inAudioStream->codecpar->codec_id) + ")");
        }

        // Setup bitstream filter for REMUX mode via VideoPipeline
        if (!videoPipeline_->setupBitstreamFilter(
            inVideoStream,
            outVideoStream,
            VideoPipeline::Mode::REMUX,
            inVideoStream->time_base)) {
            Logger::error("Failed to setup bitstream filter");
            return false;
        }
    } else if (processingMode_ == ProcessingMode::PROGRAMMATIC) {
        Logger::info("Configuring output for PROGRAMMATIC mode");
        AVStream* inVideoStream = inputFormatCtx_->streams[videoStreamIndex_];

        if (ffmpegCtx_->avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar) < 0) {
            Logger::error("Failed to copy video codec parameters");
            return false;
        }

        outVideoStream->time_base = inVideoStream->time_base;

        if (audioStreamIndex_ >= 0 && outAudioStream) {
            AVStream* inAudioStream = inputFormatCtx_->streams[audioStreamIndex_];
            if (ffmpegCtx_->avcodec_parameters_copy(outAudioStream->codecpar, inAudioStream->codecpar) < 0) {
                Logger::error("Failed to copy audio codec parameters");
                return false;
            }
            outAudioStream->time_base = inAudioStream->time_base;
            Logger::info("Audio stream configured (codec ID: " +
                        std::to_string(inAudioStream->codecpar->codec_id) + ")");
        }

        // Setup bitstream filter for PROGRAMMATIC mode via VideoPipeline
        if (!videoPipeline_->setupBitstreamFilter(
            inVideoStream,
            outVideoStream,
            VideoPipeline::Mode::PROGRAMMATIC,
            inVideoStream->time_base)) {
            Logger::error("Failed to setup bitstream filter");
            return false;
        }
    } else {
        Logger::info("Configuring output for TRANSCODE mode");

        // Setup video encoder (H.264) via VideoPipeline
        if (!videoPipeline_->setupEncoder(outVideoStream, config_)) {
            Logger::error("Failed to setup video encoder");
            return false;
        }

        // Setup bitstream filter via VideoPipeline
        if (!videoPipeline_->setupBitstreamFilter(
            inputFormatCtx_->streams[videoStreamIndex_],
            outVideoStream,
            VideoPipeline::Mode::TRANSCODE,
            videoPipeline_->getOutputCodecContext()->time_base)) {
            Logger::error("Failed to setup bitstream filter");
            return false;
        }

        // Configure audio stream in TRANSCODE mode via AudioPipeline
        if (audioStreamIndex_ >= 0 && outAudioStream) {
            AVStream* inAudioStream = inputFormatCtx_->streams[audioStreamIndex_];

            if (!audioPipeline_->setupEncoder(inAudioStream, outAudioStream,
                                               audioStreamIndex_, inputFormatCtx_)) {
                Logger::error("Failed to setup audio encoder via AudioPipeline");
                return false;
            }
        }
    }

    ffmpegCtx_->av_opt_set(outputFormatCtx_->priv_data, "hls_time", std::to_string(config_.hls.segmentDuration).c_str(), 0);

    std::string segmentPattern = config_.hls.outputDir + "/part" + std::to_string(reload_count_) + "_segment%03d.ts";
    ffmpegCtx_->av_opt_set(outputFormatCtx_->priv_data, "hls_segment_filename", segmentPattern.c_str(), 0);

    if (streamInput_->isLiveStream()) {
        ffmpegCtx_->av_opt_set(outputFormatCtx_->priv_data, "hls_playlist_type", "event", 0);
        ffmpegCtx_->av_opt_set(outputFormatCtx_->priv_data, "hls_list_size", std::to_string(config_.hls.playlistSize).c_str(), 0);
        ffmpegCtx_->av_opt_set(outputFormatCtx_->priv_data, "hls_flags", "append_list+delete_segments+independent_segments", 0);
        Logger::info("Configured for live streaming (event type, " + std::to_string(config_.hls.playlistSize) + " segments x " + std::to_string(config_.hls.segmentDuration) + "s, auto-cleanup)");
    } else {
        ffmpegCtx_->av_opt_set(outputFormatCtx_->priv_data, "hls_list_size", "0", 0);
        ffmpegCtx_->av_opt_set(outputFormatCtx_->priv_data, "hls_playlist_type", "vod", 0);
        Logger::info("Configured for VOD (Video on Demand)");
    }

    if (!(outputFormatCtx_->oformat->flags & AVFMT_NOFILE)) {
        if (ffmpegCtx_->avio_open(&outputFormatCtx_->pb, playlistPath.c_str(), AVIO_FLAG_WRITE) < 0) {
            Logger::error("Failed to open output file");
            return false;
        }
    }

    if (ffmpegCtx_->avformat_write_header(outputFormatCtx_.get(), nullptr) < 0) {
        Logger::error("Failed to write output header");
        return false;
    }

    Logger::info("HLS output configured successfully");
    return true;
}

bool FFmpegWrapper::resetOutput() {
    Logger::info(">>> RESETTING OUTPUT: Closing and recreating HLS muxer");

    if (outputFormatCtx_) {
        if (outputFormatCtx_->pb && !(outputFormatCtx_->oformat->flags & AVFMT_NOFILE)) {
            ffmpegCtx_->avio_closep(&outputFormatCtx_->pb);
            Logger::info(">>> Closed output file");
        }
        outputFormatCtx_.reset();
        Logger::info(">>> Freed output format context");
    }

    reload_count_++;
    Logger::info(">>> Starting Part " + std::to_string(reload_count_));

    // Reset video pipeline (SwsContext, encoder, decoder)
    videoPipeline_->reset();
    Logger::info(">>> Reset video pipeline");

    // Reset audio pipeline (SwrContext and PTS tracker)
    audioPipeline_->reset();
    Logger::info(">>> Reset audio pipeline");

    outputVideoStreamIndex_ = -1;
    outputAudioStreamIndex_ = -1;

    if (!setupOutput()) {
        Logger::error("Failed to recreate HLS output");
        return false;
    }

    Logger::info(">>> OUTPUT MUXER RESET COMPLETE");
    return true;
}

bool FFmpegWrapper::processVideo() {
    if (!inputFormatCtx_ || !outputFormatCtx_) {
        Logger::error("Input or output not initialized");
        return false;
    }

    if (processingMode_ == ProcessingMode::REMUX) {
        return processVideoRemux();
    } else if (processingMode_ == ProcessingMode::PROGRAMMATIC) {
        return processVideoProgrammatic();
    } else {
        return processVideoTranscode();
    }
}

bool FFmpegWrapper::processVideoRemux() {
    Logger::info("Processing video: REMUX mode (fast, no quality loss)");

    AVPacket* packet = ffmpegCtx_->av_packet_alloc();
    if (!packet) {
        Logger::error("Failed to allocate packet");
        return false;
    }

    int videoPacketCount = 0;
    int audioPacketCount = 0;

    while (ffmpegCtx_->av_read_frame(inputFormatCtx_, packet) >= 0) {
        if (interruptCallback_ && interruptCallback_()) {
            Logger::info("Processing interrupted by user (Ctrl+C)");
            break;
        }

        if (packet->stream_index == videoStreamIndex_) {
            videoPacketCount++;

            if (videoPacketCount % PACKET_LOG_INTERVAL == 0) {
                Logger::debug("Processed " + std::to_string(videoPacketCount) + " video packets, " +
                             std::to_string(audioPacketCount) + " audio packets");
            }

            // Process video packet via VideoPipeline
            videoPipeline_->processBitstreamFilter(
                packet,
                outputFormatCtx_.get(),
                outputVideoStreamIndex_,
                inputFormatCtx_->streams[videoStreamIndex_]->time_base,
                outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

            ffmpegCtx_->av_packet_unref(packet);
        } else if (audioStreamIndex_ >= 0 && packet->stream_index == audioStreamIndex_) {
            audioPacketCount++;

            // Process audio packet via AudioPipeline
            audioPipeline_->processPacket(packet, inputFormatCtx_, outputFormatCtx_.get(),
                                          audioStreamIndex_, outputAudioStreamIndex_);

            ffmpegCtx_->av_packet_unref(packet);
        } else {
            ffmpegCtx_->av_packet_unref(packet);
        }
    }

    // Flush bitstream filter via VideoPipeline
    videoPipeline_->flushBitstreamFilter(
        outputFormatCtx_.get(),
        outputVideoStreamIndex_,
        inputFormatCtx_->streams[videoStreamIndex_]->time_base,
        outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

    ffmpegCtx_->av_write_trailer(outputFormatCtx_.get());

    Logger::info("Processed " + std::to_string(videoPacketCount) + " video packets, " +
                 std::to_string(audioPacketCount) + " audio packets total");

    ffmpegCtx_->av_packet_free(&packet);

    return true;
}

bool FFmpegWrapper::processVideoProgrammatic() {
    Logger::info("Processing video: PROGRAMMATIC mode (generated packets from input)");

    AVPacket* packet = ffmpegCtx_->av_packet_alloc();
    if (!packet) {
        Logger::error("Failed to allocate packet");
        return false;
    }

    int packetCount = 0;
    int emptyIterations = 0;
    // Use constant instead of local variable

    while (true) {
        if (interruptCallback_ && interruptCallback_()) {
            Logger::info("Processing interrupted by user (Ctrl+C)");
            break;
        }

        bool hasMore = streamInput_->readPacket(packet);

        if (!hasMore) {
            Logger::info("Stream ended by input");
            break;
        }

        if (packet->size <= 0) {
            emptyIterations++;

            if (!streamInput_->isLiveStream() && emptyIterations >= MAX_EMPTY_READ_ATTEMPTS) {
                if (packetCount == 0) {
                    Logger::error("No packets received after waiting");
                    ffmpegCtx_->av_packet_free(&packet);
                    return false;
                } else {
                    Logger::info("No more packets available, ending stream");
                    break;
                }
            }

            continue;
        }

        emptyIterations = 0;
        packetCount++;

        if (packetCount % PACKET_LOG_INTERVAL == 0) {
            Logger::info("Processed " + std::to_string(packetCount) + " packets");
        }

        if (packet->stream_index == outputVideoStreamIndex_) {
            // Process video packet via VideoPipeline
            videoPipeline_->processBitstreamFilter(
                packet,
                outputFormatCtx_.get(),
                outputVideoStreamIndex_,
                inputFormatCtx_->streams[videoStreamIndex_]->time_base,
                outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

            ffmpegCtx_->av_packet_unref(packet);
        } else if (packet->stream_index == outputAudioStreamIndex_) {
            // Process audio packet via AudioPipeline
            audioPipeline_->processPacket(packet, inputFormatCtx_, outputFormatCtx_.get(),
                                          audioStreamIndex_, outputAudioStreamIndex_);

            ffmpegCtx_->av_packet_unref(packet);
        } else {
            ffmpegCtx_->av_packet_unref(packet);
        }
    }

    // Flush bitstream filter via VideoPipeline
    videoPipeline_->flushBitstreamFilter(
        outputFormatCtx_.get(),
        outputVideoStreamIndex_,
        inputFormatCtx_->streams[videoStreamIndex_]->time_base,
        outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

    ffmpegCtx_->av_write_trailer(outputFormatCtx_.get());

    Logger::info("Processed " + std::to_string(packetCount) + " packets total from programmatic input");

    ffmpegCtx_->av_packet_free(&packet);

    return true;
}

bool FFmpegWrapper::processVideoTranscode() {
    Logger::info("Processing video: TRANSCODE mode (decoding and re-encoding to H.264)");
    Logger::info("This may take longer but ensures HLS compatibility");

    AVPacket* packet = ffmpegCtx_->av_packet_alloc();
    AVFrame* frame = ffmpegCtx_->av_frame_alloc();

    if (!packet || !frame) {
        Logger::error("Failed to allocate packet or frame");
        if (packet) ffmpegCtx_->av_packet_free(&packet);
        if (frame) ffmpegCtx_->av_frame_free(&frame);
        return false;
    }

    int frameCount = 0;
    int64_t nextPts = 0;

    while (ffmpegCtx_->av_read_frame(inputFormatCtx_, packet) >= 0) {
        if (interruptCallback_ && interruptCallback_()) {
            Logger::info("Processing interrupted by user (Ctrl+C)");
            break;
        }

        // Process video packets - transcode
        if (packet->stream_index == videoStreamIndex_) {
            // Decode packet using VideoPipeline's decoder
            AVCodecContext* decoderCtx = videoPipeline_->getInputCodecContext();
            if (ffmpegCtx_->avcodec_send_packet(decoderCtx, packet) < 0) {
                Logger::error("Error sending packet to decoder");
                ffmpegCtx_->av_packet_unref(packet);
                continue;
            }

            while (ffmpegCtx_->avcodec_receive_frame(decoderCtx, frame) == 0) {
                frameCount++;

                if (frameCount % FRAME_LOG_INTERVAL == 0) {
                    Logger::info("Transcoded " + std::to_string(frameCount) + " frames");
                }

                // Convert and encode frame via VideoPipeline
                if (!videoPipeline_->convertAndEncodeFrame(frame, nextPts++)) {
                    ffmpegCtx_->av_frame_unref(frame);
                    continue;
                }

                // Receive encoded packets from VideoPipeline's encoder
                AVCodecContext* encoderCtx = videoPipeline_->getOutputCodecContext();
                AVPacket* outPacket = ffmpegCtx_->av_packet_alloc();
                while (ffmpegCtx_->avcodec_receive_packet(encoderCtx, outPacket) == 0) {
                    // Process encoded packet via VideoPipeline's bitstream filter
                    videoPipeline_->processBitstreamFilter(
                        outPacket,
                        outputFormatCtx_.get(),
                        outputVideoStreamIndex_,
                        encoderCtx->time_base,
                        outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

                    ffmpegCtx_->av_packet_unref(outPacket);
                }
                ffmpegCtx_->av_packet_free(&outPacket);

                ffmpegCtx_->av_frame_unref(frame);
            }
        }
        // Process audio packets via AudioPipeline
        else if (packet->stream_index == audioStreamIndex_ && outputAudioStreamIndex_ >= 0) {
            audioPipeline_->processPacket(packet, inputFormatCtx_, outputFormatCtx_.get(),
                                          audioStreamIndex_, outputAudioStreamIndex_);
        }

        ffmpegCtx_->av_packet_unref(packet);
    }

    // Drain decoder (flush remaining frames) via VideoPipeline
    AVCodecContext* decoderCtx = videoPipeline_->getInputCodecContext();
    AVCodecContext* encoderCtx = videoPipeline_->getOutputCodecContext();

    ffmpegCtx_->avcodec_send_packet(decoderCtx, nullptr);
    while (ffmpegCtx_->avcodec_receive_frame(decoderCtx, frame) == 0) {
        // Convert and encode flushed frame via VideoPipeline
        if (!videoPipeline_->convertAndEncodeFrame(frame, nextPts++)) {
            ffmpegCtx_->av_frame_unref(frame);
            continue;
        }

        AVPacket* outPacket = ffmpegCtx_->av_packet_alloc();
        while (ffmpegCtx_->avcodec_receive_packet(encoderCtx, outPacket) == 0) {
            // Process encoded packet via VideoPipeline's bitstream filter
            videoPipeline_->processBitstreamFilter(
                outPacket,
                outputFormatCtx_.get(),
                outputVideoStreamIndex_,
                encoderCtx->time_base,
                outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

            ffmpegCtx_->av_packet_unref(outPacket);
        }
        ffmpegCtx_->av_packet_free(&outPacket);

        ffmpegCtx_->av_frame_unref(frame);
    }

    // Flush encoder via VideoPipeline
    videoPipeline_->flushEncoder(outputFormatCtx_.get(), outputVideoStreamIndex_);

    // Flush audio decoder and encoder via AudioPipeline
    if (outputAudioStreamIndex_ >= 0) {
        audioPipeline_->flush(outputFormatCtx_.get(), outputAudioStreamIndex_);
    }

    ffmpegCtx_->av_write_trailer(outputFormatCtx_.get());

    Logger::info("Transcoded " + std::to_string(frameCount) + " frames total");

    ffmpegCtx_->av_packet_free(&packet);
    ffmpegCtx_->av_frame_free(&frame);

    return true;
}

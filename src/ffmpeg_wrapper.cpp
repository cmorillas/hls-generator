#include "ffmpeg_wrapper.h"
#include "ffmpeg_loader.h"
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

#define avformat_open_input FFmpegLib::avformat_open_input
#define avformat_close_input FFmpegLib::avformat_close_input
#define avformat_find_stream_info FFmpegLib::avformat_find_stream_info
#define avformat_alloc_output_context2 FFmpegLib::avformat_alloc_output_context2
#define avformat_free_context FFmpegLib::avformat_free_context
#define avformat_new_stream FFmpegLib::avformat_new_stream
#define avformat_write_header FFmpegLib::avformat_write_header
#define av_write_trailer FFmpegLib::av_write_trailer
#define av_interleaved_write_frame FFmpegLib::av_interleaved_write_frame
#define av_read_frame FFmpegLib::av_read_frame
#define avio_open FFmpegLib::avio_open
#define avio_closep FFmpegLib::avio_closep

#define avcodec_find_decoder FFmpegLib::avcodec_find_decoder
#define avcodec_find_encoder FFmpegLib::avcodec_find_encoder
#define avcodec_find_encoder_by_name FFmpegLib::avcodec_find_encoder_by_name
#define avcodec_alloc_context3 FFmpegLib::avcodec_alloc_context3
#define avcodec_free_context FFmpegLib::avcodec_free_context
#define avcodec_open2 FFmpegLib::avcodec_open2
#define avcodec_parameters_to_context FFmpegLib::avcodec_parameters_to_context
#define avcodec_parameters_from_context FFmpegLib::avcodec_parameters_from_context
#define avcodec_parameters_copy FFmpegLib::avcodec_parameters_copy
#define avcodec_send_packet FFmpegLib::avcodec_send_packet
#define avcodec_receive_frame FFmpegLib::avcodec_receive_frame
#define avcodec_send_frame FFmpegLib::avcodec_send_frame
#define avcodec_receive_packet FFmpegLib::avcodec_receive_packet

#define av_version_info FFmpegLib::av_version_info
#define av_frame_alloc FFmpegLib::av_frame_alloc
#define av_frame_free FFmpegLib::av_frame_free
#define av_frame_unref FFmpegLib::av_frame_unref
#define av_frame_get_buffer FFmpegLib::av_frame_get_buffer
#define av_packet_alloc FFmpegLib::av_packet_alloc
#define av_packet_free FFmpegLib::av_packet_free
#define av_packet_unref FFmpegLib::av_packet_unref
#define av_packet_clone FFmpegLib::av_packet_clone
#define av_packet_rescale_ts FFmpegLib::av_packet_rescale_ts
#define av_opt_set FFmpegLib::av_opt_set
#define av_malloc FFmpegLib::av_malloc
#define av_free FFmpegLib::av_free

#define av_bsf_get_by_name FFmpegLib::av_bsf_get_by_name
#define av_bsf_alloc FFmpegLib::av_bsf_alloc
#define av_bsf_init FFmpegLib::av_bsf_init
#define av_bsf_free FFmpegLib::av_bsf_free
#define av_bsf_send_packet FFmpegLib::av_bsf_send_packet
#define av_bsf_receive_packet FFmpegLib::av_bsf_receive_packet

#define sws_getContext FFmpegLib::sws_getContext
#define sws_freeContext FFmpegLib::sws_freeContext
#define sws_scale FFmpegLib::sws_scale

// Constants for logging and control flow
namespace {
    constexpr int PACKET_LOG_INTERVAL = 100;           // Log every N packets
    constexpr int FRAME_LOG_INTERVAL = 100;            // Log every N frames
    constexpr int MAX_EMPTY_READ_ATTEMPTS = 1000;      // Max empty reads before EOF
}

FFmpegWrapper::FFmpegWrapper() = default;

FFmpegWrapper::~FFmpegWrapper() = default;

bool FFmpegWrapper::loadLibraries(const std::string& libPath) {
    if (!loadFFmpegLibraries(libPath)) {
        Logger::error("Failed to load FFmpeg libraries");
        return false;
    }

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

    streamInput_ = StreamInputFactory::create(uri, config_);
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
    inputCodecId_ = codecpar->codec_id;

    if (videoStream->avg_frame_rate.den != 0) {
        fps_ = (double)videoStream->avg_frame_rate.num / videoStream->avg_frame_rate.den;
    } else {
        fps_ = 25.0;
    }

    if (inputFormatCtx_->duration != AV_NOPTS_VALUE) {
        duration_ = (double)inputFormatCtx_->duration / AV_TIME_BASE;
    }

    const AVCodec* decoder = avcodec_find_decoder(inputCodecId_);
    if (decoder) {
        inputCodecName_ = decoder->name;
    } else {
        inputCodecName_ = "unknown";
    }

    Logger::info("Video info:");
    Logger::info("  Codec: " + inputCodecName_);
    Logger::info("  Resolution: " + std::to_string(width_) + "x" + std::to_string(height_));
    Logger::info("  FPS: " + std::to_string(fps_));
    Logger::info("  Duration: " + std::to_string(duration_) + " seconds");

    if (!detectAndDecideProcessingMode()) {
        return false;
    }

    if (!openInputCodec()) {
        return false;
    }

    return true;
}

bool FFmpegWrapper::detectAndDecideProcessingMode() {
    Logger::info("Analyzing input codec for HLS compatibility...");

    if (streamInput_ && streamInput_->isProgrammatic()) {
        Logger::info("Input is programmatic: Using PROGRAMMATIC mode (generated packets)");
        processingMode_ = ProcessingMode::PROGRAMMATIC;
        return true;
    }

    if (inputCodecId_ == AV_CODEC_ID_H264) {
        Logger::info("Input is H.264: Using REMUX mode (fast, no quality loss)");
        processingMode_ = ProcessingMode::REMUX;
        return true;
    }

    if (inputCodecId_ == AV_CODEC_ID_HEVC) {
        Logger::warn("Input is HEVC/H.265: Limited HLS support");
        Logger::warn("Will TRANSCODE to H.264 for maximum compatibility");
        processingMode_ = ProcessingMode::TRANSCODE;
        return true;
    }

    Logger::warn("Input codec '" + inputCodecName_ + "' is not HLS-compatible");
    Logger::warn("Will TRANSCODE to H.264 (this may take longer)");
    processingMode_ = ProcessingMode::TRANSCODE;
    return true;
}

bool FFmpegWrapper::openInputCodec() {
    AVCodecParameters* codecpar = inputFormatCtx_->streams[videoStreamIndex_]->codecpar;

    const AVCodec* decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder) {
        Logger::error("Failed to find decoder");
        return false;
    }

    Logger::info("Input codec: " + std::string(decoder->name));

    inputCodecCtx_.reset(avcodec_alloc_context3(decoder));
    if (!inputCodecCtx_) {
        Logger::error("Failed to allocate decoder context");
        return false;
    }

    if (avcodec_parameters_to_context(inputCodecCtx_.get(), codecpar) < 0) {
        Logger::error("Failed to copy codec parameters");
        return false;
    }

    if (avcodec_open2(inputCodecCtx_.get(), decoder, nullptr) < 0) {
        Logger::error("Failed to open decoder");
        return false;
    }

    return true;
}

bool FFmpegWrapper::setupOutput(const AppConfig& config) {
    config_ = config;

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
    if (avformat_alloc_output_context2(&temp_format_ctx, nullptr, "hls", playlistPath.c_str()) < 0) {
        Logger::error("Failed to allocate output context");
        return false;
    }
    outputFormatCtx_.reset(temp_format_ctx);

    AVStream* outVideoStream = avformat_new_stream(outputFormatCtx_.get(), nullptr);
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
        outAudioStream = avformat_new_stream(outputFormatCtx_.get(), nullptr);
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

        if (avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar) < 0) {
            Logger::error("Failed to copy video codec parameters");
            return false;
        }

        outVideoStream->time_base = inVideoStream->time_base;

        if (audioStreamIndex_ >= 0 && outAudioStream) {
            AVStream* inAudioStream = inputFormatCtx_->streams[audioStreamIndex_];
            if (avcodec_parameters_copy(outAudioStream->codecpar, inAudioStream->codecpar) < 0) {
                Logger::error("Failed to copy audio codec parameters");
                return false;
            }
            outAudioStream->time_base = inAudioStream->time_base;
            Logger::info("Audio stream configured (codec ID: " +
                        std::to_string(inAudioStream->codecpar->codec_id) + ")");
        }

        if (!setupBitstreamFilter()) {
            return false;
        }
    } else if (processingMode_ == ProcessingMode::PROGRAMMATIC) {
        Logger::info("Configuring output for PROGRAMMATIC mode");
        AVStream* inVideoStream = inputFormatCtx_->streams[videoStreamIndex_];

        if (avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar) < 0) {
            Logger::error("Failed to copy video codec parameters");
            return false;
        }

        outVideoStream->time_base = inVideoStream->time_base;

        if (audioStreamIndex_ >= 0 && outAudioStream) {
            AVStream* inAudioStream = inputFormatCtx_->streams[audioStreamIndex_];
            if (avcodec_parameters_copy(outAudioStream->codecpar, inAudioStream->codecpar) < 0) {
                Logger::error("Failed to copy audio codec parameters");
                return false;
            }
            outAudioStream->time_base = inAudioStream->time_base;
            Logger::info("Audio stream configured (codec ID: " +
                        std::to_string(inAudioStream->codecpar->codec_id) + ")");
        }

        if (!setupBitstreamFilter()) {
            return false;
        }
    } else {
        Logger::info("Configuring output for TRANSCODE mode");

        if (!setupEncoder()) {
            return false;
        }

        if (avcodec_parameters_from_context(outVideoStream->codecpar, outputCodecCtx_.get()) < 0) {
            Logger::error("Failed to copy encoder parameters to output stream");
            return false;
        }

        outVideoStream->time_base = outputCodecCtx_->time_base;

        if (!setupBitstreamFilter()) {
            return false;
        }

        // Configure audio stream in TRANSCODE mode
        if (audioStreamIndex_ >= 0 && outAudioStream) {
            AVStream* inAudioStream = inputFormatCtx_->streams[audioStreamIndex_];
            inputAudioCodecId_ = inAudioStream->codecpar->codec_id;

            // Decide strategy: Remux AAC or Transcode to AAC
            if (inputAudioCodecId_ == AV_CODEC_ID_AAC) {
                Logger::info("Audio is AAC - will REMUX (copy without transcoding)");
                audioNeedsTranscoding_ = false;

                // Copy codec parameters for remux
                if (avcodec_parameters_copy(outAudioStream->codecpar, inAudioStream->codecpar) < 0) {
                    Logger::error("Failed to copy audio codec parameters");
                    return false;
                }
                outAudioStream->time_base = inAudioStream->time_base;
            } else {
                Logger::warn("Audio codec ID " + std::to_string(inputAudioCodecId_) +
                            " (non-AAC) - will TRANSCODE to AAC");
                audioNeedsTranscoding_ = true;

                // FIRST: Open audio decoder (needed by setupAudioEncoder to configure SwrContext)
                const AVCodec* audioDecoder = avcodec_find_decoder(inputAudioCodecId_);
                if (!audioDecoder) {
                    Logger::error("Audio decoder not found for codec " + std::to_string(inputAudioCodecId_));
                    return false;
                }

                inputAudioCodecCtx_.reset(avcodec_alloc_context3(audioDecoder));
                if (!inputAudioCodecCtx_) {
                    Logger::error("Failed to allocate audio decoder context");
                    return false;
                }

                if (avcodec_parameters_to_context(inputAudioCodecCtx_.get(), inAudioStream->codecpar) < 0) {
                    Logger::error("Failed to copy audio codec parameters to decoder");
                    return false;
                }

                if (avcodec_open2(inputAudioCodecCtx_.get(), audioDecoder, nullptr) < 0) {
                    Logger::error("Failed to open audio decoder");
                    return false;
                }

                Logger::info("Audio decoder opened for transcoding");

                // SECOND: Setup audio encoder (uses inputAudioCodecCtx_ for SwrContext config)
                if (!setupAudioEncoder(outAudioStream)) {
                    Logger::error("Failed to setup audio encoder");
                    return false;
                }
            }
        }
    }

    av_opt_set(outputFormatCtx_->priv_data, "hls_time", std::to_string(config_.hls.segmentDuration).c_str(), 0);

    std::string segmentPattern = config_.hls.outputDir + "/part" + std::to_string(reload_count_) + "_segment%03d.ts";
    av_opt_set(outputFormatCtx_->priv_data, "hls_segment_filename", segmentPattern.c_str(), 0);

    if (streamInput_->isLiveStream()) {
        av_opt_set(outputFormatCtx_->priv_data, "hls_playlist_type", "event", 0);
        av_opt_set(outputFormatCtx_->priv_data, "hls_list_size", std::to_string(config_.hls.playlistSize).c_str(), 0);
        av_opt_set(outputFormatCtx_->priv_data, "hls_flags", "append_list+delete_segments+independent_segments", 0);
        Logger::info("Configured for live streaming (event type, " + std::to_string(config_.hls.playlistSize) + " segments x " + std::to_string(config_.hls.segmentDuration) + "s, auto-cleanup)");
    } else {
        av_opt_set(outputFormatCtx_->priv_data, "hls_list_size", "0", 0);
        av_opt_set(outputFormatCtx_->priv_data, "hls_playlist_type", "vod", 0);
        Logger::info("Configured for VOD (Video on Demand)");
    }

    if (!(outputFormatCtx_->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatCtx_->pb, playlistPath.c_str(), AVIO_FLAG_WRITE) < 0) {
            Logger::error("Failed to open output file");
            return false;
        }
    }

    if (avformat_write_header(outputFormatCtx_.get(), nullptr) < 0) {
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
            avio_closep(&outputFormatCtx_->pb);
            Logger::info(">>> Closed output file");
        }
        outputFormatCtx_.reset();
        Logger::info(">>> Freed output format context");
    }

    reload_count_++;
    Logger::info(">>> Starting Part " + std::to_string(reload_count_));

    // Reset SwsContext as input resolution might change after reload
    if (swsCtx_) {
        swsCtx_.reset();
        Logger::info(">>> Reset video scaler context");
    }

    // Reset SwrContext and audio PTS tracker for clean audio restart
    if (swrCtx_) {
        swrCtx_.reset();
        Logger::info(">>> Reset audio resampler context");
    }
    lastAudioPts_ = 0;  // Reset PTS tracker to avoid inconsistent timestamps
    audioPtsInitialized_ = false;  // Reset initialization flag for next stream
    audioPtsWarningShown_ = false;  // Reset warning flag to show message for new stream

    outputVideoStreamIndex_ = -1;
    outputAudioStreamIndex_ = -1;

    if (!setupOutput(config_)) {
        Logger::error("Failed to recreate HLS output");
        return false;
    }

    Logger::info(">>> OUTPUT MUXER RESET COMPLETE");
    return true;
}

bool FFmpegWrapper::setupBitstreamFilter() {
    Logger::info("Setting up h264_mp4toannexb bitstream filter");

    const AVBitStreamFilter* bsf = av_bsf_get_by_name("h264_mp4toannexb");
    if (!bsf) {
        Logger::error("Failed to find h264_mp4toannexb filter");
        return false;
    }

    AVBSFContext* temp_bsf_ctx = nullptr;
    if (av_bsf_alloc(bsf, &temp_bsf_ctx) < 0) {
        Logger::error("Failed to allocate bitstream filter context");
        return false;
    }
    bsfCtx_.reset(temp_bsf_ctx);

    AVCodecParameters* codecParams = nullptr;
    AVRational timeBase;

    if (processingMode_ == ProcessingMode::REMUX || processingMode_ == ProcessingMode::PROGRAMMATIC) {
        AVStream* inStream = inputFormatCtx_->streams[videoStreamIndex_];
        codecParams = inStream->codecpar;
        timeBase = inStream->time_base;
    } else {
        codecParams = outputFormatCtx_->streams[0]->codecpar;
        timeBase = outputCodecCtx_->time_base;
    }

    if (avcodec_parameters_copy(bsfCtx_->par_in, codecParams) < 0) {
        Logger::error("Failed to copy codec parameters to filter");
        bsfCtx_.reset();
        return false;
    }

    bsfCtx_->time_base_in = timeBase;

    if (av_bsf_init(bsfCtx_.get()) < 0) {
        Logger::error("Failed to initialize bitstream filter");
        bsfCtx_.reset();
        return false;
    }

    Logger::info("Bitstream filter initialized successfully");
    return true;
}

bool FFmpegWrapper::setupEncoder() {
    const AVCodec* encoder = avcodec_find_encoder_by_name("libx264");
    if (!encoder) {
        Logger::warn("libx264 not found, trying default H.264 encoder");
        encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    }

    if (!encoder) {
        Logger::error("H.264 encoder not found");
        return false;
    }

    Logger::info("Using encoder: " + std::string(encoder->name));

    outputCodecCtx_.reset(avcodec_alloc_context3(encoder));
    if (!outputCodecCtx_) {
        Logger::error("Failed to allocate encoder context");
        return false;
    }

    outputCodecCtx_->width = config_.video.width;
    outputCodecCtx_->height = config_.video.height;
    outputCodecCtx_->time_base = AVRational{1, config_.video.fps};
    outputCodecCtx_->framerate = AVRational{config_.video.fps, 1};
    outputCodecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    outputCodecCtx_->bit_rate = config_.video.bitrate;
    outputCodecCtx_->gop_size = config_.video.gop_size;
    outputCodecCtx_->max_b_frames = 0;  // No B-frames for HLS streaming (prevents stuttering)

    av_opt_set(outputCodecCtx_->priv_data, "preset", "ultrafast", 0);
    av_opt_set(outputCodecCtx_->priv_data, "tune", "zerolatency", 0);

    if (avcodec_open2(outputCodecCtx_.get(), encoder, nullptr) < 0) {
        Logger::error("Failed to open encoder");
        return false;
    }

    return true;
}

bool FFmpegWrapper::setupAudioEncoder(AVStream* outAudioStream) {
    // Find AAC encoder
    const AVCodec* audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audioCodec) {
        Logger::error("AAC encoder not found");
        return false;
    }

    Logger::info("Setting up AAC audio encoder");

    // Allocate audio encoder context
    outputAudioCodecCtx_.reset(avcodec_alloc_context3(audioCodec));
    if (!outputAudioCodecCtx_) {
        Logger::error("Failed to allocate audio encoder context");
        return false;
    }

    // Configure encoder from input audio stream
    AVStream* inAudioStream = inputFormatCtx_->streams[audioStreamIndex_];
    outputAudioCodecCtx_->sample_rate = inAudioStream->codecpar->sample_rate;
    outputAudioCodecCtx_->ch_layout = inAudioStream->codecpar->ch_layout;
    outputAudioCodecCtx_->sample_fmt = audioCodec->sample_fmts ? audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    outputAudioCodecCtx_->bit_rate = 128000; // 128 kbps - good quality for streaming
    outputAudioCodecCtx_->time_base = AVRational{1, outputAudioCodecCtx_->sample_rate};

    // Open encoder
    if (avcodec_open2(outputAudioCodecCtx_.get(), audioCodec, nullptr) < 0) {
        Logger::error("Failed to open audio encoder");
        return false;
    }

    // Copy parameters to output stream
    if (avcodec_parameters_from_context(outAudioStream->codecpar, outputAudioCodecCtx_.get()) < 0) {
        Logger::error("Failed to copy audio encoder parameters to output stream");
        return false;
    }

    outAudioStream->time_base = outputAudioCodecCtx_->time_base;

    Logger::info("Audio encoder configured: AAC, " +
                std::to_string(outputAudioCodecCtx_->sample_rate) + " Hz, " +
                std::to_string(outputAudioCodecCtx_->bit_rate / 1000) + " kbps");

    // Initialize SwrContext for audio format conversion (decoder -> encoder)
    SwrContext* swrCtxRaw = FFmpegLib::swr_alloc();
    if (!swrCtxRaw) {
        Logger::error("Failed to allocate SwrContext");
        return false;
    }

    int ret = FFmpegLib::swr_alloc_set_opts2(
        &swrCtxRaw,
        &outputAudioCodecCtx_->ch_layout,           // Output channel layout
        outputAudioCodecCtx_->sample_fmt,           // Output sample format (FLTP for AAC)
        outputAudioCodecCtx_->sample_rate,          // Output sample rate
        &inputAudioCodecCtx_->ch_layout,            // Input channel layout
        inputAudioCodecCtx_->sample_fmt,            // Input sample format
        inputAudioCodecCtx_->sample_rate,           // Input sample rate
        0, nullptr);

    if (ret < 0) {
        Logger::error("Failed to configure SwrContext");
        FFmpegLib::swr_free(&swrCtxRaw);
        return false;
    }

    if (FFmpegLib::swr_init(swrCtxRaw) < 0) {
        Logger::error("Failed to initialize SwrContext");
        FFmpegLib::swr_free(&swrCtxRaw);
        return false;
    }

    // Transfer ownership to unique_ptr
    swrCtx_.reset(swrCtxRaw);

    // Allocate cached frame for audio conversion
    convertedFrame_.reset(av_frame_alloc());
    if (!convertedFrame_) {
        Logger::error("Failed to allocate converted audio frame");
        return false;
    }

    Logger::info("Audio resampler initialized successfully");

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

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        Logger::error("Failed to allocate packet");
        return false;
    }

    int videoPacketCount = 0;
    int audioPacketCount = 0;

    while (av_read_frame(inputFormatCtx_, packet) >= 0) {
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

            if (bsfCtx_) {
                if (av_bsf_send_packet(bsfCtx_.get(), packet) < 0) {
                    Logger::error("Error sending packet to bitstream filter");
                    av_packet_unref(packet);
                    continue;
                }

                while (av_bsf_receive_packet(bsfCtx_.get(), packet) == 0) {
                    packet->stream_index = outputVideoStreamIndex_;
                    av_packet_rescale_ts(packet,
                        inputFormatCtx_->streams[videoStreamIndex_]->time_base,
                        outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

                    if (av_interleaved_write_frame(outputFormatCtx_.get(), packet) < 0) {
                        Logger::error("Error writing video frame to HLS output");
                    }

                    av_packet_unref(packet);
                }
            } else {
                packet->stream_index = outputVideoStreamIndex_;
                av_packet_rescale_ts(packet,
                    inputFormatCtx_->streams[videoStreamIndex_]->time_base,
                    outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

                if (av_interleaved_write_frame(outputFormatCtx_.get(), packet) < 0) {
                    Logger::error("Error writing video frame to HLS output");
                }

                av_packet_unref(packet);
            }
        } else if (audioStreamIndex_ >= 0 && packet->stream_index == audioStreamIndex_) {
            audioPacketCount++;

            packet->stream_index = outputAudioStreamIndex_;
            av_packet_rescale_ts(packet,
                inputFormatCtx_->streams[audioStreamIndex_]->time_base,
                outputFormatCtx_->streams[outputAudioStreamIndex_]->time_base);

            if (av_interleaved_write_frame(outputFormatCtx_.get(), packet) < 0) {
                Logger::error("Error writing audio frame to HLS output");
            }

            av_packet_unref(packet);
        } else {
            av_packet_unref(packet);
        }
    }

    if (bsfCtx_) {
        av_bsf_send_packet(bsfCtx_.get(), nullptr);
        while (av_bsf_receive_packet(bsfCtx_.get(), packet) == 0) {
            packet->stream_index = outputVideoStreamIndex_;
            av_packet_rescale_ts(packet,
                inputFormatCtx_->streams[videoStreamIndex_]->time_base,
                outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

            av_interleaved_write_frame(outputFormatCtx_.get(), packet);
            av_packet_unref(packet);
        }
    }

    av_write_trailer(outputFormatCtx_.get());

    Logger::info("Processed " + std::to_string(videoPacketCount) + " video packets, " +
                 std::to_string(audioPacketCount) + " audio packets total");

    av_packet_free(&packet);

    return true;
}

bool FFmpegWrapper::processVideoProgrammatic() {
    Logger::info("Processing video: PROGRAMMATIC mode (generated packets from input)");

    AVPacket* packet = av_packet_alloc();
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
                    av_packet_free(&packet);
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
            if (bsfCtx_) {
                if (av_bsf_send_packet(bsfCtx_.get(), packet) < 0) {
                    Logger::error("Error sending video packet to bitstream filter");
                    av_packet_unref(packet);
                    continue;
                }

                while (av_bsf_receive_packet(bsfCtx_.get(), packet) == 0) {
                    packet->stream_index = outputVideoStreamIndex_;
                    av_packet_rescale_ts(packet,
                        inputFormatCtx_->streams[videoStreamIndex_]->time_base,
                        outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

                    if (av_interleaved_write_frame(outputFormatCtx_.get(), packet) < 0) {
                        Logger::error("Error writing video frame to HLS output");
                    }

                    av_packet_unref(packet);
                }
            } else {
                packet->stream_index = outputVideoStreamIndex_;
                av_packet_rescale_ts(packet,
                    inputFormatCtx_->streams[videoStreamIndex_]->time_base,
                    outputFormatCtx_->streams[outputVideoStreamIndex_]->time_base);

                if (av_interleaved_write_frame(outputFormatCtx_.get(), packet) < 0) {
                    Logger::error("Error writing video frame to HLS output");
                }

                av_packet_unref(packet);
            }
        } else if (packet->stream_index == outputAudioStreamIndex_) {
            av_packet_rescale_ts(packet,
                inputFormatCtx_->streams[audioStreamIndex_]->time_base,
                outputFormatCtx_->streams[outputAudioStreamIndex_]->time_base);

            if (av_interleaved_write_frame(outputFormatCtx_.get(), packet) < 0) {
                Logger::error("Error writing audio frame to HLS output");
            }

            av_packet_unref(packet);
        } else {
            av_packet_unref(packet);
        }
    }

    if (bsfCtx_) {
        av_bsf_send_packet(bsfCtx_.get(), nullptr);
        while (av_bsf_receive_packet(bsfCtx_.get(), packet) == 0) {
            packet->stream_index = 0;
            av_packet_rescale_ts(packet,
                inputFormatCtx_->streams[videoStreamIndex_]->time_base,
                outputFormatCtx_->streams[0]->time_base);

            av_interleaved_write_frame(outputFormatCtx_.get(), packet);
            av_packet_unref(packet);
        }
    }

    av_write_trailer(outputFormatCtx_.get());

    Logger::info("Processed " + std::to_string(packetCount) + " packets total from programmatic input");

    av_packet_free(&packet);

    return true;
}

bool FFmpegWrapper::convertAndEncodeFrame(AVFrame* inputFrame, int64_t pts) {
    if (!inputFrame || !outputCodecCtx_) {
        return false;
    }

    // Set PTS
    inputFrame->pts = pts;

    // Check if we need resolution/format conversion
    bool needsConversion = (inputFrame->width != outputCodecCtx_->width) ||
                          (inputFrame->height != outputCodecCtx_->height) ||
                          (inputFrame->format != outputCodecCtx_->pix_fmt);

    AVFrame* frameToEncode = inputFrame;
    AVFrame* scaledFrame = nullptr;

    if (needsConversion) {
        // Get or recreate SwsContext if input dimensions/format changed
        // sws_getCachedContext automatically recreates context when parameters change
        SwsContext* newCtx = FFmpegLib::sws_getCachedContext(
            swsCtx_.get(),  // Can be nullptr for first call
            inputFrame->width, inputFrame->height, (AVPixelFormat)inputFrame->format,
            outputCodecCtx_->width, outputCodecCtx_->height, outputCodecCtx_->pix_fmt,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!newCtx) {
            Logger::error("Failed to get/create video scaler context");
            return false;
        }

        // Check if context was recreated (different pointer)
        if (newCtx != swsCtx_.get()) {
            Logger::info("Video scaler " + std::string(swsCtx_ ? "recreated" : "initialized") + ": " +
                       std::to_string(inputFrame->width) + "x" + std::to_string(inputFrame->height) + " " +
                       "fmt=" + std::to_string(inputFrame->format) + " -> " +
                       std::to_string(outputCodecCtx_->width) + "x" + std::to_string(outputCodecCtx_->height) + " " +
                       "fmt=" + std::to_string(outputCodecCtx_->pix_fmt));
            swsCtx_.reset(newCtx);
        }

        // Allocate scaled frame
        scaledFrame = av_frame_alloc();
        if (!scaledFrame) {
            Logger::warn("Failed to allocate scaled frame");
            return false;
        }

        scaledFrame->format = outputCodecCtx_->pix_fmt;
        scaledFrame->width = outputCodecCtx_->width;
        scaledFrame->height = outputCodecCtx_->height;

        if (av_frame_get_buffer(scaledFrame, 0) < 0) {
            Logger::warn("Failed to allocate scaled frame buffer");
            av_frame_free(&scaledFrame);
            return false;
        }

        // Perform scaling/conversion
        sws_scale(swsCtx_.get(),
                 inputFrame->data, inputFrame->linesize,
                 0, inputFrame->height,
                 scaledFrame->data, scaledFrame->linesize);

        scaledFrame->pts = inputFrame->pts;
        frameToEncode = scaledFrame;
    }

    // Send frame to encoder
    bool success = true;
    if (avcodec_send_frame(outputCodecCtx_.get(), frameToEncode) < 0) {
        Logger::warn("Error sending frame to encoder");
        success = false;
    }

    // Free scaled frame if it was allocated
    if (scaledFrame) {
        av_frame_free(&scaledFrame);
    }

    return success;
}

bool FFmpegWrapper::processVideoTranscode() {
    Logger::info("Processing video: TRANSCODE mode (decoding and re-encoding to H.264)");
    Logger::info("This may take longer but ensures HLS compatibility");

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (!packet || !frame) {
        Logger::error("Failed to allocate packet or frame");
        if (packet) av_packet_free(&packet);
        if (frame) av_frame_free(&frame);
        return false;
    }

    int frameCount = 0;
    int64_t nextPts = 0;

    while (av_read_frame(inputFormatCtx_, packet) >= 0) {
        if (interruptCallback_ && interruptCallback_()) {
            Logger::info("Processing interrupted by user (Ctrl+C)");
            break;
        }

        // Process video packets - transcode
        if (packet->stream_index == videoStreamIndex_) {
            if (avcodec_send_packet(inputCodecCtx_.get(), packet) < 0) {
                Logger::error("Error sending packet to decoder");
                av_packet_unref(packet);
                continue;
            }

            while (avcodec_receive_frame(inputCodecCtx_.get(), frame) == 0) {
                frameCount++;

                if (frameCount % FRAME_LOG_INTERVAL == 0) {
                    Logger::info("Transcoded " + std::to_string(frameCount) + " frames");
                }

                // Use helper function to convert and encode frame
                if (!convertAndEncodeFrame(frame, nextPts++)) {
                    av_frame_unref(frame);
                    continue;
                }

                AVPacket* outPacket = av_packet_alloc();
                while (avcodec_receive_packet(outputCodecCtx_.get(), outPacket) == 0) {
                    outPacket->stream_index = 0;

                    if (bsfCtx_) {
                        if (av_bsf_send_packet(bsfCtx_.get(), outPacket) < 0) {
                            Logger::error("Error sending packet to bitstream filter");
                            av_packet_unref(outPacket);
                            continue;
                        }

                        while (av_bsf_receive_packet(bsfCtx_.get(), outPacket) == 0) {
                            av_packet_rescale_ts(outPacket,
                                outputCodecCtx_->time_base,
                                outputFormatCtx_->streams[0]->time_base);

                            if (av_interleaved_write_frame(outputFormatCtx_.get(), outPacket) < 0) {
                                Logger::error("Error writing transcoded frame to HLS output");
                            }

                            av_packet_unref(outPacket);
                        }
                    } else {
                        av_packet_rescale_ts(outPacket,
                            outputCodecCtx_->time_base,
                            outputFormatCtx_->streams[0]->time_base);

                        if (av_interleaved_write_frame(outputFormatCtx_.get(), outPacket) < 0) {
                            Logger::error("Error writing transcoded frame to HLS output");
                        }

                        av_packet_unref(outPacket);
                    }
                }
                av_packet_free(&outPacket);

                av_frame_unref(frame);
            }
        }
        // Process audio packets
        else if (packet->stream_index == audioStreamIndex_ && outputAudioStreamIndex_ >= 0) {
            if (!audioNeedsTranscoding_) {
                // Strategy: REMUX - Copy AAC audio without transcoding (efficient)
                packet->stream_index = outputAudioStreamIndex_;

                // Rescale timestamps from input to output timebase
                av_packet_rescale_ts(packet,
                    inputFormatCtx_->streams[audioStreamIndex_]->time_base,
                    outputFormatCtx_->streams[outputAudioStreamIndex_]->time_base);

                // Write audio packet to output
                if (av_interleaved_write_frame(outputFormatCtx_.get(), packet) < 0) {
                    Logger::error("Error writing audio packet to HLS output");
                }
            } else {
                // Strategy: TRANSCODE - Convert non-AAC audio to AAC
                // Decode audio packet
                if (avcodec_send_packet(inputAudioCodecCtx_.get(), packet) < 0) {
                    Logger::error("Error sending audio packet to decoder");
                    av_packet_unref(packet);
                    continue;
                }

                AVFrame* audioFrame = av_frame_alloc();
                if (!audioFrame) {
                    Logger::error("Failed to allocate audio frame");
                    av_packet_unref(packet);
                    continue;
                }

                while (avcodec_receive_frame(inputAudioCodecCtx_.get(), audioFrame) == 0) {
                    // Convert audio format using SwrContext (decoder format -> encoder format)
                    av_frame_unref(convertedFrame_.get());

                    // Re-configure convertedFrame with output format (required by swr_convert_frame)
                    convertedFrame_->format = outputAudioCodecCtx_->sample_fmt;
                    convertedFrame_->sample_rate = outputAudioCodecCtx_->sample_rate;
                    convertedFrame_->ch_layout = outputAudioCodecCtx_->ch_layout;
                    convertedFrame_->nb_samples = 0;  // Let swr_convert_frame allocate buffer

                    int ret = FFmpegLib::swr_convert_frame(swrCtx_.get(), convertedFrame_.get(), audioFrame);
                    if (ret < 0) {
                        Logger::error("Error converting audio frame with SwrContext");
                        av_frame_unref(audioFrame);
                        continue;
                    }

                    // Rescale PTS to encoder timebase (preserves A/V sync with offsets)
                    // Use best_effort_timestamp field directly (av_frame_get_best_effort_timestamp is inline)
                    int64_t srcPts = audioFrame->best_effort_timestamp;
                    if (srcPts == AV_NOPTS_VALUE) {
                        // Fallback: generate synthetic PTS for streams without timestamps
                        if (!audioPtsInitialized_) {
                            // First frame without PTS: start at 0 to avoid initial offset
                            srcPts = 0;
                            audioPtsInitialized_ = true;
                            if (!audioPtsWarningShown_) {
                                Logger::warn("Audio stream has no PTS - generating synthetic timestamps");
                                audioPtsWarningShown_ = true;
                            }
                        } else {
                            // Subsequent frames: increment from last PTS
                            // Convert nb_samples to input stream timebase (handles containers with non-standard timebases)
                            auto* inStream = inputFormatCtx_->streams[audioStreamIndex_];
                            AVRational samplesTb = {1, inputAudioCodecCtx_->sample_rate};
                            int64_t increment = FFmpegLib::av_rescale_q(audioFrame->nb_samples, samplesTb, inStream->time_base);
                            if (increment <= 0) {
                                increment = 1;  // Defensive fallback for overflow/underflow
                            }
                            srcPts = lastAudioPts_ + increment;
                            Logger::debug("Audio frame without PTS - using synthetic timestamp");
                        }
                    } else {
                        // Valid PTS received - mark as initialized
                        audioPtsInitialized_ = true;
                    }
                    lastAudioPts_ = srcPts;

                    // Rescale from input timebase to encoder timebase
                    int64_t dstPts = FFmpegLib::av_rescale_q(
                        srcPts,
                        inputFormatCtx_->streams[audioStreamIndex_]->time_base,
                        outputAudioCodecCtx_->time_base
                    );
                    convertedFrame_->pts = dstPts;

                    // Encode converted audio frame to AAC
                    if (avcodec_send_frame(outputAudioCodecCtx_.get(), convertedFrame_.get()) < 0) {
                        Logger::error("Error sending converted audio frame to encoder");
                        av_frame_unref(audioFrame);
                        continue;
                    }

                    AVPacket* outAudioPacket = av_packet_alloc();
                    if (!outAudioPacket) {
                        Logger::error("Failed to allocate output audio packet");
                        av_frame_unref(audioFrame);
                        continue;
                    }

                    while (avcodec_receive_packet(outputAudioCodecCtx_.get(), outAudioPacket) == 0) {
                        outAudioPacket->stream_index = outputAudioStreamIndex_;

                        // Rescale timestamps
                        av_packet_rescale_ts(outAudioPacket,
                            outputAudioCodecCtx_->time_base,
                            outputFormatCtx_->streams[outputAudioStreamIndex_]->time_base);

                        // Write transcoded audio packet
                        if (av_interleaved_write_frame(outputFormatCtx_.get(), outAudioPacket) < 0) {
                            Logger::error("Error writing transcoded audio packet");
                        }

                        av_packet_unref(outAudioPacket);
                    }
                    av_packet_free(&outAudioPacket);

                    av_frame_unref(audioFrame);
                }
                av_frame_free(&audioFrame);
            }
        }

        av_packet_unref(packet);
    }

    // Drain decoder (flush remaining frames)
    avcodec_send_packet(inputCodecCtx_.get(), nullptr);
    while (avcodec_receive_frame(inputCodecCtx_.get(), frame) == 0) {
        // Use helper function to convert and encode frame (fixes conversion bug during flush)
        if (!convertAndEncodeFrame(frame, nextPts++)) {
            av_frame_unref(frame);
            continue;
        }

        AVPacket* outPacket = av_packet_alloc();
        while (avcodec_receive_packet(outputCodecCtx_.get(), outPacket) == 0) {
            outPacket->stream_index = 0;
            av_packet_rescale_ts(outPacket,
                outputCodecCtx_->time_base,
                outputFormatCtx_->streams[0]->time_base);

            av_interleaved_write_frame(outputFormatCtx_.get(), outPacket);
            av_packet_unref(outPacket);
        }
        av_packet_free(&outPacket);

        av_frame_unref(frame);
    }

    avcodec_send_frame(outputCodecCtx_.get(), nullptr);
    AVPacket* outPacket = av_packet_alloc();
    while (avcodec_receive_packet(outputCodecCtx_.get(), outPacket) == 0) {
        outPacket->stream_index = 0;
        av_packet_rescale_ts(outPacket,
            outputCodecCtx_->time_base,
            outputFormatCtx_->streams[0]->time_base);

        av_interleaved_write_frame(outputFormatCtx_.get(), outPacket);
        av_packet_unref(outPacket);
    }
    av_packet_free(&outPacket);

    // Flush audio decoder and encoder if transcoding audio
    if (audioNeedsTranscoding_ && inputAudioCodecCtx_ && outputAudioCodecCtx_ && outputAudioStreamIndex_ >= 0) {
        // Drain audio decoder (flush buffered frames)
        avcodec_send_packet(inputAudioCodecCtx_.get(), nullptr);

        AVFrame* audioFrame = av_frame_alloc();
        while (avcodec_receive_frame(inputAudioCodecCtx_.get(), audioFrame) == 0) {
            // Convert audio format using SwrContext
            av_frame_unref(convertedFrame_.get());

            // Re-configure convertedFrame with output format (required by swr_convert_frame)
            convertedFrame_->format = outputAudioCodecCtx_->sample_fmt;
            convertedFrame_->sample_rate = outputAudioCodecCtx_->sample_rate;
            convertedFrame_->ch_layout = outputAudioCodecCtx_->ch_layout;
            convertedFrame_->nb_samples = 0;  // Let swr_convert_frame allocate buffer

            int ret = FFmpegLib::swr_convert_frame(swrCtx_.get(), convertedFrame_.get(), audioFrame);
            if (ret < 0) {
                Logger::error("Error converting audio frame during flush");
                av_frame_unref(audioFrame);
                continue;
            }

            // Rescale PTS to encoder timebase (same logic as normal conversion)
            int64_t srcPts = audioFrame->best_effort_timestamp;
            if (srcPts == AV_NOPTS_VALUE) {
                if (!audioPtsInitialized_) {
                    // First frame without PTS: start at 0 to avoid initial offset
                    srcPts = 0;
                    audioPtsInitialized_ = true;
                    if (!audioPtsWarningShown_) {
                        Logger::warn("Audio stream has no PTS during flush - generating synthetic timestamps");
                        audioPtsWarningShown_ = true;
                    }
                } else {
                    // Subsequent frames: increment from last PTS
                    // Convert nb_samples to input stream timebase (handles containers with non-standard timebases)
                    auto* inStream = inputFormatCtx_->streams[audioStreamIndex_];
                    AVRational samplesTb = {1, inputAudioCodecCtx_->sample_rate};
                    int64_t increment = FFmpegLib::av_rescale_q(audioFrame->nb_samples, samplesTb, inStream->time_base);
                    if (increment <= 0) {
                        increment = 1;  // Defensive fallback for overflow/underflow
                    }
                    srcPts = lastAudioPts_ + increment;
                    Logger::debug("Audio frame without PTS during flush - using synthetic timestamp");
                }
            } else {
                // Valid PTS received - mark as initialized
                audioPtsInitialized_ = true;
            }
            lastAudioPts_ = srcPts;

            int64_t dstPts = FFmpegLib::av_rescale_q(
                srcPts,
                inputFormatCtx_->streams[audioStreamIndex_]->time_base,
                outputAudioCodecCtx_->time_base
            );
            convertedFrame_->pts = dstPts;

            // Encode converted audio frame to AAC
            if (avcodec_send_frame(outputAudioCodecCtx_.get(), convertedFrame_.get()) < 0) {
                Logger::error("Error sending converted audio frame to encoder during flush");
                av_frame_unref(audioFrame);
                continue;
            }

            // Read all encoded packets
            AVPacket* outAudioPacket = av_packet_alloc();
            while (avcodec_receive_packet(outputAudioCodecCtx_.get(), outAudioPacket) == 0) {
                outAudioPacket->stream_index = outputAudioStreamIndex_;
                av_packet_rescale_ts(outAudioPacket,
                    outputAudioCodecCtx_->time_base,
                    outputFormatCtx_->streams[outputAudioStreamIndex_]->time_base);

                av_interleaved_write_frame(outputFormatCtx_.get(), outAudioPacket);
                av_packet_unref(outAudioPacket);
            }
            av_packet_free(&outAudioPacket);

            av_frame_unref(audioFrame);
        }
        av_frame_free(&audioFrame);

        // Drain audio encoder (flush buffered packets)
        avcodec_send_frame(outputAudioCodecCtx_.get(), nullptr);

        AVPacket* outAudioPacket = av_packet_alloc();
        while (avcodec_receive_packet(outputAudioCodecCtx_.get(), outAudioPacket) == 0) {
            outAudioPacket->stream_index = outputAudioStreamIndex_;
            av_packet_rescale_ts(outAudioPacket,
                outputAudioCodecCtx_->time_base,
                outputFormatCtx_->streams[outputAudioStreamIndex_]->time_base);

            av_interleaved_write_frame(outputFormatCtx_.get(), outAudioPacket);
            av_packet_unref(outAudioPacket);
        }
        av_packet_free(&outAudioPacket);

        Logger::info("Audio decoder and encoder flushed successfully");
    }

    av_write_trailer(outputFormatCtx_.get());

    Logger::info("Transcoded " + std::to_string(frameCount) + " frames total");

    av_packet_free(&packet);
    av_frame_free(&frame);

    return true;
}

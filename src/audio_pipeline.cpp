#include "audio_pipeline.h"
#include "ffmpeg_context.h"
#include "logger.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

AudioPipeline::AudioPipeline(std::shared_ptr<FFmpegContext> ctx)
    : ffmpeg_(std::move(ctx)) {
}

AudioPipeline::~AudioPipeline() = default;

bool AudioPipeline::setupEncoder(AVStream* inStream, AVStream* outStream,
                                  int audioStreamIndex, AVFormatContext* inputFormatCtx) {
    if (!ffmpeg_ || !ffmpeg_->isInitialized()) {
        Logger::error("FFmpegContext not initialized");
        return false;
    }

    audioStreamIndex_ = audioStreamIndex;
    inputFormatCtx_ = inputFormatCtx;
    inputCodecId_ = inStream->codecpar->codec_id;

    // Decide strategy: Remux AAC or Transcode to AAC
    if (inputCodecId_ == AV_CODEC_ID_AAC) {
        Logger::info("Audio is AAC - will REMUX (copy without transcoding)");
        needsTranscoding_ = false;

        // Copy codec parameters for remux
        if (ffmpeg_->avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0) {
            Logger::error("Failed to copy audio codec parameters");
            return false;
        }
        outStream->time_base = inStream->time_base;
        return true;
    }

    // Non-AAC audio: TRANSCODE to AAC
    Logger::warn("Audio codec ID " + std::to_string(inputCodecId_) + " (non-AAC) - will TRANSCODE to AAC");
    needsTranscoding_ = true;

    // FIRST: Open audio decoder
    const AVCodec* audioDecoder = ffmpeg_->avcodec_find_decoder(inputCodecId_);
    if (!audioDecoder) {
        Logger::error("Audio decoder not found for codec " + std::to_string(inputCodecId_));
        return false;
    }

    inputCodecCtx_.reset(ffmpeg_->avcodec_alloc_context3(audioDecoder));
    if (!inputCodecCtx_) {
        Logger::error("Failed to allocate audio decoder context");
        return false;
    }

    if (ffmpeg_->avcodec_parameters_to_context(inputCodecCtx_.get(), inStream->codecpar) < 0) {
        Logger::error("Failed to copy audio codec parameters to decoder");
        return false;
    }

    if (ffmpeg_->avcodec_open2(inputCodecCtx_.get(), audioDecoder, nullptr) < 0) {
        Logger::error("Failed to open audio decoder");
        return false;
    }

    Logger::info("Audio decoder opened for transcoding");

    // SECOND: Setup AAC encoder
    const AVCodec* audioCodec = ffmpeg_->avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audioCodec) {
        Logger::error("AAC encoder not found");
        return false;
    }

    Logger::info("Setting up AAC audio encoder");

    outputCodecCtx_.reset(ffmpeg_->avcodec_alloc_context3(audioCodec));
    if (!outputCodecCtx_) {
        Logger::error("Failed to allocate audio encoder context");
        return false;
    }

    // Configure encoder from input audio stream
    outputCodecCtx_->sample_rate = inStream->codecpar->sample_rate;
    outputCodecCtx_->ch_layout = inStream->codecpar->ch_layout;
    outputCodecCtx_->sample_fmt = audioCodec->sample_fmts ? audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    outputCodecCtx_->bit_rate = 128000; // 128 kbps
    outputCodecCtx_->time_base = AVRational{1, outputCodecCtx_->sample_rate};

    // Open encoder
    if (ffmpeg_->avcodec_open2(outputCodecCtx_.get(), audioCodec, nullptr) < 0) {
        Logger::error("Failed to open audio encoder");
        return false;
    }

    // Copy parameters to output stream
    if (ffmpeg_->avcodec_parameters_from_context(outStream->codecpar, outputCodecCtx_.get()) < 0) {
        Logger::error("Failed to copy audio encoder parameters to output stream");
        return false;
    }

    outStream->time_base = outputCodecCtx_->time_base;

    Logger::info("Audio encoder configured: AAC, " +
                std::to_string(outputCodecCtx_->sample_rate) + " Hz, " +
                std::to_string(outputCodecCtx_->bit_rate / 1000) + " kbps");

    // Initialize SwrContext for audio format conversion
    SwrContext* swrCtxRaw = ffmpeg_->swr_alloc();
    if (!swrCtxRaw) {
        Logger::error("Failed to allocate SwrContext");
        return false;
    }

    int ret = ffmpeg_->swr_alloc_set_opts2(
        &swrCtxRaw,
        &outputCodecCtx_->ch_layout,
        outputCodecCtx_->sample_fmt,
        outputCodecCtx_->sample_rate,
        &inputCodecCtx_->ch_layout,
        inputCodecCtx_->sample_fmt,
        inputCodecCtx_->sample_rate,
        0, nullptr);

    if (ret < 0) {
        Logger::error("Failed to configure SwrContext");
        ffmpeg_->swr_free(&swrCtxRaw);
        return false;
    }

    if (ffmpeg_->swr_init(swrCtxRaw) < 0) {
        Logger::error("Failed to initialize SwrContext");
        ffmpeg_->swr_free(&swrCtxRaw);
        return false;
    }

    swrCtx_.reset(swrCtxRaw);

    // Allocate cached frame for conversion
    convertedFrame_.reset(ffmpeg_->av_frame_alloc());
    if (!convertedFrame_) {
        Logger::error("Failed to allocate converted audio frame");
        return false;
    }

    Logger::info("Audio resampler initialized successfully");

    return true;
}

void AudioPipeline::assignPTS(AVFrame* inputFrame, AVFrame* convertedFrame) {
    // Use best_effort_timestamp field directly
    int64_t srcPts = inputFrame->best_effort_timestamp;

    if (srcPts == AV_NOPTS_VALUE) {
        // Fallback: generate synthetic PTS for streams without timestamps
        if (!ptsState_.initialized) {
            // First frame without PTS: start at 0 to avoid initial offset
            srcPts = 0;
            ptsState_.initialized = true;
            if (!ptsState_.warningShown) {
                Logger::warn("Audio stream has no PTS - generating synthetic timestamps");
                ptsState_.warningShown = true;
            }
        } else {
            // Subsequent frames: increment from last PTS
            // Convert nb_samples to input stream timebase
            auto* inStream = inputFormatCtx_->streams[audioStreamIndex_];
            AVRational samplesTb = {1, inputCodecCtx_->sample_rate};
            int64_t increment = ffmpeg_->av_rescale_q(inputFrame->nb_samples, samplesTb, inStream->time_base);
            if (increment <= 0) {
                increment = 1;  // Defensive fallback
            }
            srcPts = ptsState_.lastPts + increment;
            Logger::debug("Audio frame without PTS - using synthetic timestamp");
        }
    } else {
        // Valid PTS received
        ptsState_.initialized = true;
    }

    ptsState_.lastPts = srcPts;

    // Rescale from input timebase to encoder timebase
    int64_t dstPts = ffmpeg_->av_rescale_q(
        srcPts,
        inputFormatCtx_->streams[audioStreamIndex_]->time_base,
        outputCodecCtx_->time_base
    );
    convertedFrame->pts = dstPts;
}

bool AudioPipeline::processDecodedFrame(AVFrame* audioFrame,
                                         AVFormatContext* outputFormatCtx,
                                         int outputAudioStreamIndex) {
    // Convert audio format using SwrContext
    ffmpeg_->av_frame_unref(convertedFrame_.get());

    // Re-configure convertedFrame with output format
    convertedFrame_->format = outputCodecCtx_->sample_fmt;
    convertedFrame_->sample_rate = outputCodecCtx_->sample_rate;
    convertedFrame_->ch_layout = outputCodecCtx_->ch_layout;
    convertedFrame_->nb_samples = 0;  // Let swr_convert_frame allocate buffer

    int ret = ffmpeg_->swr_convert_frame(swrCtx_.get(), convertedFrame_.get(), audioFrame);
    if (ret < 0) {
        Logger::error("Error converting audio frame with SwrContext");
        return false;
    }

    // Assign PTS (handles synthetic generation)
    assignPTS(audioFrame, convertedFrame_.get());

    // Encode converted audio frame to AAC
    if (ffmpeg_->avcodec_send_frame(outputCodecCtx_.get(), convertedFrame_.get()) < 0) {
        Logger::error("Error sending converted audio frame to encoder");
        return false;
    }

    AVPacket* outAudioPacket = ffmpeg_->av_packet_alloc();
    if (!outAudioPacket) {
        Logger::error("Failed to allocate output audio packet");
        return false;
    }

    while (ffmpeg_->avcodec_receive_packet(outputCodecCtx_.get(), outAudioPacket) == 0) {
        outAudioPacket->stream_index = outputAudioStreamIndex;

        // Rescale timestamps
        ffmpeg_->av_packet_rescale_ts(outAudioPacket,
            outputCodecCtx_->time_base,
            outputFormatCtx->streams[outputAudioStreamIndex]->time_base);

        // Write transcoded audio packet
        if (ffmpeg_->av_interleaved_write_frame(outputFormatCtx, outAudioPacket) < 0) {
            Logger::error("Error writing transcoded audio packet");
        }

        ffmpeg_->av_packet_unref(outAudioPacket);
    }
    ffmpeg_->av_packet_free(&outAudioPacket);

    return true;
}

bool AudioPipeline::processPacket(AVPacket* packet,
                                   AVFormatContext* inputFormatCtx,
                                   AVFormatContext* outputFormatCtx,
                                   int audioStreamIndex,
                                   int outputAudioStreamIndex) {
    if (!needsTranscoding_) {
        // Strategy: REMUX - Copy AAC audio without transcoding
        packet->stream_index = outputAudioStreamIndex;

        // Rescale timestamps from input to output timebase
        ffmpeg_->av_packet_rescale_ts(packet,
            inputFormatCtx->streams[audioStreamIndex]->time_base,
            outputFormatCtx->streams[outputAudioStreamIndex]->time_base);

        // Write audio packet to output
        if (ffmpeg_->av_interleaved_write_frame(outputFormatCtx, packet) < 0) {
            Logger::error("Error writing audio packet to HLS output");
            return false;
        }
        return true;
    }

    // Strategy: TRANSCODE - Convert non-AAC audio to AAC
    if (ffmpeg_->avcodec_send_packet(inputCodecCtx_.get(), packet) < 0) {
        Logger::error("Error sending audio packet to decoder");
        return false;
    }

    AVFrame* audioFrame = ffmpeg_->av_frame_alloc();
    if (!audioFrame) {
        Logger::error("Failed to allocate audio frame");
        return false;
    }

    bool success = true;
    while (ffmpeg_->avcodec_receive_frame(inputCodecCtx_.get(), audioFrame) == 0) {
        if (!processDecodedFrame(audioFrame, outputFormatCtx, outputAudioStreamIndex)) {
            success = false;
        }
        ffmpeg_->av_frame_unref(audioFrame);
    }
    ffmpeg_->av_frame_free(&audioFrame);

    return success;
}

bool AudioPipeline::flush(AVFormatContext* outputFormatCtx, int outputAudioStreamIndex) {
    if (!needsTranscoding_ || !inputCodecCtx_ || !outputCodecCtx_) {
        return true;  // Nothing to flush in remux mode
    }

    Logger::info("Flushing audio pipeline");

    // Drain audio decoder
    ffmpeg_->avcodec_send_packet(inputCodecCtx_.get(), nullptr);

    AVFrame* audioFrame = ffmpeg_->av_frame_alloc();
    if (!audioFrame) {
        Logger::error("Failed to allocate audio frame for flush");
        return false;
    }

    while (ffmpeg_->avcodec_receive_frame(inputCodecCtx_.get(), audioFrame) == 0) {
        processDecodedFrame(audioFrame, outputFormatCtx, outputAudioStreamIndex);
        ffmpeg_->av_frame_unref(audioFrame);
    }
    ffmpeg_->av_frame_free(&audioFrame);

    // Drain audio encoder
    ffmpeg_->avcodec_send_frame(outputCodecCtx_.get(), nullptr);

    AVPacket* outAudioPacket = ffmpeg_->av_packet_alloc();
    if (!outAudioPacket) {
        Logger::error("Failed to allocate packet for audio encoder flush");
        return false;
    }

    while (ffmpeg_->avcodec_receive_packet(outputCodecCtx_.get(), outAudioPacket) == 0) {
        outAudioPacket->stream_index = outputAudioStreamIndex;

        ffmpeg_->av_packet_rescale_ts(outAudioPacket,
            outputCodecCtx_->time_base,
            outputFormatCtx->streams[outputAudioStreamIndex]->time_base);

        ffmpeg_->av_interleaved_write_frame(outputFormatCtx, outAudioPacket);
        ffmpeg_->av_packet_unref(outAudioPacket);
    }
    ffmpeg_->av_packet_free(&outAudioPacket);

    Logger::info("Audio pipeline flushed successfully");
    return true;
}

void AudioPipeline::reset() {
    inputCodecCtx_.reset();
    outputCodecCtx_.reset();
    swrCtx_.reset();
    convertedFrame_.reset();

    needsTranscoding_ = false;
    inputCodecId_ = 0;
    audioStreamIndex_ = -1;
    inputFormatCtx_ = nullptr;

    ptsState_.reset();

    Logger::info("AudioPipeline reset");
}

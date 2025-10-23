#include "video_pipeline.h"
#include "ffmpeg_context.h"
#include "logger.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

VideoPipeline::VideoPipeline(std::shared_ptr<FFmpegContext> ctx)
    : ffmpeg_(std::move(ctx)) {
}

VideoPipeline::~VideoPipeline() = default;

VideoPipeline::Mode VideoPipeline::detectMode(AVStream* inStream) {
    inputCodecId_ = inStream->codecpar->codec_id;

    const AVCodec* decoder = ffmpeg_->avcodec_find_decoder(inputCodecId_);
    if (decoder) {
        inputCodecName_ = decoder->name;
    } else {
        inputCodecName_ = "unknown";
    }

    Logger::info("Input video codec: " + inputCodecName_ + " (ID: " + std::to_string(inputCodecId_) + ")");

    // Check if codec is HLS-compatible (H.264)
    if (inputCodecId_ == AV_CODEC_ID_H264) {
        Logger::info("Input is H.264 - will use REMUX mode (fast, no quality loss)");
        mode_ = Mode::REMUX;
        return mode_;
    }

    Logger::warn("Input codec '" + inputCodecName_ + "' is not HLS-compatible");
    Logger::warn("Will TRANSCODE to H.264 (this may take longer)");
    mode_ = Mode::TRANSCODE;
    return mode_;
}

bool VideoPipeline::setupDecoder(AVStream* inStream) {
    const AVCodec* decoder = ffmpeg_->avcodec_find_decoder(inStream->codecpar->codec_id);
    if (!decoder) {
        Logger::error("Failed to find decoder");
        return false;
    }

    Logger::info("Input codec: " + std::string(decoder->name));

    inputCodecCtx_.reset(ffmpeg_->avcodec_alloc_context3(decoder));
    if (!inputCodecCtx_) {
        Logger::error("Failed to allocate decoder context");
        return false;
    }

    if (ffmpeg_->avcodec_parameters_to_context(inputCodecCtx_.get(), inStream->codecpar) < 0) {
        Logger::error("Failed to copy codec parameters");
        return false;
    }

    if (ffmpeg_->avcodec_open2(inputCodecCtx_.get(), decoder, nullptr) < 0) {
        Logger::error("Failed to open decoder");
        return false;
    }

    return true;
}

bool VideoPipeline::setupEncoder(AVStream* outStream, const AppConfig& config) {
    config_ = config;

    const AVCodec* encoder = ffmpeg_->avcodec_find_encoder_by_name("libx264");
    if (!encoder) {
        Logger::warn("libx264 not found, trying default H.264 encoder");
        encoder = ffmpeg_->avcodec_find_encoder(AV_CODEC_ID_H264);
    }

    if (!encoder) {
        Logger::error("H.264 encoder not found");
        return false;
    }

    Logger::info("Using encoder: " + std::string(encoder->name));

    outputCodecCtx_.reset(ffmpeg_->avcodec_alloc_context3(encoder));
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
    outputCodecCtx_->max_b_frames = 0;  // No B-frames for HLS streaming

    ffmpeg_->av_opt_set(outputCodecCtx_->priv_data, "preset", "ultrafast", 0);
    ffmpeg_->av_opt_set(outputCodecCtx_->priv_data, "tune", "zerolatency", 0);

    if (ffmpeg_->avcodec_open2(outputCodecCtx_.get(), encoder, nullptr) < 0) {
        Logger::error("Failed to open encoder");
        return false;
    }

    // Copy parameters to output stream
    if (ffmpeg_->avcodec_parameters_from_context(outStream->codecpar, outputCodecCtx_.get()) < 0) {
        Logger::error("Failed to copy encoder parameters to output stream");
        return false;
    }

    outStream->time_base = outputCodecCtx_->time_base;

    return true;
}

bool VideoPipeline::setupBitstreamFilter(AVStream* inStream, AVStream* outStream,
                                          Mode mode, AVRational outputCodecTimeBase) {
    Logger::info("Setting up h264_mp4toannexb bitstream filter");

    const AVBitStreamFilter* bsf = ffmpeg_->av_bsf_get_by_name("h264_mp4toannexb");
    if (!bsf) {
        Logger::error("Failed to find h264_mp4toannexb filter");
        return false;
    }

    AVBSFContext* temp_bsf_ctx = nullptr;
    if (ffmpeg_->av_bsf_alloc(bsf, &temp_bsf_ctx) < 0) {
        Logger::error("Failed to allocate bitstream filter context");
        return false;
    }
    bsfCtx_.reset(temp_bsf_ctx);

    AVCodecParameters* codecParams = nullptr;
    AVRational timeBase;

    if (mode == Mode::REMUX || mode == Mode::PROGRAMMATIC) {
        codecParams = inStream->codecpar;
        timeBase = inStream->time_base;
    } else {
        codecParams = outStream->codecpar;
        timeBase = outputCodecTimeBase;
    }

    if (ffmpeg_->avcodec_parameters_copy(bsfCtx_->par_in, codecParams) < 0) {
        Logger::error("Failed to copy codec parameters to filter");
        bsfCtx_.reset();
        return false;
    }

    bsfCtx_->time_base_in = timeBase;

    if (ffmpeg_->av_bsf_init(bsfCtx_.get()) < 0) {
        Logger::error("Failed to initialize bitstream filter");
        bsfCtx_.reset();
        return false;
    }

    Logger::info("Bitstream filter initialized successfully");
    return true;
}

bool VideoPipeline::convertAndEncodeFrame(AVFrame* inputFrame, int64_t pts) {
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
        SwsContext* newCtx = ffmpeg_->sws_getCachedContext(
            swsCtx_.get(),
            inputFrame->width, inputFrame->height, (AVPixelFormat)inputFrame->format,
            outputCodecCtx_->width, outputCodecCtx_->height, outputCodecCtx_->pix_fmt,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!newCtx) {
            Logger::error("Failed to get/create video scaler context");
            return false;
        }

        if (newCtx != swsCtx_.get()) {
            Logger::info("Video scaler " + std::string(swsCtx_ ? "recreated" : "initialized") + ": " +
                       std::to_string(inputFrame->width) + "x" + std::to_string(inputFrame->height) + " " +
                       "fmt=" + std::to_string(inputFrame->format) + " -> " +
                       std::to_string(outputCodecCtx_->width) + "x" + std::to_string(outputCodecCtx_->height) + " " +
                       "fmt=" + std::to_string(outputCodecCtx_->pix_fmt));
            swsCtx_.reset(newCtx);
        }

        // Allocate scaled frame
        scaledFrame = ffmpeg_->av_frame_alloc();
        if (!scaledFrame) {
            Logger::warn("Failed to allocate scaled frame");
            return false;
        }

        scaledFrame->format = outputCodecCtx_->pix_fmt;
        scaledFrame->width = outputCodecCtx_->width;
        scaledFrame->height = outputCodecCtx_->height;

        if (ffmpeg_->av_frame_get_buffer(scaledFrame, 0) < 0) {
            Logger::warn("Failed to allocate scaled frame buffer");
            ffmpeg_->av_frame_free(&scaledFrame);
            return false;
        }

        // Perform scaling/conversion
        ffmpeg_->sws_scale(swsCtx_.get(),
                 inputFrame->data, inputFrame->linesize,
                 0, inputFrame->height,
                 scaledFrame->data, scaledFrame->linesize);

        scaledFrame->pts = inputFrame->pts;
        frameToEncode = scaledFrame;
    }

    // Send frame to encoder
    bool success = true;
    if (ffmpeg_->avcodec_send_frame(outputCodecCtx_.get(), frameToEncode) < 0) {
        Logger::warn("Error sending frame to encoder");
        success = false;
    }

    // Free scaled frame if it was allocated
    if (scaledFrame) {
        ffmpeg_->av_frame_free(&scaledFrame);
    }

    return success;
}

bool VideoPipeline::processBitstreamFilter(AVPacket* packet,
                                            AVFormatContext* outputFormatCtx,
                                            int outputVideoStreamIndex,
                                            AVRational inputTimeBase,
                                            AVRational outputTimeBase) {
    if (!bsfCtx_) {
        // No bitstream filter, just rescale and write
        packet->stream_index = outputVideoStreamIndex;
        ffmpeg_->av_packet_rescale_ts(packet, inputTimeBase, outputTimeBase);

        if (ffmpeg_->av_interleaved_write_frame(outputFormatCtx, packet) < 0) {
            Logger::error("Error writing video frame to HLS output");
            return false;
        }
        return true;
    }

    // Send packet to bitstream filter
    if (ffmpeg_->av_bsf_send_packet(bsfCtx_.get(), packet) < 0) {
        Logger::error("Error sending packet to bitstream filter");
        return false;
    }

    // Receive filtered packets
    while (ffmpeg_->av_bsf_receive_packet(bsfCtx_.get(), packet) == 0) {
        packet->stream_index = outputVideoStreamIndex;
        ffmpeg_->av_packet_rescale_ts(packet, inputTimeBase, outputTimeBase);

        if (ffmpeg_->av_interleaved_write_frame(outputFormatCtx, packet) < 0) {
            Logger::error("Error writing video frame to HLS output");
        }

        ffmpeg_->av_packet_unref(packet);
    }

    return true;
}

bool VideoPipeline::flushEncoder(AVFormatContext* outputFormatCtx, int outputVideoStreamIndex) {
    if (!outputCodecCtx_) {
        return true;
    }

    Logger::info("Flushing video encoder");

    // Drain encoder
    ffmpeg_->avcodec_send_frame(outputCodecCtx_.get(), nullptr);

    AVPacket* outPacket = ffmpeg_->av_packet_alloc();
    if (!outPacket) {
        Logger::error("Failed to allocate packet for encoder flush");
        return false;
    }

    while (ffmpeg_->avcodec_receive_packet(outputCodecCtx_.get(), outPacket) == 0) {
        outPacket->stream_index = outputVideoStreamIndex;
        ffmpeg_->av_packet_rescale_ts(outPacket,
            outputCodecCtx_->time_base,
            outputFormatCtx->streams[outputVideoStreamIndex]->time_base);

        ffmpeg_->av_interleaved_write_frame(outputFormatCtx, outPacket);
        ffmpeg_->av_packet_unref(outPacket);
    }
    ffmpeg_->av_packet_free(&outPacket);

    Logger::info("Video encoder flushed successfully");
    return true;
}

bool VideoPipeline::flushBitstreamFilter(AVFormatContext* outputFormatCtx,
                                          int outputVideoStreamIndex,
                                          AVRational inputTimeBase,
                                          AVRational outputTimeBase) {
    if (!bsfCtx_) {
        return true;
    }

    Logger::info("Flushing bitstream filter");

    AVPacket* packet = ffmpeg_->av_packet_alloc();
    if (!packet) {
        Logger::error("Failed to allocate packet for BSF flush");
        return false;
    }

    ffmpeg_->av_bsf_send_packet(bsfCtx_.get(), nullptr);
    while (ffmpeg_->av_bsf_receive_packet(bsfCtx_.get(), packet) == 0) {
        packet->stream_index = outputVideoStreamIndex;
        ffmpeg_->av_packet_rescale_ts(packet, inputTimeBase, outputTimeBase);

        ffmpeg_->av_interleaved_write_frame(outputFormatCtx, packet);
        ffmpeg_->av_packet_unref(packet);
    }

    ffmpeg_->av_packet_free(&packet);
    Logger::info("Bitstream filter flushed successfully");
    return true;
}

void VideoPipeline::reset() {
    inputCodecCtx_.reset();
    outputCodecCtx_.reset();
    bsfCtx_.reset();
    swsCtx_.reset();

    mode_ = Mode::REMUX;
    inputCodecId_ = 0;
    inputCodecName_.clear();

    Logger::info("VideoPipeline reset");
}

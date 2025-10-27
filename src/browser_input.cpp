#include "browser_input.h"
#include "browser_backend.h"
#include "cef_backend.h"
#include "ffmpeg_context.h"
#include "logger.h"

#include <chrono>
#include <thread>
#include <cstring>

BrowserInput::BrowserInput(const AppConfig& config, std::shared_ptr<FFmpegContext> ffmpegCtx)
    : ffmpeg_(ffmpegCtx)
    , config_(config)
    , video_stream_index_(0)
    , audio_stream_index_(-1)
    , frame_ready_(false)
    , resetting_encoders_(false)
    , received_real_frame_(false)
    , frame_count_(0)
    , start_time_ms_(0)
    , snapshot_width_(0)
    , snapshot_height_(0)
    , audio_channels_(0)
    , audio_sample_rate_(0)
    , audio_samples_written_(0)
    , audio_start_pts_(-1)
    , audio_stream_started_(false)
    , initialized_(false)
    , running_(false) {
}

BrowserInput::~BrowserInput() {
    close();
}

bool BrowserInput::open(const std::string& uri) {
    Logger::info("Opening browser input: " + uri);
    Logger::info("Browser resolution: " + std::to_string(config_.video.width) + "x" + std::to_string(config_.video.height) + " @ " + std::to_string(config_.video.fps) + "fps");

    // Store URI for CEF initialization
    pending_uri_ = uri;

    // Setup encoders immediately (fast, synchronous)
    if (!setupEncoder()) {
        Logger::error("Failed to setup video encoder");
        return false;
    }

    if (!setupAudioEncoder(config_.audio.sample_rate, config_.audio.channels)) {
        Logger::error("Failed to setup audio encoder - stream will have no audio");
    } else {
        Logger::info("Audio encoder initialized (" + std::to_string(config_.audio.sample_rate) + "Hz, " + std::to_string(config_.audio.channels) + " channels)");
    }

    initialized_ = true;
    running_ = true;
    start_time_ms_ = 0;

    // Initialize CEF immediately
    Logger::info("Browser input opened - initializing CEF directly");
    tryInitializeCEF();
    return true;
}

void BrowserInput::close() {
    if (!initialized_) {
        return;
    }

    running_ = false;

    if (backend_) {
        backend_->shutdown();
        backend_.reset();
    }

    format_ctx_.reset();
    codec_ctx_.reset();
    audio_codec_ctx_.reset();
    sws_ctx_.reset();
    yuv_frame_.reset();
    audio_frame_.reset();

    initialized_ = false;
    Logger::info("Browser input closed");
}

bool BrowserInput::readPacket(AVPacket* packet) {
    if (!initialized_ || !running_) {
        return false;
    }

    int64_t current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Initialize timing on first call
    if (start_time_ms_ == 0) {
        start_time_ms_ = current_time_ms;
    }

    // Process CEF backend events and frames
    bool cef_is_ready = cef_initialized_.load();
    if (cef_is_ready && backend_) {
        backend_->processEvents();
        pullAudioFromBackend();

        CEFBackend* cef = dynamic_cast<CEFBackend*>(backend_.get());
        if (cef && cef->hasLoadError()) {
            Logger::error("Browser failed to load page - stopping");
            return false;
        }

        if (cef && cef->checkAndClearPageReload()) {
            Logger::info("Page reload detected - resetting muxer and encoders");
            if (pageReloadCallback_ && !pageReloadCallback_()) {
                Logger::error("Failed to reset output muxer on page reload");
                return false;
            }
            if (!resetEncoders()) {
                Logger::error("Failed to reset encoders on page reload");
                return false;
            }
            Logger::info("Page reload handling complete");
        }
    }

    bool has_frame = false;
    if (cef_is_ready) {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        has_frame = frame_ready_ && !current_frame_.empty();
    }

    int64_t elapsed_ms = current_time_ms - start_time_ms_;
    int64_t expected_frame = (elapsed_ms * config_.video.fps) / 1000;

    bool throttle = frame_count_ >= expected_frame;

    if (throttle) {
        // Try to emit audio while we wait for the next video frame
        if (audio_codec_ctx_ && hasAudioData() && encodeAudio(packet)) {
            return true;
        }
        if (!has_frame) {
            // No fresh video frame yet – yield briefly and try again
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return true;
        }
        // Otherwise fall through to process the available video frame
    } else if (audio_codec_ctx_ && hasAudioData() && encodeAudio(packet)) {
        return true;
    }

    if (!has_frame) {
        if (cef_is_ready && backend_) {
            CEFBackend* cef = dynamic_cast<CEFBackend*>(backend_.get());
            if (cef && backend_->isPageLoaded()) {
                cef->invalidate();
            }
        }
        // Reduced sleep to minimize frame drops (2ms instead of 10ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return true;
    }

    std::vector<uint8_t> frame_copy;
    int snap_width, snap_height;
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        frame_copy = current_frame_;
        snap_width = snapshot_width_;
        snap_height = snapshot_height_;
        frame_ready_ = false;
    }

    if (!convertBGRAtoYUVWithCrop(frame_copy.data(), snap_width, snap_height, yuv_frame_.get())) {
        Logger::error("Failed to convert BGRA to YUV");
        return false;
    }

    if (start_time_ms_ == 0) {
        start_time_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (frame_count_ == 0) {
            Logger::info("Video & audio clock started: First video frame being generated");
        } else {
            Logger::info("Video & audio clock restarted after page reload (video frame #" + std::to_string(frame_count_) + ", audio samples #" + std::to_string(audio_samples_written_) + ")");
        }
    }

    // Use frame_count_ for PTS to guarantee monotonically increasing timestamps
    // This is the correct approach: each frame gets a unique, sequential PTS
    // The encoder's timebase {1, fps} means PTS increments match real time
    yuv_frame_->pts = frame_count_;

    if (frame_count_ == 0) {
        Logger::info("Video started: First video frame generated (frame #0)");
    } else if (frame_count_ % 30 == 0) {
        Logger::info("Video frame #" + std::to_string(frame_count_) +
                   " generated (1 second of video)");
    }

    if (!encodeFrame(yuv_frame_.get(), packet)) {
        return false;
    }

    // Mark that we successfully encoded the first real frame
    if (!received_real_frame_.load()) {
        received_real_frame_ = true;
        Logger::info("First real video frame encoded successfully");
    }

    frame_count_++;
    return true;
}

AVFormatContext* BrowserInput::getFormatContext() {
    return format_ctx_.get();
}

int BrowserInput::getVideoStreamIndex() const {
    return video_stream_index_;
}

int BrowserInput::getAudioStreamIndex() const {
    return audio_stream_index_;
}

bool BrowserInput::isLiveStream() const {
    return true;
}

std::string BrowserInput::getTypeName() const {
    if (backend_) {
        return std::string("Browser (") + backend_->getName() + ")";
    }
    return "Browser";
}

bool BrowserInput::setupEncoder(bool is_reset) {
    if (!is_reset) {
        AVFormatContext* temp_format_ctx = nullptr;
        if (ffmpeg_->avformat_alloc_output_context2(&temp_format_ctx, nullptr, "mpegts", nullptr) < 0) {
            Logger::error("Failed to allocate output format context");
            return false;
        }
        format_ctx_ = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>(temp_format_ctx, AVFormatContextDeleter(ffmpeg_));
    }

    const AVCodec* codec = ffmpeg_->avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        Logger::error("H.264 codec not found");
        return false;
    }

    codec_ctx_ = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>(ffmpeg_->avcodec_alloc_context3(codec), AVCodecContextDeleter(ffmpeg_));
    if (!codec_ctx_) {
        Logger::error("Failed to allocate codec context");
        return false;
    }

    codec_ctx_->width = config_.video.width;
    codec_ctx_->height = config_.video.height;
    codec_ctx_->time_base = AVRational{1, config_.video.fps};
    codec_ctx_->framerate = AVRational{config_.video.fps, 1};
    codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx_->bit_rate = config_.video.bitrate;
    codec_ctx_->gop_size = config_.video.gop_size;
    codec_ctx_->max_b_frames = 0;

    ffmpeg_->av_opt_set(codec_ctx_->priv_data, "preset", "ultrafast", 0);
    ffmpeg_->av_opt_set(codec_ctx_->priv_data, "tune", "zerolatency", 0);

    if (ffmpeg_->avcodec_open2(codec_ctx_.get(), codec, nullptr) < 0) {
        Logger::error("Failed to open codec");
        return false;
    }

    if (!is_reset) {
        AVStream* stream = ffmpeg_->avformat_new_stream(format_ctx_.get(), nullptr);
        if (!stream) {
            Logger::error("Failed to create video stream");
            return false;
        }

        stream->time_base = codec_ctx_->time_base;
        ffmpeg_->avcodec_parameters_from_context(stream->codecpar, codec_ctx_.get());
        video_stream_index_ = stream->index;
    } else {
        // During reset, update existing stream's codec parameters
        if (video_stream_index_ >= 0 && video_stream_index_ < (int)format_ctx_->nb_streams) {
            AVStream* stream = format_ctx_->streams[video_stream_index_];
            stream->time_base = codec_ctx_->time_base;
            ffmpeg_->avcodec_parameters_from_context(stream->codecpar, codec_ctx_.get());
        }
    }

    yuv_frame_ = std::unique_ptr<AVFrame, AVFrameDeleter>(ffmpeg_->av_frame_alloc(), AVFrameDeleter(ffmpeg_));
    if (!yuv_frame_) {
        Logger::error("Failed to allocate YUV frame");
        return false;
    }

    yuv_frame_->format = AV_PIX_FMT_YUV420P;
    yuv_frame_->width = config_.video.width;
    yuv_frame_->height = config_.video.height;

    if (ffmpeg_->av_frame_get_buffer(yuv_frame_.get(), 32) < 0) {
        Logger::error("Failed to allocate YUV frame buffer");
        return false;
    }

    sws_ctx_ = std::unique_ptr<SwsContext, SwsContextDeleter>(ffmpeg_->sws_getContext(
        config_.video.width, config_.video.height, AV_PIX_FMT_BGRA,
        config_.video.width, config_.video.height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr), SwsContextDeleter(ffmpeg_));

    if (!sws_ctx_) {
        Logger::error("Failed to create swscale context");
        return false;
    }

    Logger::info("Video encoder setup complete");
    Logger::info("  - Resolution: " + std::to_string(config_.video.width) + "x" + std::to_string(config_.video.height));
    Logger::info("  - Frame rate: " + std::to_string(config_.video.fps) + " fps");
    Logger::info("  - Codec: H.264");

    return true;
}

bool BrowserInput::resetEncoders() {
    std::lock_guard<std::mutex> lock(encoder_mutex_);
    resetting_encoders_ = true;

    Logger::info("Resetting encoders: Recreating video and audio encoders (keeping CEF alive)");

    if (codec_ctx_) {
        AVPacket* temp_pkt = ffmpeg_->av_packet_alloc();
        ffmpeg_->avcodec_send_frame(codec_ctx_.get(), nullptr);
        while (ffmpeg_->avcodec_receive_packet(codec_ctx_.get(), temp_pkt) == 0) {
            ffmpeg_->av_packet_unref(temp_pkt);
        }
        ffmpeg_->av_packet_free(&temp_pkt);
        codec_ctx_.reset();
        Logger::info("Flushed and freed old video encoder");
    }

    if (audio_codec_ctx_) {
        AVPacket* temp_pkt = ffmpeg_->av_packet_alloc();
        ffmpeg_->avcodec_send_frame(audio_codec_ctx_.get(), nullptr);
        while (ffmpeg_->avcodec_receive_packet(audio_codec_ctx_.get(), temp_pkt) == 0) {
            ffmpeg_->av_packet_unref(temp_pkt);
        }
        ffmpeg_->av_packet_free(&temp_pkt);
        audio_codec_ctx_.reset();
        Logger::info("Flushed and freed old audio encoder");
    }

    if (!setupEncoder(true)) {
        Logger::error("Failed to recreate video encoder");
        resetting_encoders_ = false;
        return false;
    }
    Logger::info("Video encoder recreated");

    if (audio_sample_rate_ > 0 && audio_channels_ > 0) {
        if (!setupAudioEncoder(audio_sample_rate_, audio_channels_, true)) {
            Logger::error("Failed to recreate audio encoder");
            resetting_encoders_ = false;
            return false;
        }
        Logger::info("Audio encoder recreated");
    }

    frame_count_ = 0;
    audio_samples_written_ = 0;
    start_time_ms_ = 0;
    audio_start_pts_ = -1;
    audio_buffer_.clear();
    received_real_frame_ = false;  // Reset frame tracking after page reload
    Logger::info("Reset PTS counters, cleared audio buffer, and reset frame tracking");

    resetting_encoders_ = false;
    Logger::info("Encoders reset complete");
    return true;
}

bool BrowserInput::setupAudioEncoder(int sample_rate, int channels, bool is_reset) {
    Logger::info("Setting up AAC audio encoder: " + std::to_string(channels) + " channels @ " +
                 std::to_string(sample_rate) + " Hz");

    const AVCodec* codec = ffmpeg_->avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        Logger::error("AAC encoder not found");
        return false;
    }

    audio_codec_ctx_ = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>(ffmpeg_->avcodec_alloc_context3(codec), AVCodecContextDeleter(ffmpeg_));
    if (!audio_codec_ctx_) {
        Logger::error("Failed to allocate audio codec context");
        return false;
    }

    audio_codec_ctx_->sample_rate = sample_rate;
    if (channels == 1) {
        audio_codec_ctx_->ch_layout = AV_CHANNEL_LAYOUT_MONO;
    } else {
        audio_codec_ctx_->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    }
    audio_codec_ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;
    audio_codec_ctx_->bit_rate = config_.audio.bitrate;
    audio_codec_ctx_->time_base = AVRational{1, sample_rate};

    if (ffmpeg_->avcodec_open2(audio_codec_ctx_.get(), codec, nullptr) < 0) {
        Logger::error("Failed to open AAC encoder");
        audio_codec_ctx_ = nullptr;
        return false;
    }

    if (!is_reset) {
        AVStream* audio_stream = ffmpeg_->avformat_new_stream(format_ctx_.get(), nullptr);
        if (!audio_stream) {
            Logger::error("Failed to create audio stream");
            return false;
        }

        audio_stream->time_base = audio_codec_ctx_->time_base;
        ffmpeg_->avcodec_parameters_from_context(audio_stream->codecpar, audio_codec_ctx_.get());
        audio_stream_index_ = audio_stream->index;
        Logger::info("Created audio stream at index " + std::to_string(audio_stream_index_));
    } else {
        // During reset, update existing stream's codec parameters
        if (audio_stream_index_ >= 0 && audio_stream_index_ < (int)format_ctx_->nb_streams) {
            AVStream* audio_stream = format_ctx_->streams[audio_stream_index_];
            audio_stream->time_base = audio_codec_ctx_->time_base;
            ffmpeg_->avcodec_parameters_from_context(audio_stream->codecpar, audio_codec_ctx_.get());
        }
    }

    audio_frame_ = std::unique_ptr<AVFrame, AVFrameDeleter>(ffmpeg_->av_frame_alloc(), AVFrameDeleter(ffmpeg_));
    if (!audio_frame_) {
        Logger::error("Failed to allocate audio frame");
        return false;
    }

    audio_frame_->format = audio_codec_ctx_->sample_fmt;
    audio_frame_->ch_layout = audio_codec_ctx_->ch_layout;
    audio_frame_->sample_rate = audio_codec_ctx_->sample_rate;
    audio_frame_->nb_samples = audio_codec_ctx_->frame_size;

    if (ffmpeg_->av_frame_get_buffer(audio_frame_.get(), 0) < 0) {
        Logger::error("Failed to allocate audio frame buffer");
        return false;
    }

    audio_channels_ = channels;
    audio_sample_rate_ = sample_rate;

    Logger::info("AAC audio encoder initialized successfully");
    return true;
}

bool BrowserInput::convertBGRAtoYUVWithCrop(const uint8_t* bgra_data, int src_width, int src_height, AVFrame* yuv_frame) {
    if (!bgra_data || !yuv_frame || !sws_ctx_) {
        return false;
    }

    const uint8_t* src_data[4] = { bgra_data, nullptr, nullptr, nullptr };
    int src_linesize[4] = { src_width * 4, 0, 0, 0 };

    int ret = ffmpeg_->sws_scale(
        sws_ctx_.get(),
        src_data,
        src_linesize,
        0,
        std::min(src_height, (int)config_.video.height),
        yuv_frame->data,
        yuv_frame->linesize
    );

    return ret > 0;
}

bool BrowserInput::convertBGRAtoYUV(const uint8_t* bgra_data, AVFrame* yuv_frame) {
    return convertBGRAtoYUVWithCrop(bgra_data, config_.video.width, config_.video.height, yuv_frame);
}

bool BrowserInput::encodeFrame(AVFrame* frame, AVPacket* packet) {
    if (resetting_encoders_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(encoder_mutex_);

    if (!codec_ctx_) {
        return false;
    }

    int ret = ffmpeg_->avcodec_send_frame(codec_ctx_.get(), frame);
    if (ret < 0) {
        Logger::error("Failed to send frame to encoder");
        return false;
    }

    ret = ffmpeg_->avcodec_receive_packet(codec_ctx_.get(), packet);
    if (ret == AVERROR(EAGAIN)) {
        // Encoder needs more frames before producing a packet; caller can retry next iteration
        return true;
    } else if (ret < 0) {
        Logger::error("Failed to receive packet from encoder");
        return false;
    }

    packet->stream_index = video_stream_index_;
    packet->duration = 1;

    return true;
}

void BrowserInput::onFrameReceived(const uint8_t* bgra_data, int width, int height) {
    if (!bgra_data || width <= 0 || height <= 0) {
        return;
    }

    int adjusted_width = width & ~1;
    int adjusted_height = height & ~1;

    // Acquire lock BEFORE modifying sws_ctx_ to prevent race conditions
    std::lock_guard<std::mutex> lock(frame_mutex_);

    if (adjusted_width != snapshot_width_ || adjusted_height != snapshot_height_) {
        if (adjusted_width != width || adjusted_height != height) {
            Logger::info("Snapshot dimensions: " + std::to_string(width) + "x" + std::to_string(height) +
                         " adjusted to " + std::to_string(adjusted_width) + "x" + std::to_string(adjusted_height));
        }

        bool needs_scaling = (adjusted_width != (int)config_.video.width || adjusted_height != (int)config_.video.height);
        if (needs_scaling || snapshot_width_ == 0) {
            const char* action = sws_ctx_ ? "Recreating" : "Creating";
            if (needs_scaling) {
                Logger::info(std::string(action) + " scaler: " + std::to_string(adjusted_width) + "x" + std::to_string(adjusted_height) +
                             " -> " + std::to_string(config_.video.width) + "x" + std::to_string(config_.video.height));
            }
        }

        sws_ctx_ = std::unique_ptr<SwsContext, SwsContextDeleter>(ffmpeg_->sws_getContext(
            adjusted_width, adjusted_height, AV_PIX_FMT_BGRA,
            config_.video.width, config_.video.height, AV_PIX_FMT_YUV420P,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
        ), SwsContextDeleter(ffmpeg_));

        if (!sws_ctx_) {
            Logger::error("Failed to recreate scaler context");
            return;
        }

        snapshot_width_ = adjusted_width;
        snapshot_height_ = adjusted_height;
    }

    size_t frame_size = width * height * 4;
    current_frame_.resize(frame_size);
    std::memcpy(current_frame_.data(), bgra_data, frame_size);

    frame_ready_ = true;
}

void BrowserInput::pullAudioFromBackend() {
    if (!backend_) {
        return;
    }

    CEFBackend* cef_backend = dynamic_cast<CEFBackend*>(backend_.get());
    if (!cef_backend) {
        return;
    }

    if (!audio_stream_started_ && cef_backend->isAudioStreaming()) {
        audio_stream_started_ = true;
        Logger::info("Audio stream detected from CEF");
    }

    if (cef_backend->hasAudioData()) {
        std::vector<float> new_audio = cef_backend->getAndClearAudioBuffer();
        if (new_audio.empty()) {
            return;
        }

        static int audio_packet_count = 0;
        audio_packet_count++;

        if (start_time_ms_ == 0) {
            static bool logged_discard = false;
            if (!logged_discard) {
                Logger::info("Discarding pre-page-load audio (packet #" + std::to_string(audio_packet_count) +
                           ") - waiting for page to load and first video frame");
                logged_discard = true;
            }
            return;
        }

        if (audio_start_pts_ < 0 && audio_sample_rate_ > 0) {
            audio_start_pts_ = 0;
            Logger::info("First audio after video start - audio PTS starts from 0");
        }

        audio_buffer_.insert(audio_buffer_.end(), new_audio.begin(), new_audio.end());

        if (audio_packet_count % 50 == 0) {
            Logger::info("Audio packet #" + std::to_string(audio_packet_count) +
                       " received, buffer size: " + std::to_string(audio_buffer_.size()) + " samples");
        }
    }
}

bool BrowserInput::encodeAudio(AVPacket* packet) {
    if (resetting_encoders_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(encoder_mutex_);

    if (!audio_codec_ctx_ || !audio_frame_ || audio_channels_ == 0) {
        return false;
    }

    int samples_needed = audio_codec_ctx_->frame_size * audio_channels_;
    if ((int)audio_buffer_.size() < samples_needed) {
        return false;
    }

    float** planes = (float**)audio_frame_->data;
    for (int sample = 0; sample < audio_codec_ctx_->frame_size; sample++) {
        for (int ch = 0; ch < audio_channels_; ch++) {
            planes[ch][sample] = audio_buffer_[sample * audio_channels_ + ch];
        }
    }

    audio_frame_->pts = audio_samples_written_;
    audio_samples_written_ += audio_codec_ctx_->frame_size;

    audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + samples_needed);

    int ret = ffmpeg_->avcodec_send_frame(audio_codec_ctx_.get(), audio_frame_.get());
    if (ret < 0) {
        Logger::error("Error sending audio frame to encoder");
        return false;
    }

    ret = ffmpeg_->avcodec_receive_packet(audio_codec_ctx_.get(), packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return false;
    } else if (ret < 0) {
        Logger::error("Error receiving encoded audio packet");
        return false;
    }

    packet->stream_index = audio_stream_index_;
    return true;
}

bool BrowserInput::hasAudioData() const {
    return !audio_buffer_.empty();
}

void BrowserInput::tryInitializeCEF() {
    if (cef_initialized_.load() || pending_uri_.empty()) {
        return; // Already initialized or no URI
    }

    Logger::info("Initializing CEF browser backend...");

    if (!BrowserBackendFactory::isAvailable()) {
        Logger::error("No OBS browser backend detected");
        Logger::error("Please install OBS Studio with the Browser Source (CEF) module enabled.");
        cef_initialized_ = true; // Mark as "done" to avoid retrying
        return;
    }

    backend_ = std::unique_ptr<BrowserBackend>(BrowserBackendFactory::create());
    if (!backend_) {
        Logger::error("Failed to create browser backend");
        cef_initialized_ = true;
        return;
    }

    Logger::info("Using browser backend: " + std::string(backend_->getName()));

    backend_->setViewportSize(config_.video.width, config_.video.height);
    backend_->setFrameCallback([this](const uint8_t* data, int width, int height) {
        this->onFrameReceived(data, width, height);
    });

    // Configure JavaScript injection (if CEF backend)
    CEFBackend* cef_backend = dynamic_cast<CEFBackend*>(backend_.get());
    if (cef_backend) {
        cef_backend->setJsInjectionEnabled(config_.browser.enableJsInjection);
        if (!config_.browser.enableJsInjection) {
            Logger::info("JavaScript injection disabled (--no-js flag)");
        }
    }

    Logger::info("Loading URL: " + pending_uri_);
    if (!backend_->loadURL(pending_uri_)) {
        Logger::error("Failed to load URL in browser");
        cef_initialized_ = true;
        return;
    }

    cef_initialized_ = true;
    Logger::info("CEF initialized successfully");
}

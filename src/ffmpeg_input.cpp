
#include "ffmpeg_input.h"
#include "ffmpeg_loader.h"
#include "logger.h"

extern "C" {
#include <libavformat/avformat.h>
}

FFmpegInput::FFmpegInput(const std::string& protocol) : protocol_(protocol) {}

FFmpegInput::~FFmpegInput() {
    close();
}

bool FFmpegInput::open(const std::string& uri) {
    if (FFmpegLib::avformat_open_input(&formatContext_, uri.c_str(), nullptr, nullptr) != 0) {
        Logger::error("Failed to open input: " + uri);
        return false;
    }

    if (FFmpegLib::avformat_find_stream_info(formatContext_, nullptr) < 0) {
        Logger::error("Failed to find stream info");
        return false;
    }

    for (unsigned int i = 0; i < formatContext_->nb_streams; i++) {
        if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
        } else if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex_ = i;
        }
    }

    if (videoStreamIndex_ == -1) {
        Logger::error("No video stream found in input");
        return false;
    }

    return true;
}

bool FFmpegInput::readPacket(AVPacket* packet) {
    return FFmpegLib::av_read_frame(formatContext_, packet) >= 0;
}

void FFmpegInput::close() {
    if (formatContext_) {
        FFmpegLib::avformat_close_input(&formatContext_);
        formatContext_ = nullptr;
    }
}

AVFormatContext* FFmpegInput::getFormatContext() {
    return formatContext_;
}

int FFmpegInput::getVideoStreamIndex() const {
    return videoStreamIndex_;
}

int FFmpegInput::getAudioStreamIndex() const {
    return audioStreamIndex_;
}

bool FFmpegInput::isLiveStream() const {
    return protocol_ != "file";
}

std::string FFmpegInput::getTypeName() const {
    return protocol_;
}

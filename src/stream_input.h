#ifndef STREAM_INPUT_H
#define STREAM_INPUT_H

#include <string>
#include <memory>
#include "config.h"

// Forward declarations for FFmpeg types
extern "C" {
struct AVFormatContext;
struct AVPacket;
struct AVCodecParameters;
struct AVRational;
}

class StreamInput {
public:
    virtual ~StreamInput() = default;

    virtual bool open(const std::string& uri) = 0;
    virtual bool readPacket(AVPacket* packet) = 0;
    virtual void close() = 0;
    virtual AVFormatContext* getFormatContext() = 0;
    virtual int getVideoStreamIndex() const = 0;
    virtual int getAudioStreamIndex() const = 0;
    virtual bool isLiveStream() const = 0;
    virtual std::string getTypeName() const = 0;
    virtual bool isProgrammatic() const { return false; }
};

class StreamInputFactory {
public:
    static std::unique_ptr<StreamInput> create(const std::string& uri, const AppConfig& config);
    static std::string detectInputType(const std::string& uri);
};

#endif // STREAM_INPUT_H

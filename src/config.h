
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct HLSConfig {
    std::string inputFile;
    std::string outputDir;
    int segmentDuration = 2;  // 2s segments - good balance of latency and efficiency
    int playlistSize = 3;     // Small live window (3 segments = ~6s buffer)
};

struct VideoConfig {
    int width = 1280;
    int height = 720;
    int fps = 30;
    int bitrate = 2500000;
    int gop_size = 15;  // 15 frames = 0.5s keyframe interval for FAST segment generation
                        // This ensures the first HLS segment appears within 0.5 seconds
};

struct AudioConfig {
    int sample_rate = 44100;
    int channels = 2;
    int bitrate = 128000;
};

struct BrowserConfig {
    bool enableJsInjection = true;  // Enable JavaScript injection by default
};

struct AppConfig {
    HLSConfig hls;
    VideoConfig video;
    AudioConfig audio;
    BrowserConfig browser;
};

#endif // CONFIG_H

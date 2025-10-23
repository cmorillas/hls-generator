
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct HLSConfig {
    std::string inputFile;
    std::string outputDir;
    int segmentDuration = 2;  // Reduced from 3s to 2s for lower latency
    int playlistSize = 3;     // Reduced from 5 to 3 for faster startup
};

struct VideoConfig {
    int width = 1280;
    int height = 720;
    int fps = 30;
    int bitrate = 2500000;
    int gop_size = 60;  // 30 fps × 2s = 60 frames (one IDR per segment)
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

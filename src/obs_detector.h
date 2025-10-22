#ifndef OBS_DETECTOR_H
#define OBS_DETECTOR_H

#include <string>
#include <vector>

struct OBSPaths {
    std::string obsExecutable;
    std::string obsLibDir;
    std::string ffmpegLibDir;
    std::string cef_path;         // Directory containing libcef.dll/so
    std::string subprocess_path;   // Path to obs-browser-page helper
    bool found = false;
    std::string source = "";  // "OBS" or "FFmpeg" or "Unknown"
};

class OBSDetector {
public:
    static OBSPaths detect();

private:
    static OBSPaths detectLinux();
    static OBSPaths detectWindows();
    static OBSPaths detectSystemFFmpeg();
    static bool fileExists(const std::string& path);
    static bool directoryExists(const std::string& path);
};

#endif // OBS_DETECTOR_H

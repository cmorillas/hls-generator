#include "obs_detector.h"
#include "logger.h"
#include <sys/stat.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

bool OBSDetector::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

bool OBSDetector::directoryExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

OBSPaths OBSDetector::detect() {
    // Try OBS first (preferred)
#ifdef PLATFORM_WINDOWS
    OBSPaths paths = detectWindows();
#else
    OBSPaths paths = detectLinux();
#endif

    if (paths.found) {
        paths.source = "OBS Studio";
        return paths;
    }

    // Fallback to system FFmpeg if OBS not found
    Logger::info("OBS Studio not found. Searching for system FFmpeg...");
    paths = detectSystemFFmpeg();

    if (paths.found) {
        paths.source = "System FFmpeg";
        Logger::info("Using FFmpeg from system installation");
    }

    return paths;
}

OBSPaths OBSDetector::detectLinux() {
    Logger::info("Detecting OBS Studio on Linux...");

    OBSPaths paths;

    // 1. Search for OBS executable
    std::vector<std::string> executablePaths = {
        "/usr/bin/obs",
        "/usr/local/bin/obs",
        "/opt/obs-studio/bin/obs"
    };

    for (const auto& path : executablePaths) {
        if (fileExists(path)) {
            paths.obsExecutable = path;
            Logger::info("OBS executable found: " + path);
            break;
        }
    }

    if (paths.obsExecutable.empty()) {
        Logger::error("OBS Studio not found in the system");
        return paths;
    }

    // 2. Search for OBS libraries directory
    std::vector<std::string> libPaths = {
        "/usr/lib/obs-plugins",
        "/usr/lib/x86_64-linux-gnu/obs-plugins",
        "/usr/lib64/obs-plugins",
        "/opt/obs-studio/lib/obs-plugins"
    };

    for (const auto& path : libPaths) {
        if (directoryExists(path)) {
            paths.obsLibDir = path;
            Logger::info("OBS plugins directory: " + path);
            break;
        }
    }

    // 3. Search for FFmpeg libraries (may be in system paths)
    std::vector<std::string> ffmpegPaths = {
        "/usr/lib/x86_64-linux-gnu",
        "/usr/lib64",
        "/usr/lib",
        paths.obsLibDir  // Sometimes OBS includes its own libs
    };

    for (const auto& path : ffmpegPaths) {
        std::string avcodecPath = path + "/libavcodec.so";
        std::string avformatPath = path + "/libavformat.so";
        std::string avutilPath = path + "/libavutil.so";

        if (fileExists(avcodecPath) || fileExists(avcodecPath + ".61")) {
            paths.ffmpegLibDir = path;
            Logger::info("FFmpeg libraries found in: " + path);

            // 4. Set CEF paths if OBS plugin directory was found
            if (!paths.obsLibDir.empty()) {
                // CEF library is in the OBS plugins directory
                paths.cef_path = paths.obsLibDir;
                // obs-browser-page helper is also in plugins directory
                paths.subprocess_path = paths.obsLibDir + "/obs-browser-page";
                Logger::info("CEF path: " + paths.cef_path);
                Logger::info("CEF subprocess: " + paths.subprocess_path);
            }

            paths.found = true;
            return paths;
        }
    }

    Logger::error("FFmpeg libraries not found");
    return paths;
}

OBSPaths OBSDetector::detectWindows() {
    Logger::info("Detecting OBS Studio on Windows...");

    OBSPaths paths;

#ifdef PLATFORM_WINDOWS
    // 1. Common Windows installation paths
    std::vector<std::string> obsDirs = {
        "C:\\Program Files\\obs-studio",
        "C:\\Program Files (x86)\\obs-studio"
    };

    // 2. Search in Program Files using environment variable
    const char* programFiles = std::getenv("ProgramFiles");
    if (programFiles) {
        obsDirs.push_back(std::string(programFiles) + "\\obs-studio");
    }

    for (const auto& obsDir : obsDirs) {
        std::string exePath = obsDir + "\\bin\\64bit\\obs64.exe";

        if (fileExists(exePath)) {
            paths.obsExecutable = exePath;
            paths.obsLibDir = obsDir + "\\obs-plugins\\64bit";
            paths.ffmpegLibDir = obsDir + "\\bin\\64bit";

            Logger::info("OBS Studio found: " + obsDir);

            // Verify FFmpeg DLLs exist (try different versions)
            std::vector<std::string> avcodecVersions = {
                paths.ffmpegLibDir + "\\avcodec-61.dll",  // FFmpeg 6.1/7.0
                paths.ffmpegLibDir + "\\avcodec-62.dll",  // FFmpeg 7.1 (future)
                paths.ffmpegLibDir + "\\avcodec-60.dll",  // FFmpeg 6.0
                paths.ffmpegLibDir + "\\avcodec.dll"      // Generic
            };

            for (const auto& avcodecDll : avcodecVersions) {
                if (fileExists(avcodecDll)) {
                    Logger::info("FFmpeg libraries found in: " + paths.ffmpegLibDir);

                    // Set CEF paths for Windows
                    // In Windows, CEF DLL is in obs-plugins\64bit directory (NOT bin\64bit)
                    paths.cef_path = paths.obsLibDir;
                    // obs-browser-page.exe is also in the obs-plugins directory
                    paths.subprocess_path = paths.obsLibDir + "\\obs-browser-page.exe";
                    Logger::info("CEF path: " + paths.cef_path);
                    Logger::info("CEF subprocess: " + paths.subprocess_path);

                    paths.found = true;
                    return paths;
                }
            }
        }
    }

    Logger::warn("OBS Studio not found in the system");
#endif

    return paths;
}

OBSPaths OBSDetector::detectSystemFFmpeg() {
    Logger::info("Searching for system FFmpeg installation...");

    OBSPaths paths;

#ifdef PLATFORM_WINDOWS
    // Windows: Try common FFmpeg installation paths
    std::vector<std::string> ffmpegDirs = {
        "C:\\ffmpeg\\bin",
        "C:\\Program Files\\ffmpeg\\bin",
        std::string(std::getenv("ProgramFiles") ? std::getenv("ProgramFiles") : "") + "\\ffmpeg\\bin"
    };

    for (const auto& dir : ffmpegDirs) {
        if (directoryExists(dir)) {
            // Check if avformat DLL exists
            if (fileExists(dir + "\\avformat-61.dll") ||
                fileExists(dir + "\\avformat-60.dll") ||
                fileExists(dir + "\\avformat.dll")) {
                paths.ffmpegLibDir = dir;
                paths.found = true;
                Logger::info("FFmpeg found: " + dir);
                break;
            }
        }
    }
#else
    // Linux: Standard library paths
    std::vector<std::string> ffmpegDirs = {
        "/usr/lib/x86_64-linux-gnu",
        "/usr/lib64",
        "/usr/lib",
        "/usr/local/lib"
    };

    for (const auto& dir : ffmpegDirs) {
        if (directoryExists(dir)) {
            // Check if libavformat exists (any version)
            if (fileExists(dir + "/libavformat.so.61") ||
                fileExists(dir + "/libavformat.so.60") ||
                fileExists(dir + "/libavformat.so")) {
                paths.ffmpegLibDir = dir;
                paths.found = true;
                Logger::info("FFmpeg found: " + dir);
                break;
            }
        }
    }
#endif

    if (!paths.found) {
        Logger::info("System FFmpeg not found either");
    }

    return paths;
}

#include "browser_backend.h"
#include "logger.h"
#include "obs_detector.h"

#include <string>

#ifndef _WIN32  // Linux implementation
#include <dlfcn.h>

// Forward declaration of CEF backend factory
extern "C" BrowserBackend* createCEFBackend();

// Factory implementation for Linux - CEF only
BrowserBackend* BrowserBackendFactory::create() {
    Logger::info("Using CEF backend (OBS Chromium)...");
    BrowserBackend* cef_backend = createCEFBackend();
    if (!cef_backend) {
        Logger::error("CEF backend failed to initialize");
        Logger::error("Make sure OBS Studio is installed with CEF support");
        return nullptr;
    }
    return cef_backend;
}

bool BrowserBackendFactory::isAvailable() {
    // Check if CEF library exists in OBS installation
    OBSPaths obsPaths = OBSDetector::detect();
    if (!obsPaths.found) {
        return false;
    }

    std::string cef_path = obsPaths.obsLibDir.empty() ?
        obsPaths.ffmpegLibDir + "/obs-plugins/libcef.so" :
        obsPaths.obsLibDir + "/libcef.so";

    void* handle = dlopen(cef_path.c_str(), RTLD_LAZY);
    if (handle) {
        dlclose(handle);
        return true;
    }
    return false;
}

const char* BrowserBackendFactory::getAvailableBackendName() {
    return isAvailable() ? "CEF (OBS)" : nullptr;
}

#else  // Windows implementation

#include <windows.h>

// Forward declaration of CEF backend factory
extern "C" BrowserBackend* createCEFBackend();

// Factory implementation for Windows - CEF only
BrowserBackend* BrowserBackendFactory::create() {
    Logger::info("Using CEF backend (OBS Chromium)...");
    BrowserBackend* cef_backend = createCEFBackend();
    if (!cef_backend) {
        Logger::error("CEF backend failed to initialize");
        Logger::error("Make sure OBS Studio is installed with CEF support");
        return nullptr;
    }
    return cef_backend;
}

bool BrowserBackendFactory::isAvailable() {
    // Check if CEF library exists in OBS installation
    OBSPaths obsPaths = OBSDetector::detect();
    if (!obsPaths.found) {
        Logger::error("OBS not found, CEF backend unavailable");
        return false;
    }

    std::string cef_path = obsPaths.cef_path.empty() ?
        obsPaths.ffmpegLibDir + "\\libcef.dll" :
        obsPaths.cef_path + "\\libcef.dll";

    Logger::info("Attempting to load CEF from: " + cef_path);

    // Add CEF directory to DLL search path so Windows can find dependencies
    // (chrome_elf.dll, libEGL.dll, libGLESv2.dll, etc.)
    std::string cef_dir = obsPaths.cef_path.empty() ? obsPaths.ffmpegLibDir : obsPaths.cef_path;
    SetDllDirectoryA(cef_dir.c_str());
    Logger::info("Added DLL search directory: " + cef_dir);

    HMODULE handle = LoadLibraryA(cef_path.c_str());

    // Restore default DLL search path
    SetDllDirectoryA(nullptr);

    if (handle) {
        FreeLibrary(handle);
        Logger::info("CEF library loaded successfully");
        return true;
    }

    DWORD error = GetLastError();
    Logger::error("Failed to load CEF library from: " + cef_path);
    Logger::error("Windows error code: " + std::to_string(error));
    return false;
}

const char* BrowserBackendFactory::getAvailableBackendName() {
    return isAvailable() ? "CEF (OBS)" : nullptr;
}

#endif // _WIN32

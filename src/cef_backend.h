#ifndef CEF_BACKEND_H
#define CEF_BACKEND_H

#include "browser_backend.h"
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>

// Forward declarations for CEF C++ types
// CefRefPtr is defined in CEF headers, so we don't forward declare it
// We use void* for CEF objects to avoid including CEF headers here

/**
 * CEF (Chromium Embedded Framework) backend
 * Uses libcef.so from OBS Studio installation for offscreen rendering
 */
class CEFBackend : public BrowserBackend {
public:
    CEFBackend();
    ~CEFBackend() override;

    // BrowserBackend interface
    bool initialize() override;
    bool loadURL(const std::string& url) override;
    void setViewportSize(int width, int height) override;
    void setFrameCallback(std::function<void(const uint8_t*, int, int)> callback) override;
    void processEvents() override;
    bool isPageLoaded() const override;
    void shutdown() override;
    const char* getName() const override { return "CEF (OBS)"; }

    // Force browser repaint (for continuous frame generation)
    void invalidate();

    // Signal browser to generate next frame (OBS-style external frame control)
    void signalBeginFrame();

    // CEF callbacks
    void onPaint(const void* buffer, int width, int height);
    void onLoadEnd();
    void onLoadError(const std::string& url, const std::string& error);

    // Audio callbacks
    void onAudioStreamStarted(int channels, int sample_rate, int frames_per_buffer);
    void onAudioStreamPacket(const float** data, int frames, int channels, int64_t pts);
    void onAudioStreamStopped();
    void onAudioStreamError(const std::string& message);
    int onGetAudioChannels() const { return audio_channels_; }

    // Audio buffer access (for BrowserInput to encode)
    std::vector<float> getAndClearAudioBuffer();
    int getAudioChannels() const { return audio_channels_; }
    int getAudioSampleRate() const { return audio_sample_rate_; }
    bool hasAudioData() const;
    bool isAudioStreaming() const { return audio_streaming_; }

    // Check if page load failed
    bool hasLoadError() const;

    // Check if page was reloaded and reset the flag
    bool checkAndClearPageReload() {
        return page_reloaded_.exchange(false);
    }

    // Store browser from callback (called by LifeSpanHandler)
    void setBrowser(void* browser);

    // Control JavaScript injection
    void setJsInjectionEnabled(bool enabled) { enable_js_injection_ = enabled; }
    bool isJsInjectionEnabled() const { return enable_js_injection_; }

    // Public access to dimensions
    int width_;
    int height_;

private:
    // CEF handles using opaque pointers (actual CefRefPtr<T> in implementation)
    // We use void* here to avoid including CEF headers in this public header
    void* browser_;  // Actually CefRefPtr<CefBrowser>*
    void* client_;   // Actually CefRefPtr<CefClient>*

    std::string pending_url_;

    // Browser state
    std::atomic<bool> page_loaded_;
    std::atomic<bool> initialized_;
    std::atomic<bool> page_reloaded_;  // Set to true when page reloads (e.g., after cookie acceptance)
    std::atomic<bool> browser_created_;
    std::atomic<bool> load_error_;

    // CEF-specific state
    bool cef_initialized_;
    bool enable_js_injection_;  // Control JavaScript injection (default: true)
    std::string load_error_message_;
    std::string cache_path_;  // Temporary cache directory path (for cleanup)

    // Frame data
    std::mutex frame_mutex_;
    std::vector<uint8_t> frame_buffer_;
    std::function<void(const uint8_t*, int, int)> frame_callback_;

    // Audio data
    mutable std::mutex audio_mutex_;
    std::vector<float> audio_buffer_;  // Accumulated PCM float samples
    int audio_channels_;
    int audio_sample_rate_;
    std::atomic<bool> audio_streaming_;

    // Helper methods
    bool loadCEFLibrary();
    bool checkCEFVersion();
    bool initializeCEF();
    void createBrowser(const std::string& url);
    void cleanupCEF();
    void cleanupCacheDirectory();
};

#endif // CEF_BACKEND_H

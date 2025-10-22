// CEF C++ API headers - MUST come first before cef_loader.h
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_browser.h>
#include <include/cef_render_handler.h>
#include <include/cef_life_span_handler.h>
#include <include/cef_load_handler.h>
#include <include/cef_audio_handler.h>
#include <include/cef_command_line.h>
#include <include/cef_version.h>
#include <include/cef_api_hash.h>

#include "cef_backend.h"
#include "cef_loader.h"
#include "logger.h"
#include "obs_detector.h"
#include "all_cef_scripts.h"  // Auto-generated CEF injection scripts

#include <cstring>
#include <cstdlib>  // for std::getenv
#include <thread>
#include <chrono>
#include <filesystem>  // for std::filesystem

#ifdef _WIN32
#include <windows.h>
#include <process.h>  // for _getpid()
#define getpid _getpid
#else
#include <unistd.h>   // for getpid()
#include <fcntl.h>    // for open()
#include <sys/file.h> // for flock()
#endif

// Constants for audio packet logging
namespace {
    constexpr int AUDIO_PACKET_INITIAL_LOG_COUNT = 10;  // Log first N audio packets
    constexpr int AUDIO_PACKET_LOG_INTERVAL = 100;      // Then log every N packets
}

// Simple app to configure command-line switches (OBS-style, multi-process with CEF standalone)
class SimpleApp : public CefApp {
public:
    void OnBeforeCommandLineProcessing(
        const CefString& process_type,
        CefRefPtr<CefCommandLine> command_line) override {
        // REMOVED: single-process mode (now using multi-process like OBS)
        // command_line->AppendSwitch("single-process");

        // OBS command-line switches (exact match)
        command_line->AppendSwitch("disable-gpu-compositing");
        command_line->AppendSwitchWithValue("disable-features", "HardwareMediaKeyHandling,WebBluetooth");
        command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");

        #ifndef _WIN32
        // Linux-specific: Critical for subprocess stability
        command_line->AppendSwitchWithValue("ozone-platform", "x11");
        #endif

        Logger::info("CEF command line configured (OBS-style, multi-process)");
    }

private:
    IMPLEMENT_REFCOUNTING(SimpleApp);
};

// Simple render handler to capture frames
class SimpleRenderHandler : public CefRenderHandler {
public:
    explicit SimpleRenderHandler(CEFBackend* backend) : backend_(backend) {}

    // CefRenderHandler methods
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override {
        rect = CefRect(0, 0, backend_->width_, backend_->height_);
    }

    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType type,
                 const RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height) override {
        if (type == PET_VIEW) {
            backend_->onPaint(buffer, width, height);
        }
    }

private:
    CEFBackend* backend_;
    IMPLEMENT_REFCOUNTING(SimpleRenderHandler);
};

// Simple life span handler
class SimpleLifeSpanHandler : public CefLifeSpanHandler {
public:
    explicit SimpleLifeSpanHandler(CEFBackend* backend) : backend_(backend) {}

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        Logger::info("CEF browser created via callback");

        // Store browser in backend using public method
        backend_->setBrowser(new CefRefPtr<CefBrowser>(browser));
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        Logger::info("CEF browser closing");
    }

private:
    CEFBackend* backend_;
    IMPLEMENT_REFCOUNTING(SimpleLifeSpanHandler);
};

// Simple load handler
class SimpleLoadHandler : public CefLoadHandler {
public:
    explicit SimpleLoadHandler(CEFBackend* backend) : backend_(backend) {}

    void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   int httpStatusCode) override {
        if (frame->IsMain()) {
            backend_->onLoadEnd();
        }
    }

    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) override {
        if (frame->IsMain()) {
            backend_->onLoadError(failedUrl.ToString(), errorText.ToString());
        }
    }

private:
    CEFBackend* backend_;
    IMPLEMENT_REFCOUNTING(SimpleLoadHandler);
};

// Simple audio handler
class SimpleAudioHandler : public CefAudioHandler {
public:
    explicit SimpleAudioHandler(CEFBackend* backend) : backend_(backend) {}

    // Called when audio stream starts
    void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser,
                              const CefAudioParameters& params,
                              int channels) override {
        Logger::info("Audio stream started: " + std::to_string(channels) + " channels, " +
                     std::to_string(params.sample_rate) + " Hz, " +
                     std::to_string(params.frames_per_buffer) + " frames/buffer");

        backend_->onAudioStreamStarted(channels, params.sample_rate, params.frames_per_buffer);
    }

    // Called when PCM audio packet arrives
    void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser,
                             const float** data,
                             int frames,
                             int64_t pts) override {
        // Get channels from stored state (we need to know how many channel arrays to read)
        // data is an array of channel pointers: data[0] = left channel, data[1] = right channel, etc.
        backend_->onAudioStreamPacket(data, frames, backend_->onGetAudioChannels(), pts);
    }

    // Called when audio stream stops
    void OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override {
        Logger::info("Audio stream stopped");
        backend_->onAudioStreamStopped();
    }

    // Called on audio error
    void OnAudioStreamError(CefRefPtr<CefBrowser> browser,
                            const CefString& message) override {
        Logger::error("Audio stream error: " + message.ToString());
        backend_->onAudioStreamError(message.ToString());
    }

private:
    CEFBackend* backend_;
    IMPLEMENT_REFCOUNTING(SimpleAudioHandler);
};

// Simple client handler
class SimpleClient : public CefClient {
public:
    explicit SimpleClient(CEFBackend* backend)
        : render_handler_(new SimpleRenderHandler(backend))
        , life_span_handler_(new SimpleLifeSpanHandler(backend))
        , load_handler_(new SimpleLoadHandler(backend))
        , audio_handler_(new SimpleAudioHandler(backend)) {
    }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override {
        return render_handler_;
    }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return life_span_handler_;
    }

    CefRefPtr<CefLoadHandler> GetLoadHandler() override {
        return load_handler_;
    }

    CefRefPtr<CefAudioHandler> GetAudioHandler() override {
        return audio_handler_;
    }

private:
    CefRefPtr<CefRenderHandler> render_handler_;
    CefRefPtr<CefLifeSpanHandler> life_span_handler_;
    CefRefPtr<CefLoadHandler> load_handler_;
    CefRefPtr<CefAudioHandler> audio_handler_;
    IMPLEMENT_REFCOUNTING(SimpleClient);
};

CEFBackend::CEFBackend()
    : width_(1280)
    , height_(720)
    , browser_(nullptr)
    , client_(nullptr)
    , page_loaded_(false)
    , initialized_(false)
    , page_reloaded_(false)
    , browser_created_(false)
    , load_error_(false)
    , cef_initialized_(false)
    , enable_js_injection_(true)
    , audio_channels_(0)
    , audio_sample_rate_(0)
    , audio_streaming_(false) {
}

CEFBackend::~CEFBackend() {
    shutdown();
}

bool CEFBackend::loadCEFLibrary() {
    // Load CEF dynamically from OBS installation (like ffmpeg_loader)
    OBSPaths obsPaths = OBSDetector::detect();
    if (!obsPaths.found) {
        Logger::error("OBS not detected - cannot load CEF");
        Logger::error("Install OBS Studio to use browser source:");
        #ifdef _WIN32
        Logger::error("  Download from: https://obsproject.com/download");
        #else
        Logger::error("  sudo apt install obs-studio");
        #endif
        return false;
    }

    Logger::info("OBS detected - loading CEF from OBS installation");
    Logger::info("  CEF path: " + obsPaths.cef_path);

    // Load CEF dynamically (LoadLibrary/dlopen)
    if (!::loadCEFLibrary(obsPaths.cef_path)) {
        Logger::error("Failed to load CEF from OBS");
        Logger::error("  Path tried: " + obsPaths.cef_path);
        return false;
    }

    Logger::info("CEF loaded successfully from OBS");
    return true;
}

bool CEFBackend::initialize() {
    if (initialized_) {
        return true;
    }

    Logger::info("Initializing CEF backend...");

    // Load CEF library
    if (!loadCEFLibrary()) {
        return false;
    }

    // Initialize CEF
    if (!initializeCEF()) {
        return false;
    }

    initialized_ = true;
    Logger::info("CEF backend initialized");
    return true;
}

bool CEFBackend::checkCEFVersion() {
    // Get runtime CEF version from loaded libcef.so
    int runtime_cef_major = cef_version_info(0);
    int runtime_cef_minor = cef_version_info(1);
    int runtime_cef_patch = cef_version_info(2);
    int runtime_chrome_major = cef_version_info(4);
    int runtime_chrome_minor = cef_version_info(5);
    int runtime_chrome_build = cef_version_info(6);
    int runtime_chrome_patch = cef_version_info(7);

    // Get compile-time CEF version from headers
    int compiled_cef_major = CEF_VERSION_MAJOR;
    int compiled_chrome_build = CHROME_VERSION_BUILD;

    // Log detected versions
    Logger::info("CEF version detected:");
    Logger::info("  Runtime:  CEF " + std::to_string(runtime_cef_major) + "." +
                std::to_string(runtime_cef_minor) + "." + std::to_string(runtime_cef_patch) +
                " (Chromium " + std::to_string(runtime_chrome_major) + ".0." +
                std::to_string(runtime_chrome_build) + "." + std::to_string(runtime_chrome_patch) + ")");
    Logger::info("  Compiled: CEF " + std::to_string(compiled_cef_major) + ".x.x (Chromium " +
                std::to_string(CHROME_VERSION_MAJOR) + ".0." + std::to_string(compiled_chrome_build) + ".x)");

    // Check API hash for binary compatibility (this is the REAL compatibility test)
    const char* runtime_hash = cef_api_hash(0);  // Platform hash
    const char* compiled_hash = CEF_API_HASH_PLATFORM;

    if (strcmp(runtime_hash, compiled_hash) != 0) {
        Logger::error("╔════════════════════════════════════════════════════════════╗");
        Logger::error("║       CEF API INCOMPATIBILITY DETECTED!                    ║");
        Logger::error("╚════════════════════════════════════════════════════════════╝");
        Logger::error("");
        Logger::error("API hash mismatch - binary compatibility broken:");
        Logger::error("  Runtime:  " + std::string(runtime_hash));
        Logger::error("  Compiled: " + std::string(compiled_hash));
        Logger::error("");
        Logger::error("OBS has updated to an incompatible CEF version.");
        Logger::error("You MUST recompile hls-generator:");
        Logger::error("  cd build && cmake .. && make");
        Logger::error("");
        return false;
    }

    // Informational warnings for version differences (but API is compatible)
    if (runtime_chrome_build != compiled_chrome_build) {
        Logger::warn("CEF Chromium build differs (compiled=" + std::to_string(compiled_chrome_build) +
                    ", runtime=" + std::to_string(runtime_chrome_build) + ")");
        Logger::warn("API is compatible, but recompilation recommended for optimal performance.");
    }

    if (runtime_cef_major != compiled_cef_major) {
        Logger::warn("CEF major version differs (compiled=" + std::to_string(compiled_cef_major) +
                    ", runtime=" + std::to_string(runtime_cef_major) + ")");
        Logger::warn("API is compatible, but recompilation recommended.");
    }

    Logger::info("CEF version check: OK (API compatible)");
    return true;
}

bool CEFBackend::initializeCEF() {
    if (cef_initialized_) {
        return true;
    }

    Logger::info("Initializing CEF...");

    // Create app handler to configure command line switches
    CefRefPtr<CefApp> app = new SimpleApp();

    // Set up CEF main args
#ifdef PLATFORM_WINDOWS
    CefMainArgs main_args(GetModuleHandle(nullptr));
#else
    CefMainArgs main_args(0, nullptr);
#endif

    // Set up CEF settings (multi-process with CEF standalone)
    CefSettings settings;
    settings.no_sandbox = true;
    settings.windowless_rendering_enabled = true;
    settings.multi_threaded_message_loop = false;
    settings.log_severity = LOGSEVERITY_DISABLE;  // Disable debug.log generation

    // No cache needed - cookies are auto-accepted via JavaScript every time
    // CEF will use in-memory cache only
    cache_path_ = "";  // Empty = no cache to cleanup
    Logger::info("Running without persistent cache (in-memory only)");
    Logger::info("Cookies will be auto-accepted via JavaScript");

    // Set browser subprocess path to OBS's obs-browser-page helper
    OBSPaths obsPaths = OBSDetector::detect();
    std::string subprocess_path = obsPaths.subprocess_path;

    if (!subprocess_path.empty()) {
        CefString(&settings.browser_subprocess_path).FromASCII(subprocess_path.c_str());
        Logger::info("CEF subprocess path: " + subprocess_path);
    } else {
        Logger::warn("No subprocess path found - CEF may not work correctly");
    }

    // Resources are provided by OBS's libcef.dll/so (embedded)
    // We don't need to set resources_dir_path, locales_dir_path, or framework_dir_path
    // because OBS's CEF has these paths compiled in

    Logger::info("CEF paths configured:");
    Logger::info("  Using OBS's embedded CEF resources");

    // Initialize CEF using the C++ API with app handler
    bool result = CefInitialize(main_args, settings, app.get(), nullptr);

    if (!result) {
        Logger::error("CefInitialize failed");
        return false;
    }

    cef_initialized_ = true;
    Logger::info("CEF initialized successfully");

    // Verify CEF version compatibility
    if (!checkCEFVersion()) {
        Logger::error("CEF version check failed - binary incompatibility detected");
        cleanupCEF();
        return false;
    }

    return true;
}

bool CEFBackend::loadURL(const std::string& url) {
    if (!initialized_) {
        Logger::error("CEF backend not initialized");
        return false;
    }

    Logger::info("Loading URL: " + url);

    // Create browser with URL
    createBrowser(url);

    return browser_created_;
}

void CEFBackend::createBrowser(const std::string& url) {
    Logger::info("Creating CEF browser for URL: " + url);

    // Create client handler
    CefRefPtr<CefClient> client = new SimpleClient(this);

    // Store client in opaque pointer
    if (client_) {
        delete static_cast<CefRefPtr<CefClient>*>(client_);
    }
    client_ = new CefRefPtr<CefClient>(client);

    // Set up window info for off-screen rendering (OBS-style multi-process)
    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);  // 0 = no parent window
    // NOTE: external_begin_frame_enabled causes deadlock in our single-threaded model
    // OBS uses it with complex multi-threaded architecture we don't have
    // window_info.external_begin_frame_enabled = true;

    // Set up browser settings (auto frame generation at 30fps, audio muted)
    CefBrowserSettings browser_settings;
    browser_settings.windowless_frame_rate = 30;  // CEF auto-generates frames at 30fps

    // Mute audio - we only capture it, don't play it through speakers
    CefString(&browser_settings.default_encoding).FromASCII("utf-8");

    // Create browser with initial mute state
    // Note: We'll also use CefBrowserHost::SetAudioMuted after creation

    // Use CreateBrowserSync - requires pumping message loop
    // Note: We need to pump the message loop before calling CreateBrowserSync
    // to allow CEF to process initialization messages
    for (int i = 0; i < 10; i++) {
        CefDoMessageLoopWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CefRefPtr<CefBrowser> browser_ref = CefBrowserHost::CreateBrowserSync(
        window_info,
        client,
        url,
        browser_settings,
        nullptr,
        nullptr
    );

    if (browser_ref) {
        Logger::info("CEF browser created successfully");

        // Mute browser audio output while keeping audio capture working
        // SetAudioMuted(true) prevents audio from playing on speakers but does NOT block
        // our OnAudioStreamPacket callback - we can still capture audio for HLS segments
        browser_ref->GetHost()->SetAudioMuted(true);
        Logger::info("Browser audio muted (capture still active)");

        // Store browser
        if (browser_) {
            delete static_cast<CefRefPtr<CefBrowser>*>(browser_);
        }
        browser_ = new CefRefPtr<CefBrowser>(browser_ref);
        browser_created_ = true;
    } else {
        Logger::error("CreateBrowserSync returned nullptr");
    }
}

void CEFBackend::setBrowser(void* browser) {
    // Delete old browser if it exists
    if (browser_) {
        delete static_cast<CefRefPtr<CefBrowser>*>(browser_);
    }

    // Store new browser
    browser_ = browser;
    browser_created_ = true;

    Logger::info("Browser stored successfully via callback");
}

void CEFBackend::setViewportSize(int width, int height) {
    width_ = width;
    height_ = height;

    Logger::info("CEF viewport: " + std::to_string(width) + "x" + std::to_string(height));

    // If browser already exists, notify it of size change
    if (browser_) {
        CefRefPtr<CefBrowser>* browser_ptr = static_cast<CefRefPtr<CefBrowser>*>(browser_);
        if (browser_ptr && browser_ptr->get()) {
            (*browser_ptr)->GetHost()->WasResized();
        }
    }
}

void CEFBackend::setFrameCallback(std::function<void(const uint8_t*, int, int)> callback) {
    frame_callback_ = callback;
}

void CEFBackend::processEvents() {
    if (!initialized_ || !cef_initialized_) {
        return;
    }

    // Process CEF message loop (C++ API)
    // Note: We call SendExternalBeginFrame() from browser_input when needed,
    // not here in every processEvents call
    CefDoMessageLoopWork();
}

bool CEFBackend::isPageLoaded() const {
    return page_loaded_;
}

void CEFBackend::invalidate() {
    // Force browser to repaint (like OBS does)
    if (browser_ && page_loaded_) {
        CefRefPtr<CefBrowser>* browser_ptr = static_cast<CefRefPtr<CefBrowser>*>(browser_);
        if (browser_ptr && browser_ptr->get()) {
            (*browser_ptr)->GetHost()->Invalidate(PET_VIEW);
        }
    }
}

void CEFBackend::signalBeginFrame() {
    // Signal CEF to generate next frame (OBS-style external frame control)
    // Only call this when page is loaded and we actually need a frame
    if (browser_ && page_loaded_) {
        CefRefPtr<CefBrowser>* browser_ptr = static_cast<CefRefPtr<CefBrowser>*>(browser_);
        if (browser_ptr && browser_ptr->get()) {
            (*browser_ptr)->GetHost()->SendExternalBeginFrame();
        }
    }
}

void CEFBackend::shutdown() {
    if (!initialized_) {
        return;
    }

    Logger::info("Shutting down CEF backend...");

    // Clean up browser
    if (browser_) {
        CefRefPtr<CefBrowser>* browser_ptr = static_cast<CefRefPtr<CefBrowser>*>(browser_);
        delete browser_ptr;
        browser_ = nullptr;
    }

    // Clean up client
    if (client_) {
        CefRefPtr<CefClient>* client_ptr = static_cast<CefRefPtr<CefClient>*>(client_);
        delete client_ptr;
        client_ = nullptr;
    }

    cleanupCEF();

    initialized_ = false;
}

void CEFBackend::cleanupCacheDirectory() {
    if (!cache_path_.empty() && std::filesystem::exists(cache_path_)) {
        try {
            std::filesystem::remove_all(cache_path_);
            Logger::info("Cleaned up temporary cache: " + cache_path_);
        } catch (const std::exception& e) {
            Logger::warn("Could not cleanup cache: " + std::string(e.what()));
        }
    }
}

void CEFBackend::cleanupCEF() {
    if (cef_initialized_) {
        Logger::info("Shutting down CEF...");
        CefShutdown();
        cef_initialized_ = false;
    }

    // No cache to cleanup (using in-memory only)
}

void CEFBackend::onPaint(const void* buffer, int width, int height) {
    std::lock_guard<std::mutex> lock(frame_mutex_);

    // Copy frame data (BGRA format)
    size_t size = width * height * 4;
    frame_buffer_.resize(size);
    std::memcpy(frame_buffer_.data(), buffer, size);

    // Call frame callback
    if (frame_callback_) {
        frame_callback_(frame_buffer_.data(), width, height);
    }
}

void CEFBackend::onLoadEnd() {
    // Only mark as loaded if there was no error
    if (!load_error_) {
        // Detect page reload (happens when cookies are accepted)
        if (page_loaded_.load()) {
            page_reloaded_ = true;
            Logger::info(">>> PAGE RELOADED detected - will resync audio/video");
        }

        page_loaded_ = true;
        Logger::info("CEF page loaded");

        // Auto-accept cookies via heuristic JavaScript injection with MutationObserver
        if (browser_) {
            CefRefPtr<CefBrowser>* browser_ptr = static_cast<CefRefPtr<CefBrowser>*>(browser_);
            if (browser_ptr && browser_ptr->get()) {
                CefRefPtr<CefFrame> frame = (*browser_ptr)->GetMainFrame();
                if (frame) {
                    if (enable_js_injection_) {
                        // Inject all CEF scripts in order (from js-inject/ directory)
                        for (const char* const* script_ptr = all_cef_scripts; *script_ptr != nullptr; ++script_ptr) {
                            frame->ExecuteJavaScript(*script_ptr, frame->GetURL(), 0);
                        }
                        Logger::info(">>> JAVASCRIPT INJECTED: All CEF scripts from js-inject/ directory");
                    } else {
                        Logger::info(">>> JAVASCRIPT INJECTION DISABLED (--no-js flag)");
                    }
                }
            }
        }
    }
}

void CEFBackend::onLoadError(const std::string& url, const std::string& error) {
    load_error_ = true;
    load_error_message_ = error;
    Logger::error("Failed to load URL: " + url);
    Logger::error("Error: " + error);
}

bool CEFBackend::hasLoadError() const {
    return load_error_;
}

// Audio stream callbacks
void CEFBackend::onAudioStreamStarted(int channels, int sample_rate, int frames_per_buffer) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    audio_channels_ = channels;
    audio_sample_rate_ = sample_rate;
    audio_streaming_ = true;

    // Reserve buffer space (10 seconds worth of samples)
    size_t buffer_size = channels * sample_rate * 10;
    audio_buffer_.reserve(buffer_size);

    Logger::info("Audio capture started: " + std::to_string(channels) + " channels @ " +
                 std::to_string(sample_rate) + " Hz");
}

void CEFBackend::onAudioStreamPacket(const float** data, int frames, int channels, int64_t pts) {
    if (!audio_streaming_) {
        return;
    }

    std::lock_guard<std::mutex> lock(audio_mutex_);

    // CEF provides planar audio: data[0] = left channel, data[1] = right channel, etc.
    // We need to interleave it: [L, R, L, R, L, R, ...]

    // Calculate total samples
    size_t total_samples = frames * channels;
    size_t old_size = audio_buffer_.size();
    audio_buffer_.resize(old_size + total_samples);

    // Interleave channels
    for (int frame = 0; frame < frames; frame++) {
        for (int ch = 0; ch < channels; ch++) {
            audio_buffer_[old_size + frame * channels + ch] = data[ch][frame];
        }
    }

    // Log packet reception
    static int packet_count = 0;
    packet_count++;
    if (packet_count <= AUDIO_PACKET_INITIAL_LOG_COUNT || packet_count % AUDIO_PACKET_LOG_INTERVAL == 0) {
        Logger::info("CEF audio packet #" + std::to_string(packet_count) +
                   ": " + std::to_string(frames) + " frames (" + std::to_string(total_samples) + " samples total), " +
                   "buffer_size=" + std::to_string(audio_buffer_.size()) +
                   ", ch=" + std::to_string(channels) +
                   ", sr=" + std::to_string(audio_sample_rate_));
    }

    // Log every second worth of audio
    static int64_t last_log_pts = 0;
    if (pts - last_log_pts >= 1000) {
        Logger::info("Audio buffer: " + std::to_string(audio_buffer_.size() / channels / audio_sample_rate_) +
                     " seconds (" + std::to_string(audio_buffer_.size() * sizeof(float) / 1024 / 1024) + " MB)");
        last_log_pts = pts;
    }
}

void CEFBackend::onAudioStreamStopped() {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_streaming_ = false;
    Logger::info("Audio capture stopped. Total samples: " + std::to_string(audio_buffer_.size()));
}

void CEFBackend::onAudioStreamError(const std::string& message) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_streaming_ = false;
    Logger::error("Audio capture error: " + message);
}

std::vector<float> CEFBackend::getAndClearAudioBuffer() {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    std::vector<float> buffer = std::move(audio_buffer_);
    audio_buffer_.clear();
    return buffer;
}

bool CEFBackend::hasAudioData() const {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    return !audio_buffer_.empty() && audio_streaming_;
}

// Factory implementation
extern "C" BrowserBackend* createCEFBackend() {
    CEFBackend* backend = new CEFBackend();
    if (!backend->initialize()) {
        delete backend;
        return nullptr;
    }
    return backend;
}

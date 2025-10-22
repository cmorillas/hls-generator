#ifndef BROWSER_BACKEND_H
#define BROWSER_BACKEND_H

#include <string>
#include <functional>
#include <cstdint>

/**
 * Abstract interface for browser rendering backends
 * Implementations: OBS Chromium Embedded Framework (CEF)
 */
class BrowserBackend {
public:
    virtual ~BrowserBackend() = default;

    /**
     * Initialize the browser backend
     * @return true on success, false on failure
     */
    virtual bool initialize() = 0;

    /**
     * Load a URL in the browser
     * @param url URL to load
     * @return true on success, false on failure
     */
    virtual bool loadURL(const std::string& url) = 0;

    /**
     * Set the viewport size
     * @param width Width in pixels
     * @param height Height in pixels
     */
    virtual void setViewportSize(int width, int height) = 0;

    /**
     * Set frame callback - called when a new frame is rendered
     * @param callback Function to call with frame data (BGRA format, width, height)
     */
    virtual void setFrameCallback(std::function<void(const uint8_t*, int, int)> callback) = 0;

    /**
     * Process events (call periodically to pump message loop)
     */
    virtual void processEvents() = 0;

    /**
     * Check if page has finished loading
     * @return true if loaded, false if still loading
     */
    virtual bool isPageLoaded() const = 0;

    /**
     * Shutdown the browser backend
     */
    virtual void shutdown() = 0;

    /**
     * Get backend name
     */
    virtual const char* getName() const = 0;
};

/**
 * Factory to create the appropriate backend for the current platform
 */
class BrowserBackendFactory {
public:
    /**
     * Create a browser backend
     * @return Backend instance or nullptr if no backend available
     */
    static BrowserBackend* create();

    /**
     * Check if a browser backend is available on this system
     * @return true if available, false otherwise
     */
    static bool isAvailable();

    /**
     * Get the name of the available backend
     */
    static const char* getAvailableBackendName();
};

#endif // BROWSER_BACKEND_H

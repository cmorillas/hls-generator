
#ifndef DYNAMIC_LIBRARY_H
#define DYNAMIC_LIBRARY_H

#include <string>
#include <memory>
#include <utility>  // for std::move
#include "logger.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

class DynamicLibrary {
public:
    DynamicLibrary(const std::string& libName) : libName_(libName), handle_(nullptr) {}

    ~DynamicLibrary() {
        if (handle_) {
#ifdef PLATFORM_WINDOWS
            FreeLibrary((HMODULE)handle_);
#else
            dlclose(handle_);
#endif
        }
    }

    // Rule of Five: Disable copy, enable move
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    DynamicLibrary(DynamicLibrary&& other) noexcept
        : libName_(std::move(other.libName_))
        , handle_(other.handle_) {
        other.handle_ = nullptr;  // Transfer ownership
    }

    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept {
        if (this != &other) {
            // Close current handle if open
            if (handle_) {
#ifdef PLATFORM_WINDOWS
                FreeLibrary((HMODULE)handle_);
#else
                dlclose(handle_);
#endif
            }

            // Transfer ownership from other
            libName_ = std::move(other.libName_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    bool load() {
#ifdef PLATFORM_WINDOWS
        handle_ = LoadLibraryA(libName_.c_str());
#else
        handle_ = dlopen(libName_.c_str(), RTLD_LAZY);
#endif
        if (!handle_) {
            Logger::error("Failed to load library: " + libName_);
            return false;
        }
        return true;
    }

    template<typename T>
    T getFunction(const std::string& funcName) {
        if (!handle_) {
            Logger::error("Cannot get function '" + funcName + "': Library '" + libName_ + "' not loaded");
            return nullptr;
        }

#ifdef PLATFORM_WINDOWS
        T func = (T)GetProcAddress((HMODULE)handle_, funcName.c_str());
        if (!func) {
            DWORD error = GetLastError();
            Logger::error("Failed to load function '" + funcName + "' from '" + libName_ +
                         "': Windows error code " + std::to_string(error));
        }
        return func;
#else
        // Clear any previous error
        dlerror();

        T func = (T)dlsym(handle_, funcName.c_str());

        // Check for errors
        const char* error = dlerror();
        if (error) {
            Logger::error("Failed to load function '" + funcName + "' from '" + libName_ + "': " + std::string(error));
        }

        return func;
#endif
    }

private:
    std::string libName_;
    void* handle_;
};

#endif // DYNAMIC_LIBRARY_H

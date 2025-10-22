
#ifndef DYNAMIC_LIBRARY_H
#define DYNAMIC_LIBRARY_H

#include <string>
#include <memory>
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
            return nullptr;
        }
#ifdef PLATFORM_WINDOWS
        return (T)GetProcAddress((HMODULE)handle_, funcName.c_str());
#else
        return (T)dlsym(handle_, funcName.c_str());
#endif
    }

private:
    std::string libName_;
    void* handle_;
};

#endif // DYNAMIC_LIBRARY_H

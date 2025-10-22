// cef_stubs.cpp
// Stub implementations for CEF functions that are normally in libcef_dll.cc/libcef_dll2.cc
// These functions are implemented in the wrapper, not dynamically loaded from libcef.so

#include "include/cef_version.h"
#include "include/cef_api_hash.h"
#include "include/internal/cef_types.h"
#include "include/internal/cef_string.h"

// From libcef_dll2.cc - version info functions
extern "C" {

int cef_version_info(int entry) {
    switch (entry) {
        case 0: return CEF_VERSION_MAJOR;
        case 1: return CEF_VERSION_MINOR;
        case 2: return CEF_VERSION_PATCH;
        case 3: return CEF_COMMIT_NUMBER;
        case 4: return CHROME_VERSION_MAJOR;
        case 5: return CHROME_VERSION_MINOR;
        case 6: return CHROME_VERSION_BUILD;
        case 7: return CHROME_VERSION_PATCH;
        default: return 0;
    }
}

const char* cef_api_hash(int entry) {
    switch (entry) {
        case 0: return CEF_API_HASH_PLATFORM;
        case 1: return CEF_API_HASH_UNIVERSAL;
        case 2: return CEF_COMMIT_HASH;
        default: return nullptr;
    }
}

// From libcef_dll.cc - test helper functions (we provide stubs since we don't use them)
void cef_execute_java_script_with_user_gesture_for_tests(
    struct _cef_frame_t* frame,
    const cef_string_t* javascript) {
    // Stub - not implemented (test function)
}

void cef_set_data_directory_for_tests(const cef_string_t* dir) {
    // Stub - not implemented (test function)
}

int cef_is_feature_enabled_for_tests(const cef_string_t* feature_name) {
    // Stub - not implemented (test function)
    return 0;
}

} // extern "C"

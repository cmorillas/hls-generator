cmake_minimum_required(VERSION 3.16)
project(hls-generator VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Platform detection (set variables for later use with target-specific commands)
if(WIN32)
    message(STATUS "Building for Windows")
    set(PLATFORM_DEFINITIONS PLATFORM_WINDOWS OS_WIN=1)
    set(PLATFORM_COMPILE_OPTIONS)
    if(NOT MSVC)
        list(APPEND PLATFORM_COMPILE_OPTIONS -U__linux__)
    endif()
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
    set(PLATFORM_DEFINITIONS PLATFORM_LINUX OS_LINUX=1 CEF_X11=1)
    set(PLATFORM_COMPILE_OPTIONS)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# ===== CEF Scripts Embedding =====
# Find all .js files in js-inject/ and embed them in the binary
file(GLOB JS_SCRIPTS "${CMAKE_SOURCE_DIR}/js-inject/*.js")
list(SORT JS_SCRIPTS)  # Alphabetical order (numeric prefixes control execution order)

if(JS_SCRIPTS)
    # Create generated directory
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

    # Generate individual headers for each script
    set(ALL_SCRIPT_INCLUDES "")
    set(ALL_SCRIPT_ARRAY "")

    foreach(JS_FILE ${JS_SCRIPTS})
        get_filename_component(JS_NAME ${JS_FILE} NAME_WE)  # Filename without extension

        # Convert filename to valid C++ variable name (e.g., 01-cookie-consent-killer → _01_cookie_consent_killer_js)
        string(REGEX REPLACE "[^a-zA-Z0-9]" "_" VAR_NAME "${JS_NAME}")
        set(VAR_NAME "_${VAR_NAME}_js")

        # Read JavaScript file content
        file(READ ${JS_FILE} JS_CONTENT)

        # Generate individual header
        set(HEADER_FILE "${CMAKE_BINARY_DIR}/generated/${VAR_NAME}.h")
        file(WRITE ${HEADER_FILE}
             "// Auto-generated from ${JS_NAME}.js - DO NOT EDIT\n"
             "#pragma once\n\n"
             "static const char* ${VAR_NAME} = R\"JSDELIMITER(\n"
             "${JS_CONTENT}\n"
             ")JSDELIMITER\";\n")

        # Build list for master header
        list(APPEND ALL_SCRIPT_INCLUDES "#include \"${VAR_NAME}.h\"\n")
        list(APPEND ALL_SCRIPT_ARRAY "    ${VAR_NAME},\n")

        message(STATUS "Embedded CEF script: ${JS_NAME}.js → ${VAR_NAME}")
    endforeach()

    # Generate master header with all scripts
    string(REPLACE ";" "" ALL_SCRIPT_INCLUDES "${ALL_SCRIPT_INCLUDES}")
    string(REPLACE ";" "" ALL_SCRIPT_ARRAY "${ALL_SCRIPT_ARRAY}")

    set(MASTER_HEADER "${CMAKE_BINARY_DIR}/generated/all_cef_scripts.h")
    file(WRITE ${MASTER_HEADER}
         "// Auto-generated - All CEF injection scripts - DO NOT EDIT\n"
         "#pragma once\n\n"
         "${ALL_SCRIPT_INCLUDES}\n"
         "// Array of all scripts in execution order\n"
         "static const char* const all_cef_scripts[] = {\n"
         "${ALL_SCRIPT_ARRAY}"
         "    nullptr  // Sentinel\n"
         "};\n")

    # Save generated directory for target-specific include
    set(GENERATED_HEADERS_DIR ${CMAKE_BINARY_DIR}/generated)

    message(STATUS "Generated master header: all_cef_scripts.h")
else()
    message(WARNING "No .js files found in js-inject/ directory")
endif()
# ===== End CEF Scripts Embedding =====

# CEF (Chromium Embedded Framework) headers and wrapper
# Headers for compilation, wrapper for C++ API, runtime uses OBS's libcef.so
set(CEF_ROOT_DIR "${CMAKE_SOURCE_DIR}/external/cef")
if(EXISTS "${CEF_ROOT_DIR}/include")
    set(CEF_INCLUDE_DIR ${CEF_ROOT_DIR})
    message(STATUS "CEF headers found: ${CEF_ROOT_DIR}/include")

    # Add libcef_dll_wrapper subdirectory (provides C++ API wrappers)
    if(EXISTS "${CEF_ROOT_DIR}/libcef_dll/CMakeLists.txt")
        add_subdirectory(${CEF_ROOT_DIR}/libcef_dll libcef_dll_wrapper)
        message(STATUS "Building libcef_dll_wrapper from external/cef/libcef_dll")
    endif()
else()
    message(FATAL_ERROR "CEF headers not found in external/cef/include. This is required for compilation.")
endif()

# FFmpeg headers (for structure definitions only, NOT for linking)
# We load FFmpeg functions dynamically from OBS Studio at runtime
# Headers are only needed for compilation to know structure definitions

# Strategy: Try system headers first (fastest), fallback to FetchContent (automatic download)
set(FFMPEG_HEADERS_FOUND FALSE)

# Try system headers first via pkg-config (only for native builds, not cross-compilation)
if(NOT CMAKE_CROSSCOMPILING)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBAV QUIET libavformat libavcodec libavutil libswscale)
        if(LIBAV_FOUND)
            set(FFMPEG_INCLUDE_DIRS ${LIBAV_INCLUDE_DIRS})
            message(STATUS "FFmpeg headers found in system: ${LIBAV_INCLUDE_DIRS}")
            set(FFMPEG_HEADERS_FOUND TRUE)
        endif()
    endif()
endif()

# Fallback 2: Try local headers downloaded by scripts (external/ffmpeg/linux or external/ffmpeg/windows)
if(NOT FFMPEG_HEADERS_FOUND)
    if(WIN32 OR CMAKE_CROSSCOMPILING)
        set(FFMPEG_LOCAL_HEADERS "${CMAKE_SOURCE_DIR}/external/ffmpeg/windows")
    else()
        set(FFMPEG_LOCAL_HEADERS "${CMAKE_SOURCE_DIR}/external/ffmpeg/linux")
    endif()

    if(EXISTS "${FFMPEG_LOCAL_HEADERS}")
        set(FFMPEG_INCLUDE_DIRS ${FFMPEG_LOCAL_HEADERS})
        message(STATUS "FFmpeg headers found in local directory: ${FFMPEG_LOCAL_HEADERS}")
        set(FFMPEG_HEADERS_FOUND TRUE)
    endif()
endif()

# Fallback 3: Automatic download via FetchContent (works on all platforms)
if(NOT FFMPEG_HEADERS_FOUND)
    message(STATUS "FFmpeg headers not found locally, using FetchContent for automatic download...")

    include(FetchContent)

    set(FFMPEG_VERSION "7.0.2")
    message(STATUS "Downloading FFmpeg ${FFMPEG_VERSION} source (headers only)...")

    FetchContent_Declare(
        ffmpeg
        URL https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz
        URL_HASH SHA256=8646515b638a3ad303e23af6a3587734447cb8fc0a0c064ecdb8e95c4fd8b389
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    # Download and extract (does not build FFmpeg, just extracts headers)
    FetchContent_MakeAvailable(ffmpeg)

    # Set header paths (all lib* directories contain headers we need)
    set(FFMPEG_INCLUDE_DIRS
        ${ffmpeg_SOURCE_DIR}/libavcodec
        ${ffmpeg_SOURCE_DIR}/libavdevice
        ${ffmpeg_SOURCE_DIR}/libavfilter
        ${ffmpeg_SOURCE_DIR}/libavformat
        ${ffmpeg_SOURCE_DIR}/libavutil
        ${ffmpeg_SOURCE_DIR}/libswscale
        ${ffmpeg_SOURCE_DIR}/libswresample
    )

    message(STATUS "FFmpeg headers downloaded via FetchContent: ${ffmpeg_SOURCE_DIR}")
    message(STATUS "  - This happens automatically during CMake configuration")
    message(STATUS "  - Headers are cached in build/_deps/ffmpeg-src/ for future builds")
    set(FFMPEG_HEADERS_FOUND TRUE)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/obs_detector.cpp
    src/logger.cpp
    src/ffmpeg_loader.cpp
    src/ffmpeg_wrapper.cpp
    src/cef_loader.cpp
    src/cef_function_wrappers.cpp
    src/cef_stubs.cpp
    src/hls_generator.cpp
    src/stream_input.cpp
    src/ffmpeg_input.cpp
    src/browser_input.cpp
)

# Platform-specific browser backend
# CEF backend on both platforms (uses OBS's libcef.dll/so via dynamic loading)
list(APPEND SOURCES
    src/cef_backend.cpp
    src/browser_backend_factory.cpp
)

# Executable
add_executable(hls-generator ${SOURCES})

# Modern CMake: Target-specific properties instead of global commands
# Platform definitions
target_compile_definitions(hls-generator PRIVATE ${PLATFORM_DEFINITIONS})

# Platform-specific compile options
if(PLATFORM_COMPILE_OPTIONS)
    target_compile_options(hls-generator PRIVATE ${PLATFORM_COMPILE_OPTIONS})
endif()

# Release optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(hls-generator PRIVATE /O2)
    else()
        target_compile_options(hls-generator PRIVATE -O2)
        target_compile_definitions(hls-generator PRIVATE NDEBUG)
    endif()
endif()

# Include directories
target_include_directories(hls-generator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CEF_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
)

# Add generated headers if available
if(GENERATED_HEADERS_DIR)
    target_include_directories(hls-generator PRIVATE ${GENERATED_HEADERS_DIR})
endif()

# Warnings
if(MSVC)
    target_compile_options(hls-generator PRIVATE /W4)
else()
    target_compile_options(hls-generator PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Platform-specific libraries (NO FFmpeg/CEF system libraries needed!)
# Both platforms load libraries dynamically at runtime
if(WIN32)
    # Windows: System libraries + CEF wrapper
    target_link_libraries(hls-generator
        ws2_32
        shell32
        libcef_dll_wrapper  # C++ API wrapper
    )

    # Static linking for MinGW runtime (self-contained binary)
    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
        message(STATUS "Enabling static linking for Windows (MinGW)")
    endif()
else()
    # Linux: System libraries + CEF wrapper (NO static libcef.so linking)
    target_link_libraries(hls-generator
        pthread
        dl
        libcef_dll_wrapper  # C++ API wrapper
    )

    # Optional: Semi-static linking for better portability
    # Enable with: cmake -DSTATIC_STDLIB=ON
    option(STATIC_STDLIB "Link libstdc++ and libgcc statically" OFF)
    if(STATIC_STDLIB)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        message(STATUS "Enabling semi-static linking (libstdc++, libgcc)")
    endif()
endif()

message(STATUS "Using CEF from OBS Studio (loaded dynamically at runtime)")
message(STATUS "Linking with: libcef_dll_wrapper (C++ API)")

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")

# Strip symbols in Release builds to reduce binary size
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(WIN32)
        # Windows cross-compilation: strip after build
        add_custom_command(TARGET hls-generator POST_BUILD
            COMMAND ${CMAKE_STRIP} $<TARGET_FILE:hls-generator>
            COMMENT "Stripping debug symbols from Windows executable"
        )
    else()
        # Linux: strip after build
        add_custom_command(TARGET hls-generator POST_BUILD
            COMMAND ${CMAKE_STRIP} $<TARGET_FILE:hls-generator>
            COMMENT "Stripping debug symbols from Linux executable"
        )
    endif()
endif()

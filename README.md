# HLS Generator

[![Version](https://img.shields.io/github/v/release/cmorillas/hls-generator)](https://github.com/cmorillas/hls-generator/releases)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/cmorillas/hls-generator)

HLS stream generator from video files and browser sources, using FFmpeg and CEF libraries included in OBS Studio via **dynamic runtime loading**.

## Features

### Core Capabilities
✅ **Dynamic FFmpeg loading** - No FFmpeg installation required on the system
✅ **Dynamic CEF loading** - Browser source support using OBS's CEF installation
✅ **Uses OBS libraries** - Automatically updated when OBS is updated
✅ **Cross-platform** - Linux and Windows
✅ **Zero-dependency binaries** - Only requires OBS Studio installed
✅ **Auto-generated wrappers** - 175 CEF functions loaded dynamically

### Audio/Video Features
✅ **Full audio support** - AAC encoding from CEF browser sources
✅ **Audio/video synchronization** - Automatic sync via FFmpeg timebases
✅ **Page reload handling** - Seamless encoder reset on browser page reloads
✅ **Live HLS streaming** - Event-type playlists for continuous playback
✅ **Low-latency HLS** - 4-6 second latency (2s segments, optimized for live playback)
✅ **Auto cookie acceptance** - JavaScript injection for GDPR cookie banners

### Technical Robustness
✅ **Encoder flush on reset** - Clean buffer management preventing DTS errors
✅ **CEF singleton handling** - Proper lifecycle management without reinitialization
✅ **Simplified PTS system** - Robust monotonic timestamps for A/V sync
✅ **Separation of concerns** - Clean architecture with proper component isolation

## Requirements

### Runtime (to execute the binary)
- **OBS Studio** installed on the system (provides FFmpeg and CEF libraries)

### Compilation (to build from source)
- CMake 3.16+
- C++17 compiler (GCC/Clang/MSVC)
- **FFmpeg headers** - Three options (in order of priority):
  - **Option A**: Install dev packages: `sudo apt install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev` (fastest, uses system headers)
  - **Option B**: Download headers once: `./scripts/linux/download-ffmpeg-headers-linux.sh` (offline compilation, cached locally)
  - **Option C**: Nothing! CMake downloads automatically via FetchContent (slowest first time, but always works)

**Note**: FFmpeg headers are used only during compilation to define structures. The resulting binary does **NOT** depend on FFmpeg libraries, only on OBS.

**How it works**: CMake tries system headers first (fastest), then local headers (`external/ffmpeg/`), then downloads automatically if needed. Choose the option that fits your workflow.

## Compilation

### Linux

**Quick start (automatic headers download):**
```bash
# Install build dependencies
sudo apt install cmake g++

# Compile (CMake downloads FFmpeg headers automatically if needed)
cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j4
```

**Recommended (faster, offline-ready):**
```bash
# 1. Install build dependencies
sudo apt install cmake g++

# 2. Download FFmpeg headers once (cached for offline use)
./scripts/linux/download-ffmpeg-headers-linux.sh

# 3. Compile (uses cached headers, no download)
cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j4
```

**Alternative (use system headers):**
```bash
sudo apt install cmake g++ libavformat-dev libavcodec-dev libavutil-dev libswscale-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j4
```

**Output**: `build/hls-generator` (1.2 MB stripped, Release build)

**Linking options:**
- **Dynamic** (default): 1.2 MB - Depends on libstdc++, libgcc, libc
- **Semi-static** (`-DSTATIC_STDLIB=ON`): ~4 MB - Only depends on libc (better portability)

### Windows (Cross-compilation from Linux)

**Quick start (automatic headers download):**
```bash
# Install MinGW cross-compiler
sudo apt install mingw-w64

# Cross-compile (CMake downloads FFmpeg headers automatically if needed)
cmake -B build-windows -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw.cmake -DCMAKE_BUILD_TYPE=Release
make -C build-windows -j4
```

**Recommended (faster, offline-ready):**
```bash
# 1. Install MinGW cross-compiler
sudo apt install mingw-w64

# 2. Download FFmpeg headers for Windows once
./scripts/linux/download-ffmpeg-headers-windows.sh

# 3. Cross-compile (uses cached headers, no download)
cmake -B build-windows -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw.cmake -DCMAKE_BUILD_TYPE=Release
make -C build-windows -j4
```

**Output**: `build-windows/hls-generator.exe` (2.0 MB stripped, Release build)

**Windows binary characteristics:**
- **Fully static**: libstdc++ and libgcc embedded (no runtime DLLs needed)
- **Minimal dependencies**: Only KERNEL32.dll and msvcrt.dll (always present in Windows)
- **No Visual C++ Redistributables needed**
- **Self-contained**: Single .exe file, ready to distribute

See [docs/BUILD-GUIDE.md](docs/BUILD-GUIDE.md) for all build options and platforms.

### Windows (Native compilation with MSYS2/MinGW)

```bash
# 1. Install dependencies (only once)
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake

# 2. Get FFmpeg headers (choose one):
# Option A: Install from MSYS2
pacman -S mingw-w64-x86_64-ffmpeg
# Option B: Download headers only (recommended if you have OBS)
scripts/windows/download-ffmpeg-headers-windows.bat

# 3. Compile
cmake -B build/windows -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
make -C build/windows
```

## Usage

```bash
./hls-generator [OPTIONS] <video_input> <output_directory>
```

### Options

- `--no-js` - Disable JavaScript injection (no automatic cookie consent handling)

### Examples

**Basic usage (video file):**
```bash
./hls-generator /path/to/video.mp4 /path/to/hls_output
```

**Browser source with automatic cookie consent:**
```bash
./hls-generator https://www.youtube.com/watch?v=dQw4w9WgXcQ /path/to/hls_output
```

**Browser source WITHOUT JavaScript injection:**
```bash
./hls-generator --no-js https://example.com /path/to/hls_output
```

### Output

The program will generate:
- `playlist.m3u8` - HLS playlist
- `segment000.ts`, `segment001.ts`, ... - Video segments

## How It Works

### Dynamic Library Loading

1. **Detects OBS Studio** automatically on the system
2. **Dynamically loads FFmpeg libraries** from OBS:
   - `libavformat.so` / `avformat-60.dll`
   - `libavcodec.so` / `avcodec-60.dll`
   - `libavutil.so` / `avutil-58.dll`
3. **Dynamically loads CEF library** from OBS:
   - `libcef.so` / `libcef.dll`
   - 175 CEF C API functions loaded via dlopen/LoadLibrary
4. **Processes media** using these libraries
5. **Generates HLS segments** compatible with any player

See [CEF-DYNAMIC-LOADING.md](docs/CEF-DYNAMIC-LOADING.md) for detailed implementation guide.

### Verify Dependencies

To verify that the binary does NOT depend on system FFmpeg or CEF:

```bash
ldd ./hls-generator | grep -E "(libav|libcef)"
```

Should show nothing. Only depends on libc, libstdc++, etc.

## Project Status

### Core Infrastructure (Completed ✅)
- [x] Basic structure and build system (CMake, cross-platform)
- [x] Automatic OBS detection (Linux/Windows)
- [x] Logging system with timestamps and levels
- [x] Dynamic FFmpeg library loading (220+ functions)
- [x] Dynamic CEF library loading (175 functions)
- [x] FFmpeg integration with encoding pipeline

### HLS Generation (Completed ✅)
- [x] HLS segment generation (configurable duration)
- [x] playlist.m3u8 creation and updating
- [x] Live streaming mode (event-type playlists)
- [x] Automatic segment cleanup (rolling window)
- [x] Bitstream filters (h264_mp4toannexb)

### Browser Source (Completed ✅)
- [x] CEF browser backend (Linux & Windows)
- [x] Browser source input implementation
- [x] Video capture from CEF (BGRA → YUV → H.264)
- [x] Audio capture from CEF (PCM → AAC)
- [x] JavaScript injection for cookie auto-acceptance
- [x] Page reload detection and handling
- [x] Encoder reset without CEF reinitialization

### Audio/Video Synchronization (Completed ✅)
- [x] AAC audio encoding (44.1kHz/48kHz, stereo/mono)
- [x] Audio/video timebase synchronization
- [x] Simplified PTS system (monotonic from 0)
- [x] Encoder flush on reset (prevents DTS errors)
- [x] Pre-page-load audio discarding
- [x] Automatic sync via av_interleaved_write_frame()

### Known Limitations
- [ ] Embedded HTTP server - Use external server (e.g., `python -m http.server`)
- [ ] Multiple browser instances - Currently single browser source
- [ ] Custom video resolution - Fixed at 1280x720@30fps

## Project Structure

```
hls-generator/
├── src/                    # Source code (.cpp + .h)
│   ├── cef_loader.h        # CEF dynamic loader
│   └── ...                 # Other implementation files
├── js-inject/              # JavaScript injection scripts (NEW ⭐)
│   └── 01-cookie-consent-killer.js  # Auto cookie consent (embedded at compile-time)
├── external/               # External dependencies
│   ├── cef/                # CEF headers + wrapper (3.6 MB - in repo)
│   │   ├── include/        # CEF C++ API headers (public interface)
│   │   └── libcef_dll/     # CEF wrapper (CppToC/CtoCpp converters) ⚠️ CRITICAL
│   │       ├── cpptoc/     # 100+ C++ to C converters (auto-generated by CEF)
│   │       ├── ctocpp/     # 100+ C to C++ converters (auto-generated by CEF)
│   │       └── wrapper/    # Additional wrapper utilities
│   └── ffmpeg/             # FFmpeg headers (downloaded locally, not in repo)
│       ├── linux/          # Headers for Linux compilation
│       └── windows/        # Headers for Windows compilation
├── dev-scripts/            # Development and build scripts
│   ├── linux/              # Scripts to run on Linux (.sh)
│   │   ├── download-ffmpeg-headers-linux.sh
│   │   └── download-ffmpeg-headers-windows.sh
│   └── windows/            # Scripts to run on Windows (.bat)
│       ├── download-ffmpeg-headers-linux.bat
│       └── download-ffmpeg-headers-windows.bat
├── build/                  # Build artifacts (ignored by git)
│   └── generated/          # Auto-generated headers from js-inject/ (compile-time)
│       ├── _01_cookie_consent_killer_js.h  # Individual script header
│       └── all_cef_scripts.h               # Master header with all scripts
├── docs/                   # Documentation
└── CMakeLists.txt          # Build configuration
```

**Key points:**
- `src/` contains all source code and headers - no separate `include/` directory needed
- `js-inject/` ⭐ **NEW**: JavaScript files automatically embedded at compile-time
  - Add any `.js` file here with numeric prefix (e.g., `02-analytics.js`)
  - CMake automatically generates C++ headers in `build/generated/`
  - Scripts are injected in alphabetical order (prefix controls execution order)
  - Clean JavaScript code, no C++ string escaping needed
- `external/cef/` is in the repository (specific CEF version needed)
  - ⚠️ **CRITICAL**: `libcef_dll/` contains the COMPLETE auto-generated wrapper from CEF
  - ❌ **DO NOT** attempt to create a "minimal" version - see [docs/DEVELOPMENT-JOURNEY.md#desafío-6](docs/DEVELOPMENT-JOURNEY.md#desafío-6-el-wrapper-cef-completo-vs-minimalista)
  - The wrapper implements the C++↔C conversion layer essential for CEF dynamic loading
- `external/ffmpeg/` is NOT in the repository (downloaded locally via scripts)
- `build/` contains temporary compilation artifacts (can be deleted anytime)
  - `build/generated/` contains auto-generated headers from `js-inject/` (recreated on each CMake run)
- `dev-scripts/` contains scripts for development and compilation, organized by platform

## Documentation

- [docs/DEVELOPMENT-JOURNEY.md](docs/DEVELOPMENT-JOURNEY.md) ⭐ **Start here** - Complete development story, all problems solved, and lessons learned
- [docs/BUILD-GUIDE.md](docs/BUILD-GUIDE.md) - Complete build guide for all platforms (Linux, Windows, cross-compilation)
- [docs/TECHNICAL-REFERENCE.md](docs/TECHNICAL-REFERENCE.md) - Complete technical reference for FFmpeg + CEF dynamic loading
- [docs/SPECIFICATIONS.md](docs/SPECIFICATIONS.md) - Complete technical specifications
- [external/README.md](external/README.md) - External dependencies explanation
- [docs/README.md](docs/README.md) - Full documentation index

## Recent Updates

### v1.4.1 (2025-01-23): Low-Latency HLS Optimization ⚡
Optimized HLS configuration for reduced playback latency in live streaming scenarios.

**⚡ Latency Improvements:**
- ✅ **Reduced segment duration** - 3s → 2s (33% faster segment generation)
- ✅ **Optimized playlist size** - 5 → 3 segments (faster player startup)
- ✅ **GOP alignment** - One IDR keyframe per segment (no buffering needed)
- ✅ **Latency reduction** - From ~9-15s to ~4-6s in VLC and similar players

**Configuration Changes:**
- `segmentDuration`: 3s → 2s
- `playlistSize`: 5 → 3 segments
- `gop_size`: 60 frames (aligned with 2s @ 30fps)

**Benefits:**
- Faster initial playback startup
- Lower end-to-end latency for live streams
- Better user experience in VLC, mpv, and browser players
- Maintains HLS compatibility and quality

---

### v1.4.0 (2025-01-23): Modular Architecture & Memory Safety 🏗️
Complete architectural refactoring with zero memory leaks and immutable configuration.

**🏗️ Modular Pipeline Architecture:**
- ✅ **FFmpegContext** - Single point of FFmpeg library loading (eliminates dual loading)
- ✅ **VideoPipeline** - Specialized video processing (REMUX/TRANSCODE/PROGRAMMATIC)
- ✅ **AudioPipeline** - Specialized audio processing with PTS tracking
- ✅ **Separation of concerns** - Clean, maintainable architecture

**🛡️ Memory Safety (RAII):**
- ✅ **Zero memory leaks** - 9 memory leaks fixed with custom deleters
- ✅ **Automatic cleanup** - All FFmpeg resources freed via smart pointers
- ✅ **Type-aware deleters** - AVCodecContext, AVFrame, SwsContext, SwrContext, AVBSFContext

**🔒 Thread-Safe Configuration:**
- ✅ **Immutable AppConfig** - Passed via constructor as const member
- ✅ **No runtime mutations** - Configuration cannot change after construction
- ✅ **Cleaner API** - `FFmpegWrapper(config)` instead of `wrapper.setConfig(config)`

**🧹 Code Cleanup:**
- ✅ **Eliminated legacy code** - Removed `ffmpeg_loader` and `FFmpegLib::` namespace
- ✅ **Removed dead code** - Eliminated unused `MuxerManager` component
- ✅ **Reduced codebase** - ~500 lines of duplicated/legacy code removed

**⚡ Performance:**
- ✅ **Single library load** - FFmpeg loaded once and shared (was: dual loading)
- ✅ **Faster startup** - Reduced initialization overhead
- ✅ **Better memory usage** - Shared context pattern

**Breaking Changes:**
- Constructor now requires config: `FFmpegWrapper wrapper(config);`
- `setConfig()` method removed (no longer needed)
- `setupOutput()` no longer takes config parameter

See [CHANGELOG.md](CHANGELOG.md) for complete details and [ARCHITECTURE.md](ARCHITECTURE.md) for architecture documentation.

---

### v1.3.0 (2025-10-23): Code Quality Deep Dive - 4 Critical Fixes 🔍
Comprehensive code quality improvements resolving critical functional and robustness issues.

**Critical Fixes:**
- ✅ **Audio in TRANSCODE mode** - Intelligent audio handling: AAC → remux (fast), non-AAC → transcode to AAC
- ✅ **Dynamic FFmpeg loading** - Completed in `ffmpeg_loader.cpp`: tries unversioned → known versions → dynamic scan
- ✅ **SwsContext auto-recreation** - Uses `sws_getCachedContext()` to handle resolution changes mid-stream
- ✅ **AppConfig propagation** - Backends now receive correct configuration via `setConfig()` before `openInput()`

**Audio Processing Strategy (Explicit & Automatic):**
- **AAC audio** → REMUX (copy without transcoding) ⚡ Efficient, no quality loss
- **Non-AAC audio** (MP3/Opus/Vorbis/etc.) → TRANSCODE to AAC 🔄 HLS-compatible
- Automatic detection with clear logging: `"Audio is AAC - will REMUX"` or `"Audio codec ID 86018 (non-AAC) - will TRANSCODE to AAC"`

**Technical Improvements:**
- Added `setupAudioEncoder()` - AAC encoder configuration (128 kbps, preserves sample rate)
- Added `tryLoadLibrary()` - 3-phase FFmpeg library loading with detailed logging
- Added `sws_getCachedContext()` to FFmpegLib - Automatic context recreation when input dimensions change
- Added `setConfig()` method - Ensures backends get configuration before instantiation

**Robustness:**
- Prevents audio loss in transcode mode
- Prevents crashes from invalid scaling context
- Prevents configuration mismatches in browser input
- Future-proof FFmpeg loading (works with versions 59-65+)

**Impact**: 4 critical issues resolved, audio now works in all scenarios, future-proof library loading, safer video scaling.

**✅ Audio Transcoding Complete (SwrContext Implementation):**
- ✅ **SwrContext integration** - Full audio format conversion (MP3, Opus, Vorbis → AAC)
- ✅ **Modern FFmpeg APIs** - Uses `swr_alloc_set_opts2()` and `swr_convert_frame()` (FFmpeg 5.1+)
- ✅ **Complete audio flushing** - Symmetric decoder/encoder draining (no audio loss)
- ✅ **Cached frame optimization** - Reuses `convertedFrame_` for better performance
- ✅ **Dynamic swresample loading** - Auto-detects any swresample version

**Technical Implementation:**
- Added `libswresample` dynamic loading with version detection
- Created `SwrContextDeleter` for RAII cleanup
- Fixed decoder initialization order (decoder → encoder → SwrContext)
- Fixed `av_frame_unref()` issue by reconfiguring frame fields before conversion

See [DEVELOPMENT-JOURNEY.md - SwrContext Implementation](docs/DEVELOPMENT-JOURNEY.md#solución-completa-swrcontext-implementation-v130-final) for complete technical details.

---

### v1.2.0 (2025-10-22): Dynamic FFmpeg Version Detection 🔮
Future-proof FFmpeg library detection to prevent breakage when OBS updates.

**What's New:**
- ✅ **Dynamic FFmpeg scanning** - Automatically detects any FFmpeg version (59, 60, 61, 62, 63+)
- ✅ **No more hardcoded versions** - Scans directories for `avformat-*.dll` / `libavformat.so.*`
- ✅ **Fast-path optimization** - Checks known versions first, falls back to dynamic scan
- ✅ **Better logging** - Shows exact FFmpeg version detected (e.g., "libavformat.so.61")
- ✅ **Zero runtime impact** - Detection happens once at startup

**Technical Details:**
- Added `findFFmpegLibraries()` - Platform-specific directory scanning (Windows: `FindFirstFile`, Linux: `opendir`)
- Added `hasFFmpegLibraries()` - Two-phase check (known versions → dynamic scan)
- Updated all detection functions: `detectLinux()`, `detectWindows()`, `detectSystemFFmpeg()`

**Impact**: Future OBS updates won't break the project. Backwards compatible with FFmpeg 59+.

See [DEVELOPMENT-JOURNEY.md](docs/DEVELOPMENT-JOURNEY.md#desafío-11-dynamic-ffmpeg-version-detection-v120) for complete technical details.

---

### v1.1.0 (2025-10-22): Code Quality & Robustness Release 🔒
Major improvements focusing on stability, security, and maintainability.

**Critical Fixes:**
- ✅ **Logger thread-safety** - Fixed race condition using `localtime_r()`/`localtime_s()`
- ✅ **Race condition in sws_ctx_** - Eliminated frame corruption in browser capture
- ✅ **DynamicLibrary Rule of Five** - Prevents double-free bugs, enables move semantics

**UX & Debugging:**
- ✅ **Fail-fast validation** - Early input/output validation with clear error messages
- ✅ **Consistent error handling** - Critical errors now logged as errors (not warnings)
- ✅ **Detailed error logging** - DynamicLibrary reports OS-specific errors (`dlerror()`/`GetLastError()`)

**Code Quality:**
- ✅ **Eliminated magic numbers** - Replaced with descriptive constants
- ✅ **Modernized CMake** - Target-specific commands instead of global
- ✅ **Simplified argument parsing** - Clear and explicit logic

**Impact**: 0 critical bugs (was: 2 race conditions), complete thread safety, better debugging with detailed error messages.

See [DEVELOPMENT-JOURNEY.md](docs/DEVELOPMENT-JOURNEY.md#desafío-10-code-quality-y-robustness-improvements) for complete technical details.

---

### 2025-10-22: CEF Scripts Architecture Refactoring (Desafío 14)
- ✅ **Refactored JavaScript injection** - Moved from embedded C++ strings to external `.js` files
- ✅ **Automatic embedding system** - CMake detects and embeds all scripts from `cef-scripts/` at compile-time
- ✅ **Better maintainability** - Clean JavaScript code with proper syntax highlighting and linting
- ✅ **Extensible architecture** - Add new scripts by simply creating `.js` files with numeric prefixes
- ✅ **Reverted to working version** - Cookie consent killer back to version that worked with YouTube

See [DEVELOPMENT-JOURNEY.md](docs/DEVELOPMENT-JOURNEY.md#desafío-14-arquitectura-de-scripts-cef-externos-2025-10-22) for complete technical details.

### 2025-10-22: Audio/Video Synchronization & Encoder Reset (Desafío 13)
- ✅ **Fixed DTS monotonicity errors** - Proper encoder flush before reset
- ✅ **Simplified PTS system** - Audio PTS starts from 0, sync handled by FFmpeg
- ✅ **Encoder reset on page reload** - Clean buffer management without CEF restart
- ✅ **Perfect A/V sync** - Before and after page reloads (cookie acceptance)

See [DEVELOPMENT-JOURNEY.md](docs/DEVELOPMENT-JOURNEY.md#desafío-13-errores-dts-monotónicos-y-reset-de-encoders-2025-10-21) for complete technical details.

## License

MIT

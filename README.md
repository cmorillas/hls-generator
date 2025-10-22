# HLS Generator

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

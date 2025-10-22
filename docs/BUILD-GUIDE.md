# Build Guide - All Platforms

Complete guide for building HLS Generator on all platforms and configurations.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Linux Native Build](#linux-native-build)
3. [Windows Native Build](#windows-native-build)
4. [Cross-Compilation Linux → Windows](#cross-compilation-linux--windows)
5. [Cross-Compilation Windows → Linux](#cross-compilation-windows--linux)
6. [Build Options](#build-options)
7. [Binary Characteristics](#binary-characteristics)
8. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Prerequisites (All Platforms)

- **CMake** 3.16 or newer
- **C++17 compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **OBS Studio** installed on target system (for runtime)
- **FFmpeg headers** (for compilation - see below)

### FFmpeg Headers Setup (Choose One Option)

You need FFmpeg headers (.h files) to compile, but **NOT** the full FFmpeg libraries since the project loads them dynamically from OBS Studio at runtime.

#### **Option A: Install Development Packages** (Traditional)

**Linux (Debian/Ubuntu):**
```bash
sudo apt install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install ffmpeg-devel
```

**Linux (Arch):**
```bash
sudo pacman -S ffmpeg
```

**Windows (MSYS2/MinGW):**
```bash
pacman -S mingw-w64-x86_64-ffmpeg
```

**Windows (vcpkg):**
```bash
vcpkg install ffmpeg:x64-windows
```

#### **Option B: Download Headers Only** (Recommended if you already have OBS)

If you already have OBS Studio installed, you don't need to install full FFmpeg development packages. Just download the headers (~25 MB):

**On Linux:**
```bash
# For native Linux compilation
./scripts/linux/download-ffmpeg-headers-linux.sh

# For cross-compilation to Windows
./scripts/linux/download-ffmpeg-headers-windows.sh
```

**On Windows:**
```cmd
REM For native Windows compilation
scripts\windows\download-ffmpeg-headers-windows.bat

REM For cross-compilation to Linux (rare)
scripts\windows\download-ffmpeg-headers-linux.bat
```

**Manual:**
```bash
# Download FFmpeg source
wget https://ffmpeg.org/releases/ffmpeg-7.0.2.tar.xz
tar -xf ffmpeg-7.0.2.tar.xz

# Linux: Extract only headers
mkdir -p external/ffmpeg/linux
cd ffmpeg-7.0.2
find libavcodec libavformat libavutil libswscale -name "*.h" -exec cp --parents {} ../external/ffmpeg/linux/ \;
cd ..

# Windows: Extract only headers
mkdir -p external/ffmpeg/windows
cd ffmpeg-7.0.2
find libavcodec libavformat libavutil libswscale -name "*.h" -exec cp --parents {} ../external/ffmpeg/windows/ \;
cd ..
```

**Build system behavior:**
- CMake will first try to find system-installed headers
- If not found, it will look for `external/ffmpeg/{linux,windows}/`
- This allows flexibility: use system packages OR downloaded headers
- `external/ffmpeg/` is ignored by git (local downloads only)

---

## Linux Native Build

### 1. Install Build Dependencies

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install cmake g++
# FFmpeg headers: See "FFmpeg Headers Setup" section above
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake gcc-c++
# FFmpeg headers: See "FFmpeg Headers Setup" section above
```

**Arch Linux:**
```bash
sudo pacman -S cmake gcc
# FFmpeg headers: See "FFmpeg Headers Setup" section above
```

### 2. Build (Standard - Dynamic Libraries)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Output**: `build/hls-generator` (2.7 MB)

**Dependencies**: libstdc++.so, libgcc_s.so, libc.so, libm.so

### 3. Build (Semi-Static - Better Portability)

```bash
mkdir build && cd build
cmake .. -DSTATIC_STDLIB=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Output**: `build/hls-generator` (4.1 MB)

**Dependencies**: libc.so only (works on 90%+ Linux distributions)

**Recommended for distribution**: Semi-static provides maximum compatibility while maintaining dlopen support.

### 4. Verify Build

```bash
# Check binary size
ls -lh build/hls-generator

# Check dependencies (should NOT show libcef or libav)
ldd build/hls-generator | grep -E "(libcef|libav)"
# Should be empty

# Test execution
./build/hls-generator --help
```

---

## Windows Native Build

### Using MSYS2/MinGW (Recommended)

#### 1. Install MSYS2

Download from: https://www.msys2.org/

#### 2. Install Build Tools

Open MSYS2 MinGW 64-bit terminal:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make
# FFmpeg headers: See "FFmpeg Headers Setup" section above
```

#### 3. Build

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Output**: `build/hls-generator.exe` (2.7 MB)

**Dependencies**: Only KERNEL32.dll and msvcrt.dll (included in Windows)

### Using Visual Studio

#### 1. Install Visual Studio 2017+

With C++ development tools.

#### 2. Setup FFmpeg Headers

See "FFmpeg Headers Setup" section above for options (vcpkg or downloaded headers).

#### 3. Build

```powershell
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

**Output**: `build/Release/hls-generator.exe`

---

## Cross-Compilation Linux → Windows

Build Windows executables from Linux using MinGW.

### 1. Install Cross-Compiler

**Debian/Ubuntu:**
```bash
sudo apt install mingw-w64 g++-mingw-w64-x86-64 cmake
```

**Fedora/RHEL:**
```bash
sudo dnf install mingw64-gcc-c++ mingw64-winpthreads-static cmake
```

**Arch Linux:**
```bash
sudo pacman -S mingw-w64-gcc cmake
```

### 2. Download FFmpeg Headers for Windows

```bash
mkdir -p build/ffmpeg-headers-windows
cd build/ffmpeg-headers-windows
wget https://ffmpeg.org/releases/ffmpeg-7.0.2.tar.xz
tar -xf ffmpeg-7.0.2.tar.xz
cp -r ffmpeg-7.0.2/libav* ./
echo '#define AV_HAVE_BIGENDIAN 0' > libavutil/avconfig.h
cd ../..
```

**Why needed**: Windows headers have different paths than Linux headers.

### 3. Cross-Compile

```bash
mkdir -p build/windows && cd build/windows
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-mingw.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Output**: `build/windows/hls-generator.exe` (3.1 MB)

### 4. Create Distribution Package

```bash
cd build/windows
zip -j ../../hls-generator-windows-x64.zip hls-generator.exe
cd ../..
```

**Output**: `hls-generator-windows-x64.zip` (619 KB compressed)

### 5. Test on Windows

Transfer the .exe to a Windows machine with OBS Studio installed:

```powershell
# On Windows
hls-generator.exe C:\path\to\video.mp4 C:\output
```

### 6. Test with Wine (Optional, Linux)

```bash
wine build/windows/hls-generator.exe --help
```

**Note**: Wine may not fully support all features.

---

## Cross-Compilation Windows → Linux

Build Linux executables from Windows using WSL2 or MinGW-w64.

### Method 1: WSL2 (Recommended)

#### 1. Install WSL2

```powershell
# On Windows PowerShell (Admin)
wsl --install
```

Reboot and set up Ubuntu.

#### 2. Build from WSL2

```bash
# Inside WSL2 Ubuntu
cd /mnt/c/Users/YourName/hls-generator
sudo apt update
sudo apt install cmake g++ libavformat-dev libavcodec-dev libavutil-dev

# Build for Linux (runs on WSL2 and native Linux)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_STDLIB=ON
make -j$(nproc)
```

**Output**: `build/hls-generator` (Linux ELF executable)

**Can run on**:
- ✅ WSL2 itself
- ✅ Native Linux systems (if semi-static)
- ❌ Windows directly (different executable format)

### Method 2: MinGW Cross-Compile to Linux (Advanced)

**Note**: This is extremely rare and not well-supported. Use WSL2 instead.

Cross-compiling from Windows to Linux requires a Linux cross-compiler toolchain which is difficult to set up on Windows.

**Alternatives**:
1. Use WSL2 (recommended)
2. Use a Linux VM (VirtualBox, VMware)
3. Use CI/CD (GitHub Actions, GitLab CI)
4. Use Docker on Windows

### Method 3: Docker on Windows

```powershell
# On Windows, create Dockerfile
docker run -v ${PWD}:/workspace -w /workspace ubuntu:22.04 bash -c "
    apt update &&
    apt install -y cmake g++ libavformat-dev libavcodec-dev libavutil-dev &&
    mkdir -p build && cd build &&
    cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_STDLIB=ON &&
    make -j\$(nproc)
"
```

**Output**: `build/hls-generator` (Linux binary)

---

## Cross-Compilation Summary

| From → To | Method | Difficulty | Recommended |
|-----------|--------|------------|-------------|
| **Linux → Windows** | MinGW-w64 | Easy | ✅ Yes |
| **Linux → Linux** | Native | Trivial | ✅ Yes |
| **Windows → Windows** | MSYS2/VS | Easy | ✅ Yes |
| **Windows → Linux** | WSL2 | Medium | ✅ Yes (WSL2) |
| **Windows → Linux** | MinGW cross | Very Hard | ❌ No (use WSL2) |
| **macOS → Linux** | Docker/VM | Medium | ✅ Yes |
| **macOS → Windows** | MinGW cross | Medium | ⚠️ Possible |

**General Rule**:
- **Linux → Windows**: Well-supported with MinGW
- **Windows → Linux**: Use WSL2 or Docker
- **macOS → Any**: Use Docker or VM

---

## Build Options

### CMake Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug, Release | Release | Build configuration |
| `STATIC_STDLIB` | ON, OFF | OFF | Static link libstdc++/libgcc (Linux only) |
| `CMAKE_TOOLCHAIN_FILE` | Path | - | Cross-compilation toolchain |

### Examples

**Debug build:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

**Semi-static release:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_STDLIB=ON
```

**Verbose build:**
```bash
make VERBOSE=1
```

**Clean build:**
```bash
rm -rf build
mkdir build && cd build
cmake .. && make
```

---

## Binary Characteristics

### Linux Dynamic (Default)

```
Size: 2.7 MB
Dependencies:
  - libc.so.6 (always present)
  - libstdc++.so.6 (common)
  - libgcc_s.so.1 (common)
  - libm.so.6 (always present)

Portability: Good (95% distributions)
Use case: Development, modern systems
```

### Linux Semi-Static (-DSTATIC_STDLIB=ON)

```
Size: 4.1 MB (+54%)
Dependencies:
  - libc.so.6 only

Portability: Excellent (99% distributions)
Use case: Distribution, production
Recommended: Yes
```

### Windows (MinGW)

```
Size: 2.7 MB (619 KB compressed)
Dependencies:
  - KERNEL32.dll (Windows system, always present)
  - msvcrt.dll (Windows system, always present)

Portability: Perfect (all Windows 7+)
Use case: Production, distribution
Features:
  - No Visual C++ Redistributables needed
  - Static libstdc++, libgcc
  - LoadLibrary compatible for dynamic loading
```

### Runtime Dependencies (All Platforms)

**None of these are linked** - they're loaded dynamically at runtime:

- **OBS Studio** must be installed
- **FFmpeg libraries** from OBS:
  - Linux: libavformat.so.61, libavcodec.so.61, libavutil.so.59
  - Windows: avformat-61.dll, avcodec-61.dll, avutil-59.dll
- **CEF library** from OBS:
  - Linux: libcef.so
  - Windows: libcef.dll

---

## Troubleshooting

### Build Issues

#### "CMake Error: Could not find FFmpeg"

**Linux:**
```bash
# Install development headers
sudo apt install libavformat-dev libavcodec-dev libavutil-dev
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-ffmpeg
```

#### "undefined reference to `dlopen`"

Add `-ldl` to linker flags. Should be automatic in CMakeLists.txt.

#### "C++17 required"

Update compiler:
```bash
# Debian/Ubuntu
sudo apt install g++-9

# Set as default
export CXX=g++-9
```

#### Cross-compilation: "toolchain file not found"

```bash
# Verify file exists
ls -la cmake/toolchain-mingw.cmake

# Use absolute path
cmake .. -DCMAKE_TOOLCHAIN_FILE=/full/path/to/cmake/toolchain-mingw.cmake
```

### Runtime Issues

#### "OBS Studio not found"

**Linux:**
```bash
# Verify OBS installed
which obs

# Verify libraries exist
ls -la /usr/lib/x86_64-linux-gnu/libavformat.so*
ls -la /usr/lib/x86_64-linux-gnu/obs-plugins/libcef.so
```

**Windows:**
```powershell
# Check OBS installation
dir "C:\Program Files\obs-studio\bin\64bit\obs64.exe"

# Check FFmpeg DLLs
dir "C:\Program Files\obs-studio\bin\64bit\avcodec*.dll"
```

**Solution**: Install OBS Studio from https://obsproject.com/download

#### "Failed to load FFmpeg library"

Check library version compatibility:

```bash
# Linux
ldd /usr/lib/x86_64-linux-gnu/libavformat.so
strings /usr/lib/x86_64-linux-gnu/libavformat.so | grep "LIBAVFORMAT"

# Windows
# OBS usually includes compatible FFmpeg version
```

#### Segmentation fault on startup

**Possible causes**:
1. Incompatible FFmpeg version
2. Missing CEF library
3. Corrupted build

**Debug**:
```bash
# Run with debugging
gdb ./hls-generator
(gdb) run
(gdb) bt  # backtrace on crash

# Or with logs
./hls-generator --verbose
```

---

## Performance Optimization

### Compile-Time Optimizations

```bash
# Maximum optimization
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Link-Time Optimization (LTO)
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

**Warning**: `-march=native` creates non-portable binaries optimized for your CPU.

### Size Optimization

```bash
# Optimize for size
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel

# Strip symbols
strip build/hls-generator

# Results:
# Before: 2.7 MB
# After strip: ~2.0 MB
# After UPX: ~800 KB (may break dynamic loading)
```

---

## Advanced Build Scenarios

### Custom FFmpeg Location

If FFmpeg is installed in non-standard location:

```bash
cmake .. -DFFMPEG_INCLUDE_DIR=/custom/path/include
```

### Static CEF Wrapper Only

The libcef_dll_wrapper is always compiled statically (~2 MB embedded in binary). libcef.so is always loaded dynamically at runtime.

### Multiple Configurations

Build both dynamic and semi-static:

```bash
# Dynamic
mkdir build-dynamic && cd build-dynamic
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

# Semi-static
mkdir build-static && cd build-static
cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_STDLIB=ON
make -j$(nproc)
cd ..
```

---

## Build System Details

### Directory Structure

```
build/
├── linux/          # Linux native build
└── windows/        # Windows cross-compile build
external/
├── cef/include/    # CEF headers
└── cef/wrapper/    # CEF wrapper base code
build/ffmpeg-headers-windows/  # Windows FFmpeg headers (local, not in repo)
```

### Toolchain File (cmake/toolchain-mingw.cmake)

Configures cross-compilation:
- Target: Windows x86_64
- Compiler: x86_64-w64-mingw32-g++
- Linker flags: -static -static-libgcc -static-libstdc++
- Find mode: Libraries from target only

### CMakeLists.txt Highlights

1. **Include directories**: external/cef, src/
2. **Compile libcef_dll_wrapper**: Static library from 188 source files
3. **Exclude problematic files**: libcef_dll.cc, libcef_dll2.cc
4. **Link libraries**: pthread, dl, libcef_dll_wrapper (NOT libcef.so)
5. **Optional static linking**: STATIC_STDLIB option

---

## Distribution Checklist

### Linux Release

- [ ] Build with `-DSTATIC_STDLIB=ON`
- [ ] Test on clean Ubuntu/Debian VM
- [ ] Verify no FFmpeg/CEF dependencies: `ldd hls-generator`
- [ ] Test with OBS 28, 29, 30
- [ ] Create tarball: `tar czf hls-generator-linux-x64.tar.gz hls-generator`

### Windows Release

- [ ] Cross-compile from Linux or build on MSYS2
- [ ] Test on Windows 10/11
- [ ] Verify dependencies: `objdump -p hls-generator.exe | grep "DLL Name"`
- [ ] Test with latest OBS Studio
- [ ] Create zip: `zip hls-generator-windows-x64.zip hls-generator.exe`
- [ ] Optional: Sign executable (authenticode)

### Documentation

- [ ] README.md updated
- [ ] Version number in SPECIFICATIONS.md
- [ ] Changelog with changes
- [ ] Build instructions verified

---

## Build Time Estimates

| Configuration | Time (4 cores) | Size |
|--------------|----------------|------|
| Linux dynamic | ~2 minutes | 2.7 MB |
| Linux semi-static | ~2 minutes | 4.1 MB |
| Windows cross-compile | ~3 minutes | 2.7 MB |
| Clean build (all) | ~5 minutes | - |

**Note**: First build compiles libcef_dll_wrapper (188 files) which takes most of the time. Incremental builds are much faster (~10 seconds).

---

## References

- CMake documentation: https://cmake.org/documentation/
- MinGW-w64: https://www.mingw-w64.org/
- FFmpeg: https://ffmpeg.org/
- OBS Studio: https://obsproject.com/

---

**Last Updated**: 2024-10-19

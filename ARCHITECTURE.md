# Architecture v1.4.0

This document describes the modular architecture introduced in v1.4.0.

## Overview

HLS Generator uses a **modular pipeline architecture** where FFmpeg operations are separated into specialized components with clear responsibilities. All components share a single `FFmpegContext` for library loading and resource management uses RAII with custom deleters.

## Architecture Diagram

```
HLSGenerator (Public API)
    └── FFmpegWrapper (Orchestrator)
            ├── FFmpegContext (shared_ptr - Single FFmpeg loader)
            │
            ├── VideoPipeline
            │   ├── Mode Detection (REMUX/TRANSCODE/PROGRAMMATIC)
            │   ├── Decoder (AVCodecContext with custom deleter)
            │   ├── Encoder (AVCodecContext with custom deleter)
            │   ├── Bitstream Filter (AVBSFContext with custom deleter)
            │   ├── Scaler (SwsContext with custom deleter)
            │   └── Frame Processing
            │
            ├── AudioPipeline
            │   ├── Mode Detection (REMUX/TRANSCODE)
            │   ├── Decoder (AVCodecContext with custom deleter)
            │   ├── Encoder (AVCodecContext with custom deleter)
            │   ├── Resampler (SwrContext with custom deleter)
            │   └── PTS Tracking
            │
            └── StreamInput (Factory creates)
                    ├── BrowserInput (uses FFmpegContext)
                    └── FFmpegInput (uses FFmpegContext)
```

## Core Components

### 1. FFmpegContext

**Purpose**: Single point of FFmpeg library loading and function pointer management.

**Responsibilities**:
- Load FFmpeg libraries dynamically (avformat, avcodec, avutil, swscale, swresample)
- Expose ~50 FFmpeg function pointers
- Shared via `std::shared_ptr` across all components
- Thread-safe initialization

**Location**: `src/ffmpeg_context.{h,cpp}`

**Key Methods**:
- `initialize(libPath)` - Loads all FFmpeg libraries
- Public function pointers: `avformat_open_input`, `avcodec_alloc_context3`, etc.

**Design Pattern**: Shared ownership via `std::shared_ptr<FFmpegContext>`

---

### 2. VideoPipeline

**Purpose**: Video processing pipeline for all three modes.

**Responsibilities**:
- Mode detection (REMUX for H.264, TRANSCODE for others, PROGRAMMATIC for browser)
- Decoder setup and frame decoding
- Encoder setup (H.264 with libx264)
- Bitstream filtering (h264_mp4toannexb for HLS)
- Scaling/format conversion (SwsContext)
- Frame encoding

**Location**: `src/video_pipeline.{h,cpp}`

**Processing Modes**:
```cpp
enum class Mode {
    REMUX,        // H.264 → copy packets (fast, no quality loss)
    TRANSCODE,    // Non-H.264 → decode → encode to H.264
    PROGRAMMATIC  // Browser source → encode frames to H.264
};
```

**Key Methods**:
- `detectMode(AVStream*)` - Determines processing mode
- `setupDecoder(AVStream*)` - Initializes video decoder
- `setupEncoder(AVStream*, AppConfig)` - Initializes H.264 encoder
- `setupBitstreamFilter(...)` - Sets up h264_mp4toannexb filter
- `convertAndEncodeFrame(AVFrame*, pts)` - Processes a single frame
- `processBitstreamFilter(AVPacket*)` - Filters encoded packets
- `flush*()` - Drains encoder/filter buffers

**RAII Resources** (all use custom deleters):
- `std::unique_ptr<AVCodecContext, AVCodecContextDeleter> inputCodecCtx_`
- `std::unique_ptr<AVCodecContext, AVCodecContextDeleter> outputCodecCtx_`
- `std::unique_ptr<AVBSFContext, AVBSFContextDeleter> bsfCtx_`
- `std::unique_ptr<SwsContext, SwsContextDeleter> swsCtx_`

---

### 3. AudioPipeline

**Purpose**: Audio processing pipeline with PTS tracking.

**Responsibilities**:
- Mode detection (REMUX for AAC, TRANSCODE for others)
- Decoder setup (for non-AAC audio)
- Encoder setup (AAC encoding)
- Resampling (SwrContext for format conversion)
- PTS tracking and synthetic timestamp generation
- Audio packet processing

**Location**: `src/audio_pipeline.{h,cpp}`

**Processing Logic**:
```cpp
if (codec == AAC) {
    // REMUX: Copy packets directly (fast, no transcoding)
} else {
    // TRANSCODE: Decode → Resample → Encode to AAC
}
```

**Key Methods**:
- `setupEncoder(...)` - Detects mode and sets up decoder/encoder/resampler
- `processPacket(AVPacket*, ...)` - Processes one audio packet
- `flush(...)` - Drains encoder buffers
- `reset()` - Resets state for page reloads

**RAII Resources** (all use custom deleters):
- `std::unique_ptr<AVCodecContext, AVCodecContextDeleter> inputCodecCtx_`
- `std::unique_ptr<AVCodecContext, AVCodecContextDeleter> outputCodecCtx_`
- `std::unique_ptr<SwrContext, SwrContextDeleter> swrCtx_`
- `std::unique_ptr<AVFrame, AVFrameDeleter> convertedFrame_`

**PTS State** (encapsulated):
```cpp
struct PTSState {
    int64_t lastPts = 0;
    bool initialized = false;
    bool warningShown = false;
    void reset();
};
```

---

### 4. Custom Deleters

**Purpose**: RAII cleanup of FFmpeg resources using the correct FFmpegContext.

**Responsibilities**:
- Store `std::shared_ptr<FFmpegContext>` to access cleanup functions
- Call appropriate FFmpeg free functions on destruction
- Enable automatic resource management with `std::unique_ptr`

**Location**: `src/ffmpeg_deleters.{h,cpp}`

**Deleters Implemented**:
```cpp
struct AVFormatContextDeleter {
    std::shared_ptr<FFmpegContext> ffmpeg;
    void operator()(AVFormatContext* ctx) const;
};

struct AVCodecContextDeleter { /* ... */ };
struct AVFrameDeleter { /* ... */ };
struct SwsContextDeleter { /* ... */ };
struct AVBSFContextDeleter { /* ... */ };
struct SwrContextDeleter { /* ... */ };
```

**Usage Pattern**:
```cpp
// Create with deleter that has FFmpegContext
inputCodecCtx_ = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>(
    ffmpeg_->avcodec_alloc_context3(decoder),
    AVCodecContextDeleter(ffmpeg_)  // Deleter knows how to free
);

// Automatic cleanup when out of scope or reset
inputCodecCtx_.reset();  // Calls ffmpeg_->avcodec_free_context()
```

---

### 5. StreamInput Hierarchy

**Purpose**: Abstract input sources (files, browser, streams).

**Hierarchy**:
```
StreamInput (interface)
    ├── BrowserInput (CEF browser source)
    └── FFmpegInput (files/RTMP/SRT/etc.)
```

**Factory Pattern**:
```cpp
std::unique_ptr<StreamInput> StreamInputFactory::create(
    const std::string& uri,
    const AppConfig& config,
    std::shared_ptr<FFmpegContext> ffmpegCtx  // v1.4.0: Receives shared context
);
```

**Migration in v1.4.0**:
- `BrowserInput` and `FFmpegInput` now receive `std::shared_ptr<FFmpegContext>`
- Both use `ffmpeg_->` instead of `FFmpegLib::` for all FFmpeg calls
- Custom deleters with context for all smart pointers

---

## Resource Lifetimes

### Shared Resources
```cpp
// FFmpegContext is shared (reference counted)
std::shared_ptr<FFmpegContext> ffmpegCtx_;  // In FFmpegWrapper

// Passed to all components
videoPipeline_ = std::make_unique<VideoPipeline>(ffmpegCtx_);
audioPipeline_ = std::make_unique<AudioPipeline>(ffmpegCtx_);
streamInput_ = StreamInputFactory::create(uri, config, ffmpegCtx_);
```

### Owned Resources
```cpp
// Each pipeline owns its resources
std::unique_ptr<AVCodecContext, AVCodecContextDeleter> inputCodecCtx_;

// Automatic cleanup on:
// 1. Pipeline destruction
// 2. Explicit reset: inputCodecCtx_.reset()
// 3. Reassignment: inputCodecCtx_ = std::make_unique<...>()
```

---

## Configuration Management

### Immutable Configuration (v1.4.0)

```cpp
class FFmpegWrapper {
public:
    explicit FFmpegWrapper(const AppConfig& config);  // Config required at construction

private:
    const AppConfig config_;  // Immutable - cannot change after construction
};
```

**Benefits**:
- Thread-safe by design (const = no mutations)
- Single source of truth
- Clear lifetime (set once, never changes)
- Prevents configuration bugs

**Before v1.4.0**:
```cpp
FFmpegWrapper wrapper;
wrapper.setConfig(config);  // ❌ Mutable, can be called multiple times
```

**After v1.4.0**:
```cpp
FFmpegWrapper wrapper(config);  // ✅ Immutable, set once at construction
```

---

## Memory Safety Guarantees

### Zero Memory Leaks
All FFmpeg resources are wrapped in `std::unique_ptr` with custom deleters:

| Resource | Type | Deleter | Cleanup Function |
|----------|------|---------|------------------|
| Codec Context | `AVCodecContext*` | `AVCodecContextDeleter` | `avcodec_free_context()` |
| Frame | `AVFrame*` | `AVFrameDeleter` | `av_frame_free()` |
| Scaler | `SwsContext*` | `SwsContextDeleter` | `sws_freeContext()` |
| Resampler | `SwrContext*` | `SwrContextDeleter` | `swr_free()` |
| Bitstream Filter | `AVBSFContext*` | `AVBSFContextDeleter` | `av_bsf_free()` |
| Format Context | `AVFormatContext*` | `AVFormatContextDeleter` | `avformat_free_context()` |

### Automatic Cleanup
```cpp
// Resources freed automatically when:
1. Object goes out of scope
2. Object is destroyed
3. reset() is called
4. Object is reassigned

// NO manual cleanup needed:
// - No av_free() calls
// - No memory leak possibilities
// - No forgetting to clean up in error paths
```

---

## Thread Safety

### Read-Only Operations (Thread-Safe)
- `FFmpegContext` function pointers (initialized once, read-only thereafter)
- `const AppConfig config_` (immutable after construction)

### Not Thread-Safe (By Design)
- FFmpeg operations (codec, format contexts) - Not designed for concurrent access
- Pipeline state (PTS tracking, frame counters) - Single-threaded processing
- HLSGenerator uses one FFmpegWrapper per instance (no shared state)

**Design**: Single-threaded processing model. Each `HLSGenerator` instance owns its resources exclusively.

---

## Design Patterns Used

### 1. Shared Ownership
```cpp
std::shared_ptr<FFmpegContext> ffmpegCtx_;  // Shared across components
```

### 2. Factory Pattern
```cpp
StreamInputFactory::create(uri, config, ffmpegCtx);
```

### 3. RAII (Resource Acquisition Is Initialization)
```cpp
std::unique_ptr<T, CustomDeleter> resource_;  // Automatic cleanup
```

### 4. Strategy Pattern
```cpp
enum class Mode { REMUX, TRANSCODE, PROGRAMMATIC };  // Different processing strategies
```

### 5. Composition Over Inheritance
```cpp
class FFmpegWrapper {
    std::unique_ptr<VideoPipeline> videoPipeline_;  // Has-a relationship
    std::unique_ptr<AudioPipeline> audioPipeline_;
};
```

---

## Extensibility

### Adding New Input Sources
1. Inherit from `StreamInput`
2. Accept `std::shared_ptr<FFmpegContext>` in constructor
3. Use `ffmpeg_->` for all FFmpeg calls
4. Add to `StreamInputFactory::create()`

### Adding New Processing Modes
1. Add enum value to `VideoPipeline::Mode` or `AudioPipeline`
2. Extend `detectMode()` logic
3. Implement processing in `processPacket()` or equivalent

### Adding New Pipelines
1. Create class with `std::shared_ptr<FFmpegContext>` member
2. Use custom deleters for all FFmpeg resources
3. Add to `FFmpegWrapper` as `std::unique_ptr<YourPipeline>`

---

## Migration from v1.3.0

### Code Changes Required

**1. FFmpegWrapper Construction**:
```cpp
// Before v1.3.0:
FFmpegWrapper wrapper;
wrapper.setConfig(config);

// After v1.4.0:
FFmpegWrapper wrapper(config);
```

**2. setupOutput() Call**:
```cpp
// Before v1.3.0:
wrapper.setupOutput(config);

// After v1.4.0:
wrapper.setupOutput();  // No parameter needed
```

**3. Component Migration** (if extending):
- Replace `FFmpegLib::function()` with `ffmpeg_->function()`
- Add `std::shared_ptr<FFmpegContext>` member
- Use custom deleters for all FFmpeg resources

---

## Performance Characteristics

### Memory Usage
- **FFmpeg Context**: Loaded once, ~5 MB (shared)
- **Pipelines**: ~500 bytes each (lightweight)
- **Codec Contexts**: ~10 KB each (created as needed)
- **Frames/Packets**: Allocated/freed per-frame (~1-10 MB working set)

### Initialization Time
- **Library Loading**: ~50ms (one-time, shared)
- **Pipeline Creation**: <1ms (lightweight)
- **Decoder Setup**: ~10ms (per stream)
- **Encoder Setup**: ~20ms (per stream)

### Runtime Performance
- **REMUX mode**: Near-zero CPU (packet copying)
- **TRANSCODE mode**: CPU-bound (decoding/encoding)
- **PROGRAMMATIC mode**: CPU-bound (encoding only)
- **Memory overhead**: Negligible vs. FFmpeg operations

---

## Testing

### Unit Testing Recommendations
- Mock `FFmpegContext` for pipeline testing
- Test each pipeline independently
- Verify deleter cleanup (valgrind, ASan)
- Test configuration immutability

### Integration Testing
- Test all three video modes (REMUX/TRANSCODE/PROGRAMMATIC)
- Test audio REMUX and TRANSCODE
- Test page reloads (encoder reset)
- Test resource cleanup (no leaks)

---

## References

- [CHANGELOG.md](CHANGELOG.md) - Version history
- [DEVELOPMENT-JOURNEY.md](docs/DEVELOPMENT-JOURNEY.md) - Development story
- [TECHNICAL-REFERENCE.md](docs/TECHNICAL-REFERENCE.md) - FFmpeg/CEF details
- Source code: `src/ffmpeg_context.{h,cpp}`, `src/*_pipeline.{h,cpp}`, `src/ffmpeg_deleters.{h,cpp}`

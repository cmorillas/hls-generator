# Changelog

All notable changes to the HLS Generator project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2025-01-23

### Added
- **Modular Pipeline Architecture**: Separated FFmpeg processing into specialized components
  - `FFmpegContext`: Centralized FFmpeg library loading with shared context
  - `VideoPipeline`: Video processing pipeline (REMUX/TRANSCODE/PROGRAMMATIC modes)
  - `AudioPipeline`: Audio processing pipeline with PTS tracking
  - Custom deleters with FFmpeg context for automatic resource management
- **RAII Memory Management**: All FFmpeg resources now use smart pointers with custom deleters
  - Zero memory leaks - all resources freed automatically
  - Type-aware deleters (AVCodecContext, AVFrame, SwsContext, etc.)
- **Immutable Configuration**: `AppConfig` now passed via constructor as const member
  - Thread-safe by design
  - Prevents configuration bugs from runtime mutations

### Changed
- **Breaking**: `FFmpegWrapper` constructor now requires `AppConfig` parameter
  - Old: `FFmpegWrapper wrapper; wrapper.setConfig(config);`
  - New: `FFmpegWrapper wrapper(config);`
- **Breaking**: `FFmpegWrapper::setupOutput()` no longer takes config parameter
- Migrated `BrowserInput` and `FFmpegInput` to use `FFmpegContext` (28 and 4 changes respectively)
- `StreamInputFactory::create()` now accepts `std::shared_ptr<FFmpegContext>`

### Removed
- **Eliminated dual FFmpeg loading**: Single initialization point via FFmpegContext
- Removed `FFmpegWrapper::setConfig()` method (configuration now immutable)
- Removed `src/ffmpeg_loader.{h,cpp}` - replaced by FFmpegContext
- Removed `src/muxer_manager.{h,cpp}` - dead code that was never integrated
- Removed `FFmpegLib::` namespace - all code uses FFmpegContext

### Fixed
- **Memory Leaks** (9 total fixed):
  - VideoPipeline: inputCodecCtx, outputCodecCtx, bsfCtx, swsCtx, scaledFrame
  - AudioPipeline: inputCodecCtx, outputCodecCtx, swrCtx, convertedFrame
- Crash bug from uninitialized FFmpeg function pointers in legacy components
- Resource cleanup in error paths now automatic via RAII

### Performance
- Reduced memory usage: Single FFmpeg library load instead of dual loading
- Faster startup: Libraries loaded once and shared across all components
- Better cache locality: Shared context pattern

### Code Quality
- Reduced codebase by ~500 lines (duplicated/legacy code removed)
- Improved encapsulation: Clear separation of concerns with pipelines
- Enhanced thread safety: Immutable configuration, const-correct design
- Better maintainability: Modular architecture with single responsibilities

## [1.3.0] - 2025-01-19

### Added
- Initial release with basic HLS generation
- Support for multiple input types (file, browser, streaming protocols)
- Video transcoding with H.264
- Audio transcoding to AAC
- CEF browser integration for web content capture

---

**Full Changelog**: https://github.com/cmorillas/hls-generator/compare/v1.3.0...v1.4.0

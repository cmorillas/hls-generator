@echo off
REM
REM Download FFmpeg Headers for Linux
REM Execute on: Windows
REM Target: Linux cross-compilation (from Windows to Linux - rare case)
REM
REM This script downloads and extracts ONLY header files (.h) from FFmpeg source
REM for cross-compiling to Linux from Windows.
REM
REM Requirements: tar, curl or wget available in PATH
REM

setlocal enabledelayedexpansion

set FFMPEG_VERSION=7.0.2
set FFMPEG_URL=https://ffmpeg.org/releases/ffmpeg-%FFMPEG_VERSION%.tar.xz
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..\..
set OUTPUT_DIR=%PROJECT_ROOT%\external\ffmpeg\linux
set TEMP_DIR=%TEMP%\ffmpeg-headers-download-%RANDOM%

echo ==================================================
echo FFmpeg Headers Downloader
echo ==================================================
echo Version:     FFmpeg %FFMPEG_VERSION%
echo Platform:    Windows (running on Windows)
echo Target:      Linux cross-compilation
echo Output:      %OUTPUT_DIR%
echo ==================================================
echo.

REM Create temp directory
if not exist "%TEMP_DIR%" mkdir "%TEMP_DIR%"
cd /d "%TEMP_DIR%"

REM Download FFmpeg source
echo [1/4] Downloading FFmpeg source tarball...
where curl >nul 2>&1
if %errorlevel% equ 0 (
    curl -# -L "%FFMPEG_URL%" -o "ffmpeg-%FFMPEG_VERSION%.tar.xz"
) else (
    where wget >nul 2>&1
    if %errorlevel% equ 0 (
        wget -q --show-progress "%FFMPEG_URL%" -O "ffmpeg-%FFMPEG_VERSION%.tar.xz"
    ) else (
        echo Error: Neither curl nor wget found. Please install one of them.
        exit /b 1
    )
)

REM Extract tarball (requires tar in Windows 10+ or from Git Bash/MSYS2)
echo [2/4] Extracting tarball...
tar -xf "ffmpeg-%FFMPEG_VERSION%.tar.xz"
if %errorlevel% neq 0 (
    echo Error: tar command failed. Make sure you have tar installed.
    echo On Windows 10+, tar is built-in. On older Windows, install Git Bash or 7-Zip.
    exit /b 1
)

REM Extract only headers
echo [3/4] Extracting headers only (.h files^)...
if not exist "headers-only" mkdir "headers-only"
cd "ffmpeg-%FFMPEG_VERSION%"

for %%L in (libavcodec libavdevice libavfilter libavformat libavutil libswscale libswresample) do (
    if exist "%%L\" (
        echo   - Copying %%L headers...
        xcopy "%%L\*.h" "..\headers-only\%%L\" /S /I /Q >nul 2>&1
    )
)

REM Move to output directory
echo [4/4] Installing headers to %OUTPUT_DIR%...
if exist "%OUTPUT_DIR%" rmdir /s /q "%OUTPUT_DIR%"
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
xcopy "..\headers-only\*" "%OUTPUT_DIR%\" /S /E /I /Q >nul 2>&1

REM Cleanup
cd /d "%PROJECT_ROOT%"
rmdir /s /q "%TEMP_DIR%" >nul 2>&1

REM Summary
echo.
echo ==================================================
echo Installation complete!
echo ==================================================
echo Location: %OUTPUT_DIR%
echo.
echo You can now cross-compile for Linux from Windows.
echo ==================================================

endlocal

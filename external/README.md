# External Dependencies

Este directorio contiene dependencias externas necesarias para compilar el proyecto. **No contiene librerías binarias**, solo headers y código auxiliar mínimo.

## Estructura

```
external/
├── cef/              (commiteado al repo - 3.6 MB)
│   ├── include/      Headers CEF C API
│   └── wrapper/      Código wrapper C++ mínimo
└── ffmpeg/           (NO commiteado - descargado localmente)
    ├── linux/        Headers FFmpeg para Linux (~25 MB)
    └── windows/      Headers FFmpeg para Windows (~25 MB)
```

## CEF (Chromium Embedded Framework)

- **`cef/include/`** - Headers de CEF C API (~3.5 MB)
  - Necesarios para conocer las estructuras y funciones de CEF durante la compilación
  - En runtime, el programa carga las librerías de OBS Studio dinámicamente

- **`cef/wrapper/base/`** - Código base del wrapper CEF (~52 KB)
  - Implementaciones básicas de utilidades CEF (locks, callbacks, ref counting)
  - Se compila en `libcef_dll_wrapper.a` durante el build
  - Facilita el uso de la C API de CEF desde C++

**Incluido en el repositorio**: Los headers y wrapper de CEF están commiteados porque son específicos de una versión y difíciles de obtener.

## FFmpeg Headers

**NO están commiteados** en el repositorio. Se gestionan de forma flexible:

### Opción A: Paquetes del sistema (idiomático)

- **Linux**: `sudo apt install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev`
- **Windows (MSYS2)**: `pacman -S mingw-w64-x86_64-ffmpeg`
- **Windows (vcpkg)**: `vcpkg install ffmpeg:x64-windows`

### Opción B: Headers descargados en `external/ffmpeg/` (si ya tienes OBS instalado)

- **Linux**: `external/ffmpeg/linux/` (headers .h descargados localmente)
- **Windows**: `external/ffmpeg/windows/` (headers .h descargados localmente)

**Ventaja**: No necesitas instalar paquetes de desarrollo completos si ya tienes OBS Studio instalado. Solo descargas los headers (~25 MB por plataforma) para compilar.

**Scripts disponibles**:
- Linux: `./scripts/linux/download-ffmpeg-headers-linux.sh` o `./scripts/linux/download-ffmpeg-headers-windows.sh`
- Windows: `scripts\windows\download-ffmpeg-headers-windows.bat` o `scripts\windows\download-ffmpeg-headers-linux.bat`

**Comportamiento del build**:
1. CMake intenta primero los headers del sistema (Opción A)
2. Si no están disponibles, busca en `external/ffmpeg/{platform}/` (Opción B)
3. Esto permite flexibilidad sin duplicar espacio

## ¿Por qué solo headers?

El proyecto usa **dynamic loading** (carga dinámica):

- **En compilación**: Necesita headers (.h) para conocer las estructuras de datos
- **En ejecución**: Carga las DLLs/SOs desde la instalación de OBS Studio

Esto mantiene los binarios pequeños (~400 KB vs ~800 MB si incluyera CEF/FFmpeg completos).

## Licencias

- CEF: BSD License (ver `cef/include/cef_api_hash.h` para detalles)
- El código wrapper se distribuye bajo las mismas condiciones que el proyecto principal

# Technical Reference - Dynamic Loading Architecture

Guía técnica completa de la arquitectura de dynamic loading de **FFmpeg y CEF** usada en HLS Generator.

**Propósito**: Referencia técnica para entender la implementación, aplicar el mismo patrón a otros proyectos, o modificar el código existente.

---

## Tabla de Contenidos

1. [Arquitectura General](#arquitectura-general)
2. [FFmpeg Dynamic Loading](#ffmpeg-dynamic-loading)
3. [CEF Dynamic Loading](#cef-dynamic-loading)
4. [OBS Detection System](#obs-detection-system)
5. [Cross-Platform Abstractions](#cross-platform-abstractions)
6. [Build System Configuration](#build-system-configuration)
7. [Code Reference](#code-reference)

---

## Arquitectura General

### Concepto: Runtime Dynamic Loading

En lugar de linkear estáticamente las bibliotecas (que requiere descargar SDKs gigantes), cargamos las funciones dinámicamente en runtime usando:

- **Linux**: `dlopen()` + `dlsym()`
- **Windows**: `LoadLibrary()` + `GetProcAddress()`

### Ventajas

| Aspecto | Static Linking | Dynamic Loading (nuestra solución) |
|---------|----------------|-----------------------------------|
| **SDK Download** | 1 GB (FFmpeg + CEF) | 0 MB (usa OBS) |
| **Binary Size** | 200+ MB | 2.7 MB |
| **Dependencies** | libcef.so, libav*.so | Solo libc |
| **Updates** | Recompilar | Automático con OBS |
| **Distribution** | Bundlear libs | Solo ejecutable |

### Flujo de Ejecución

```
1. main() inicia
   ↓
2. Detectar OBS Studio instalado
   ↓
3. Cargar FFmpeg dinámicamente desde OBS
   ↓
4. Cargar CEF dinámicamente desde OBS
   ↓
5. Usar funciones como si estuvieran linkeadas estáticamente
   ↓
6. Generar HLS
```

---

## FFmpeg Dynamic Loading

### Arquitectura

FFmpeg se carga en 3 pasos:

1. **Detección**: Encontrar bibliotecas de OBS
2. **Loading**: Cargar .so/.dll con dlopen/LoadLibrary
3. **Symbol Resolution**: Resolver símbolos de funciones con dlsym/GetProcAddress

### Implementación

#### Archivo: `src/ffmpeg_loader.cpp`

**1. Declarar Function Pointers**

```cpp
// Typedefs para type-safety
typedef AVFormatContext* (*avformat_alloc_context_t)(void);
typedef int (*avformat_open_input_t)(AVFormatContext**, const char*, AVInputFormat*, AVDictionary**);
typedef int (*avformat_find_stream_info_t)(AVFormatContext*, AVDictionary**);
// ... 45 funciones más

// Punteros globales (inicializados a nullptr)
static avformat_alloc_context_t avformat_alloc_context_ptr = nullptr;
static avformat_open_input_t avformat_open_input_ptr = nullptr;
static avformat_find_stream_info_t avformat_find_stream_info_ptr = nullptr;
// ... 45 punteros más
```

**2. Cargar Bibliotecas**

```cpp
bool FFmpegLoader::loadFFmpeg(const std::string& libDir) {
    // Linux
    #ifndef _WIN32
    void* avformat_lib = dlopen((libDir + "/libavformat.so.61").c_str(), RTLD_LAZY);
    void* avcodec_lib = dlopen((libDir + "/libavcodec.so.61").c_str(), RTLD_LAZY);
    void* avutil_lib = dlopen((libDir + "/libavutil.so.59").c_str(), RTLD_LAZY);
    void* swscale_lib = dlopen((libDir + "/libswscale.so.8").c_str(), RTLD_LAZY);

    // Windows
    #else
    HMODULE avformat_lib = LoadLibraryA((libDir + "\\avformat-61.dll").c_str());
    HMODULE avcodec_lib = LoadLibraryA((libDir + "\\avcodec-61.dll").c_str());
    HMODULE avutil_lib = LoadLibraryA((libDir + "\\avutil-59.dll").c_str());
    HMODULE swscale_lib = LoadLibraryA((libDir + "\\swscale-8.dll").c_str());
    #endif

    if (!avformat_lib || !avcodec_lib || !avutil_lib || !swscale_lib) {
        return false;
    }

    // Continuar con symbol resolution...
}
```

**3. Resolver Símbolos**

```cpp
// Macro helper para simplificar
#define LOAD_FUNC(lib, func) \
    func##_ptr = (func##_t)GET_PROC(lib, #func); \
    if (!func##_ptr) { \
        Logger::error("Failed to load: " #func); \
        return false; \
    }

// Cargar todas las funciones
LOAD_FUNC(avformat_lib, avformat_alloc_context);
LOAD_FUNC(avformat_lib, avformat_open_input);
LOAD_FUNC(avformat_lib, avformat_find_stream_info);
LOAD_FUNC(avformat_lib, avformat_close_input);
// ... 41 funciones más
```

**4. Crear Wrappers Públicos**

```cpp
// En ffmpeg_wrapper.cpp - funciones públicas que usan los punteros
extern "C" {

AVFormatContext* avformat_alloc_context(void) {
    return FFmpegLoader::avformat_alloc_context_ptr();
}

int avformat_open_input(AVFormatContext** ps, const char* url,
                        AVInputFormat* fmt, AVDictionary** options) {
    return FFmpegLoader::avformat_open_input_ptr(ps, url, fmt, options);
}

// ... 43 wrappers más

} // extern "C"
```

### Funciones FFmpeg Cargadas (45 total)

**avformat (demuxing/muxing)**:
- `avformat_alloc_context`, `avformat_free_context`
- `avformat_open_input`, `avformat_close_input`
- `avformat_find_stream_info`
- `avformat_alloc_output_context2`
- `avformat_new_stream`
- `avformat_write_header`, `av_write_frame`, `av_write_trailer`
- `av_read_frame`
- `avio_open`, `avio_close`

**avcodec (encoding/decoding)**:
- `avcodec_find_encoder`, `avcodec_find_decoder`
- `avcodec_alloc_context3`, `avcodec_free_context`
- `avcodec_parameters_to_context`, `avcodec_parameters_from_context`
- `avcodec_open2`
- `avcodec_send_frame`, `avcodec_receive_packet`
- `avcodec_send_packet`, `avcodec_receive_frame`

**avutil (utilities)**:
- `av_frame_alloc`, `av_frame_free`
- `av_packet_alloc`, `av_packet_free`, `av_packet_unref`
- `av_malloc`, `av_free`, `av_freep`
- `av_image_get_buffer_size`, `av_image_fill_arrays`
- `av_opt_set`, `av_dict_set`, `av_dict_free`
- `av_rescale_q`, `av_rescale_q_rnd`
- `av_get_default_channel_layout`

**swscale (pixel format conversion)**:
- `sws_getContext`, `sws_freeContext`
- `sws_scale`

---

## CEF Dynamic Loading

### Arquitectura

CEF es más complejo que FFmpeg porque:
1. Tiene **175 funciones** C API
2. Requiere el **libcef_dll_wrapper** C++ (188 archivos)
3. El wrapper C++ llama internamente al C API

**Solución en 4 capas**:

```
Capa 4: Código de aplicación (cef_backend.cpp)
           ↓ usa
Capa 3: libcef_dll_wrapper (C++ estático compilado)
           ↓ llama
Capa 2: Function redirects (#define macros)
           ↓ redirige a
Capa 1: Dynamic loader (cef_loader.cpp con 175 punteros)
           ↓ carga dinámicamente
Capa 0: libcef.so de OBS (runtime)
```

### Implementación

#### Archivo: `src/cef_loader.cpp`

**1. Declarar 175 Function Pointers**

```cpp
// Typedefs
typedef int (*cef_initialize_t)(
    const struct _cef_main_args_t* args,
    const struct _cef_settings_t* settings,
    cef_app_t* application,
    void* windows_sandbox_info
);

typedef void (*cef_shutdown_t)(void);

typedef void (*cef_do_message_loop_work_t)(void);

typedef int (*cef_browser_host_create_browser_t)(
    const cef_window_info_t* windowInfo,
    struct _cef_client_t* client,
    const cef_string_t* url,
    const cef_browser_settings_t* settings,
    struct _cef_dictionary_value_t* extra_info,
    struct _cef_request_context_t* request_context
);

// ... 171 typedefs más

// Punteros globales
namespace CEFLoader {
    static cef_initialize_t cef_initialize_ptr = nullptr;
    static cef_shutdown_t cef_shutdown_ptr = nullptr;
    static cef_do_message_loop_work_t cef_do_message_loop_work_ptr = nullptr;
    static cef_browser_host_create_browser_t cef_browser_host_create_browser_ptr = nullptr;
    // ... 171 punteros más
}
```

**2. Cargar libcef.so**

```cpp
bool CEFLoader::loadCEF(const std::string& cefPath) {
    // Linux
    #ifndef _WIN32
    std::string libPath = cefPath + "/libcef.so";
    cef_lib = dlopen(libPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);

    // Windows
    #else
    std::string libPath = cefPath + "\\libcef.dll";
    cef_lib = LoadLibraryA(libPath.c_str());
    #endif

    if (!cef_lib) {
        Logger::error("Failed to load CEF library: " + libPath);
        return false;
    }

    // Cargar 175 funciones...
    loadAllSymbols();

    return true;
}
```

**3. Cargar 175 Símbolos**

```cpp
bool CEFLoader::loadAllSymbols() {
    #define LOAD_CEF_FUNC(name) \
        name##_ptr = (name##_t)GET_PROC(cef_lib, #name); \
        if (!name##_ptr) { \
            Logger::error("Failed to load CEF function: " #name); \
            return false; \
        }

    // Core functions
    LOAD_CEF_FUNC(cef_initialize);
    LOAD_CEF_FUNC(cef_shutdown);
    LOAD_CEF_FUNC(cef_execute_process);
    LOAD_CEF_FUNC(cef_do_message_loop_work);
    LOAD_CEF_FUNC(cef_quit_message_loop);

    // Browser functions
    LOAD_CEF_FUNC(cef_browser_host_create_browser);
    LOAD_CEF_FUNC(cef_browser_host_create_browser_sync);

    // String functions
    LOAD_CEF_FUNC(cef_string_utf8_to_utf16);
    LOAD_CEF_FUNC(cef_string_utf16_to_utf8);
    LOAD_CEF_FUNC(cef_string_userfree_utf16_free);

    // ... 165 LOAD_CEF_FUNC más

    #undef LOAD_CEF_FUNC
    return true;
}
```

**4. Function Redirects (la clave del sistema)**

#### Archivo: `include/cef_function_redirects.h`

El wrapper C++ llama directamente a funciones CEF. Para que use nuestros punteros dinámicos, redirigimos con macros:

```cpp
#ifndef CEF_FUNCTION_REDIRECTS_H
#define CEF_FUNCTION_REDIRECTS_H

#include "cef_loader.h"

// Redirigir todas las llamadas del wrapper a nuestros punteros
#define cef_initialize CEFLoader::cef_initialize_ptr
#define cef_shutdown CEFLoader::cef_shutdown_ptr
#define cef_execute_process CEFLoader::cef_execute_process_ptr
#define cef_do_message_loop_work CEFLoader::cef_do_message_loop_work_ptr
#define cef_browser_host_create_browser CEFLoader::cef_browser_host_create_browser_ptr

// ... 170 #define más

#endif
```

**Efecto**: Cuando el wrapper hace:
```cpp
cef_initialize(&args, &settings, nullptr, nullptr);
```

Se convierte en:
```cpp
CEFLoader::cef_initialize_ptr(&args, &settings, nullptr, nullptr);
```

**5. Stubs para Símbolos del Wrapper**

Algunos archivos del wrapper (`libcef_dll.cc`, `libcef_dll2.cc`) tienen dependencias complejas. Los excluimos y creamos stubs:

#### Archivo: `src/cef_stubs.cpp`

```cpp
extern "C" {

int cef_initialize(const struct _cef_main_args_t* args,
                   const struct _cef_settings_t* settings,
                   cef_app_t* application,
                   void* windows_sandbox_info) {
    return CEFLoader::cef_initialize_ptr(args, settings, application, windows_sandbox_info);
}

void cef_shutdown(void) {
    CEFLoader::cef_shutdown_ptr();
}

int cef_execute_process(const struct _cef_main_args_t* args,
                        cef_app_t* application,
                        void* windows_sandbox_info) {
    return CEFLoader::cef_execute_process_ptr(args, application, windows_sandbox_info);
}

const char* cef_api_hash(int entry) {
    if (CEFLoader::cef_api_hash_ptr) {
        return CEFLoader::cef_api_hash_ptr(entry);
    }
    return "unknown";
}

int cef_version_info(int entry) {
    if (CEFLoader::cef_version_info_ptr) {
        return CEFLoader::cef_version_info_ptr(entry);
    }
    return 0;
}

} // extern "C"
```

### Lista Completa de Funciones CEF (175 total)

**Core (5 funciones)**:
- `cef_initialize`, `cef_shutdown`, `cef_execute_process`
- `cef_do_message_loop_work`, `cef_quit_message_loop`

**Versioning (2)**:
- `cef_api_hash`, `cef_version_info`

**Browser (15)**:
- `cef_browser_host_create_browser`, `cef_browser_host_create_browser_sync`
- Múltiples funciones de control de browser

**String Handling (10)**:
- `cef_string_utf8_to_utf16`, `cef_string_utf16_to_utf8`
- `cef_string_userfree_*` variants
- `cef_string_list_*`, `cef_string_map_*`, `cef_string_multimap_*`

**Handlers (~143 funciones)**:
- App handlers
- Client handlers
- Life span handlers
- Load handlers
- Display handlers
- Render handlers
- Request handlers
- Resource handlers
- Cookie handlers
- Download handlers
- Context menu handlers
- Dialog handlers
- ... y muchos más

---

## OBS Detection System

### Arquitectura Multi-Plataforma

Detectar OBS instalado y sus bibliotecas es crítico. Cada plataforma tiene diferentes ubicaciones.

#### Archivo: `src/obs_detector.cpp`

### Linux Detection

```cpp
OBSPaths OBSDetector::detectLinux() {
    OBSPaths paths;

    // 1. Verificar ejecutable OBS
    std::string obsExe = findExecutable("obs");
    if (obsExe.empty()) {
        return paths; // Not found
    }

    paths.found = true;
    paths.obs_executable = obsExe;

    // 2. Inferir ubicación de bibliotecas
    // Debian/Ubuntu: /usr/lib/x86_64-linux-gnu/
    if (directoryExists("/usr/lib/x86_64-linux-gnu/obs-plugins")) {
        paths.ffmpeg_lib_dir = "/usr/lib/x86_64-linux-gnu";
        paths.cef_path = "/usr/lib/x86_64-linux-gnu/obs-plugins";
    }
    // Fedora/RHEL: /usr/lib64/
    else if (directoryExists("/usr/lib64/obs-plugins")) {
        paths.ffmpeg_lib_dir = "/usr/lib64";
        paths.cef_path = "/usr/lib64/obs-plugins";
    }
    // Arch: /usr/lib/
    else if (directoryExists("/usr/lib/obs-plugins")) {
        paths.ffmpeg_lib_dir = "/usr/lib";
        paths.cef_path = "/usr/lib/obs-plugins";
    }

    // 3. Verificar subprocess helper
    paths.cef_subprocess = paths.cef_path + "/obs-browser-page";

    return paths;
}
```

### Windows Detection

```cpp
OBSPaths OBSDetector::detectWindows() {
    OBSPaths paths;

    // Rutas a verificar
    std::vector<std::string> searchPaths = {
        "C:\\Program Files\\obs-studio",
        "C:\\Program Files (x86)\\obs-studio",
        getenv("ProgramFiles") + std::string("\\obs-studio")
    };

    for (const auto& basePath : searchPaths) {
        std::string obsExe = basePath + "\\bin\\64bit\\obs64.exe";

        if (fileExists(obsExe)) {
            paths.found = true;
            paths.obs_executable = obsExe;
            paths.ffmpeg_lib_dir = basePath + "\\bin\\64bit";
            paths.cef_path = basePath + "\\obs-plugins\\64bit";

            // Verificar que FFmpeg DLLs existen
            if (fileExists(paths.ffmpeg_lib_dir + "\\avcodec-61.dll")) {
                return paths;
            }
        }
    }

    return paths; // Not found
}
```

### Estructura OBSPaths

```cpp
struct OBSPaths {
    bool found = false;
    std::string obs_executable;      // /usr/bin/obs o C:\...\obs64.exe
    std::string ffmpeg_lib_dir;      // /usr/lib/x86_64-linux-gnu
    std::string cef_path;            // /usr/lib/.../obs-plugins
    std::string cef_subprocess;      // .../obs-browser-page
};
```

---

## Cross-Platform Abstractions

### Dynamic Loading Macros

#### Archivo: `include/ffmpeg_loader.h` y `include/cef_loader.h`

```cpp
#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIB(path) LoadLibraryA(path)
    #define GET_PROC(lib, name) GetProcAddress((HMODULE)lib, name)
    #define CLOSE_LIB(lib) FreeLibrary((HMODULE)lib)
    typedef HMODULE LibHandle;
#else
    #include <dlfcn.h>
    #define LOAD_LIB(path) dlopen(path, RTLD_LAZY)
    #define GET_PROC(lib, name) dlsym(lib, name)
    #define CLOSE_LIB(lib) dlclose(lib)
    typedef void* LibHandle;
#endif
```

### Path Handling

```cpp
#ifdef _WIN32
    #define PATH_SEPARATOR "\\"
#else
    #define PATH_SEPARATOR "/"
#endif

std::string joinPath(const std::string& base, const std::string& file) {
    return base + PATH_SEPARATOR + file;
}
```

### Library Naming

```cpp
std::string getLibraryName(const std::string& base, int version) {
    #ifdef _WIN32
        return base + "-" + std::to_string(version) + ".dll";
    #else
        return "lib" + base + ".so." + std::to_string(version);
    #endif
}

// Ejemplo:
// Linux: getLibraryName("avcodec", 61) → "libavcodec.so.61"
// Windows: getLibraryName("avcodec", 61) → "avcodec-61.dll"
```

---

## Build System Configuration

### CMakeLists.txt - Secciones Clave

**1. Incluir Headers de Terceros**

```cmake
# FFmpeg headers (solo para compilación)
include_directories(/usr/include)

# CEF headers (external/cef/include)
set(CEF_ROOT_DIR "${CMAKE_SOURCE_DIR}/third_party/cef")
include_directories(${CEF_ROOT_DIR})

# Project headers
include_directories(${CMAKE_SOURCE_DIR}/include)
```

**2. Compilar libcef_dll_wrapper**

```cmake
# Compilar wrapper C++ de CEF como biblioteca estática
if(EXISTS "${CMAKE_SOURCE_DIR}/external/cef/wrapper")
    file(GLOB_RECURSE WRAPPER_SOURCES
        "${CMAKE_SOURCE_DIR}/external/cef/wrapper/*.cc"
        "${CMAKE_SOURCE_DIR}/external/cef/wrapper/*.cpp"
    )

    # EXCLUIR archivos con dependencias complejas
    list(FILTER WRAPPER_SOURCES EXCLUDE REGEX "libcef_dll\\.cc$")
    list(FILTER WRAPPER_SOURCES EXCLUDE REGEX "libcef_dll2\\.cc$")

    add_library(libcef_dll_wrapper STATIC ${WRAPPER_SOURCES})
    target_include_directories(libcef_dll_wrapper PRIVATE ${CEF_ROOT_DIR})
endif()
```

**3. Compilar Ejecutable Principal**

```cmake
# Source files
set(SOURCES
    src/main.cpp
    src/logger.cpp
    src/obs_detector.cpp
    src/ffmpeg_loader.cpp
    src/ffmpeg_wrapper.cpp
    src/cef_loader.cpp
    src/cef_function_wrappers.cpp
    src/cef_stubs.cpp
    src/cef_backend.cpp
    src/browser_input.cpp
    src/hls_generator.cpp
    # ... más archivos
)

add_executable(hls-generator ${SOURCES})
```

**4. Linkear (SIN libcef ni libav)**

```cmake
target_link_libraries(hls-generator
    pthread          # Threading
    dl               # Dynamic loading (Linux)
    libcef_dll_wrapper  # Wrapper C++ estático
    # NO linkear libcef.so - se carga dinámicamente
    # NO linkear libav*.so - se carga dinámicamente
)
```

**5. Opciones de Compilación**

```cmake
# Semi-static linking (Linux)
option(STATIC_STDLIB "Link libstdc++ and libgcc statically" OFF)
if(STATIC_STDLIB)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
```

### Toolchain Windows (cmake/toolchain-mingw.cmake)

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compilers
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Search modes
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Static linking
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
```

---

## Code Reference

### Estructura de Archivos Clave

```
src/
├── obs_detector.cpp        # Detecta OBS instalado
├── ffmpeg_loader.cpp       # Carga 45 funciones FFmpeg
├── ffmpeg_wrapper.cpp      # Wrappers públicos FFmpeg
├── cef_loader.cpp          # Carga 175 funciones CEF
├── cef_function_wrappers.cpp  # Wrappers extern "C" CEF
├── cef_stubs.cpp           # Stubs para símbolos faltantes
├── cef_backend.cpp         # Implementación browser CEF
└── hls_generator.cpp       # Generador HLS principal

include/
├── obs_detector.h
├── ffmpeg_loader.h
├── ffmpeg_wrapper.h
├── cef_loader.h
├── cef_function_redirects.h   # Macros #define críticas
├── cef_backend.h
└── hls_generator.h
```

### Flujo de Inicialización

```cpp
// 1. Detectar OBS
OBSPaths obs = OBSDetector::detect();
if (!obs.found) {
    error("OBS not found");
    exit(1);
}

// 2. Cargar FFmpeg
if (!FFmpegLoader::loadFFmpeg(obs.ffmpeg_lib_dir)) {
    error("Failed to load FFmpeg");
    exit(1);
}

// 3. Cargar CEF
if (!CEFLoader::loadCEF(obs.cef_path)) {
    error("Failed to load CEF");
    exit(1);
}

// 4. Inicializar CEF
CefMainArgs args(argc, argv);
CefSettings settings;
settings.no_sandbox = true;
CefString(&settings.browser_subprocess_path).FromASCII(obs.cef_subprocess);

if (!cef_initialize(&args, &settings, nullptr, nullptr)) {
    error("CEF initialization failed");
    exit(1);
}

// 5. Usar normalmente
// Las funciones av_* y cef_* ahora funcionan como si estuvieran linkeadas
```

---

## Aplicar este Patrón a Otras Bibliotecas

### Template Genérico

**1. Crear `mylib_loader.h`**:
```cpp
class MyLibLoader {
public:
    static bool loadMyLib(const std::string& libPath);
    static void unloadMyLib();

    // Function pointers (públicos para redirects)
    static mylib_func1_t mylib_func1_ptr;
    static mylib_func2_t mylib_func2_ptr;
    // ...

private:
    static LibHandle lib_handle;
};
```

**2. Implementar `mylib_loader.cpp`**:
```cpp
// Typedefs
typedef int (*mylib_func1_t)(int arg);
typedef void (*mylib_func2_t)(void);

// Punteros globales
LibHandle MyLibLoader::lib_handle = nullptr;
mylib_func1_t MyLibLoader::mylib_func1_ptr = nullptr;
mylib_func2_t MyLibLoader::mylib_func2_ptr = nullptr;

bool MyLibLoader::loadMyLib(const std::string& libPath) {
    lib_handle = LOAD_LIB(libPath.c_str());
    if (!lib_handle) return false;

    LOAD_FUNC(lib_handle, mylib_func1);
    LOAD_FUNC(lib_handle, mylib_func2);

    return true;
}
```

**3. Crear wrappers `mylib_wrapper.cpp`**:
```cpp
extern "C" {
    int mylib_func1(int arg) {
        return MyLibLoader::mylib_func1_ptr(arg);
    }

    void mylib_func2(void) {
        MyLibLoader::mylib_func2_ptr();
    }
}
```

**4. Usar en código**:
```cpp
if (!MyLibLoader::loadMyLib("/path/to/mylib.so")) {
    error("Failed to load");
}

// Usar normalmente
mylib_func1(42);
mylib_func2();
```

---

## Performance y Memory

### Overhead

- **Llamada directa** (static): 1 instrucción (call)
- **Llamada con puntero** (dynamic): 2 instrucciones (load pointer + call)
- **Diferencia**: ~1-2 nanosegundos (despreciable para operaciones multimedia)

### Memory Usage

- **Static linking**: Código duplicado si múltiples apps usan misma lib
- **Dynamic loading**: Una copia en memoria compartida entre apps
- **Nuestro caso**:
  - Wrapper estático: ~2 MB en binario
  - libcef.so dinámico: ~220 MB compartido con OBS

---

## Debugging Tips

### Verificar Funciones Cargadas

```cpp
void verifyFFmpegLoaded() {
    #define CHECK_PTR(name) \
        if (!name##_ptr) Logger::error("Not loaded: " #name);

    CHECK_PTR(avformat_alloc_context);
    CHECK_PTR(avcodec_find_encoder);
    // ... verificar todas
}
```

### Logging de Dynamic Loading

```cpp
bool loadWithLogging(void* lib, const char* funcName, void** funcPtr) {
    *funcPtr = GET_PROC(lib, funcName);
    if (*funcPtr) {
        Logger::debug("✓ Loaded: " + std::string(funcName));
        return true;
    } else {
        Logger::error("✗ Failed: " + std::string(funcName));
        return false;
    }
}
```

### ldd vs Runtime

```bash
# Verificar que NO depende de libcef/libav
ldd ./hls-generator | grep -E "(libcef|libav)"
# Debe estar vacío

# Ver qué se carga en runtime
LD_DEBUG=libs ./hls-generator 2>&1 | grep -E "(libcef|libav)"
# Debe mostrar dlopen de OBS libs
```

---

## Conclusión

Esta arquitectura de **dynamic loading** permite:

✅ **Binarios pequeños** (2.7 MB vs 200+ MB)
✅ **Sin descargas** (usa bibliotecas del sistema)
✅ **Auto-actualización** (usa versión de OBS)
✅ **Múltiples versiones** (fallback chains)
✅ **Cross-platform** (misma arquitectura Linux/Windows)

El mismo patrón puede aplicarse a cualquier biblioteca grande (Vulkan, CUDA, OpenCL, etc.).

---

**Referencias**:
- Código completo en `src/ffmpeg_loader.cpp`, `src/cef_loader.cpp`
- Ver `docs/DEVELOPMENT-JOURNEY.md` para contexto e historia
- Ver `CMakeLists.txt` para configuración de build completa

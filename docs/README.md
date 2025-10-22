# Documentación HLS Generator

## Índice de Documentos

### Desarrollo y Arquitectura

#### [Development Journey](DEVELOPMENT-JOURNEY.md) ⭐⭐ ESENCIAL

**Historia completa del desarrollo de esta herramienta** - Cómo se construyó desde cero, todos los problemas encontrados y las soluciones.

**Contenido**:
- Proceso completo de desarrollo (9 días condensados)
- Todas las barreras encontradas y cómo se resolvieron
- Decisiones técnicas críticas y por qué se tomaron
- Lecciones aprendidas que ahorran días de trabajo
- Troubleshooting reference completa
- Si algún día necesitas reconstruir esto: **lee este documento primero**

**Ideal para**:
- Entender TODO el proceso de desarrollo
- Reconstruir la herramienta desde cero
- Aprender de errores y aciertos
- Troubleshooting de problemas complejos

---

#### [Technical Reference](TECHNICAL-REFERENCE.md) ⭐ REFERENCIA TÉCNICA

**Guía técnica completa de dynamic loading FFmpeg + CEF** - Arquitectura, implementación y código de todo el sistema.

**Contenido**:
- Arquitectura completa del dynamic loading (FFmpeg + CEF)
- Implementación paso a paso con código fuente
- 45 funciones FFmpeg + 175 funciones CEF
- OBS detection system multi-plataforma
- Cross-platform abstractions (Linux/Windows)
- Build system configuration
- Template genérico para aplicar a otras bibliotecas

**Ideal para**:
- Referencia técnica durante desarrollo
- Entender la implementación completa
- Aplicar el patrón a otras bibliotecas (Vulkan, CUDA, OpenCL)
- Debugging y troubleshooting

---

### Especificaciones Técnicas y Compilación

#### [Technical Specifications](SPECIFICATIONS.md)

Especificaciones técnicas completas del proyecto HLS Generator.

**Contenido**:
- Arquitectura del sistema
- Formatos soportados (archivos, SRT, RTMP, NDI, RTSP, browser)
- Requisitos del sistema
- Guía de compilación e instalación
- Solución de problemas

**Ideal para**:
- Entender la arquitectura completa del sistema
- Conocer todos los formatos de entrada soportados
- Configurar el entorno de compilación

---

#### [Windows Build Guide](WINDOWS-BUILD.md)

Guía completa para compilación en Windows (cross-compilation desde Linux).

**Contenido**:
- Compilación cruzada con MinGW
- Compilación nativa con MSYS2
- Características del binario estático Windows
- Verificación de dependencias

**Ideal para**:
- Compilar binarios para Windows desde Linux
- Crear distribuciones Windows auto-contenidas
- Entender las diferencias entre compilación Linux/Windows

---

## Guía Rápida: Browser Source

### Requisitos

```bash
# Instalar OBS Studio (incluye libcef.so)
sudo apt install obs-studio
```

### Uso Básico

```bash
# Capturar página web y generar HLS
./hls-generator https://example.com ./output

# Capturar reloj web
./hls-generator https://time.gov ./output
```

### Archivos Generados

```
output/
├── playlist.m3u8      # Playlist HLS
├── segment000.ts      # Segmento de video 0
├── segment001.ts      # Segmento de video 1
└── segment002.ts      # Segmento de video 2 (continúa...)
```

### Verificar Compatibilidad

El programa verifica automáticamente la compatibilidad con la versión de CEF instalada:

```
[INFO] CEF version detected:
[INFO]   Runtime:  CEF 127.0.0 (Chromium 127.0.6533.120)
[INFO]   Compiled: CEF 127.x.x (Chromium 127.0.6533.x)
[INFO] CEF version check: OK (API compatible)
```

Si OBS actualiza a una versión incompatible:

```
[ERROR] CEF API INCOMPATIBILITY DETECTED!
[ERROR] You MUST recompile: cd build && cmake .. && make
```

### Solución de Problemas

#### GPU Process Crashes

**Síntoma**: Logs con `[ERROR:viz_main_impl.cc] Exiting GPU process`

**Causa**: Configuración incorrecta de Ozone platform

**Solución**: Ya configurado en el código (`ozone-platform=x11`)

#### Página No Carga

**Síntoma**: Solo frames negros

**Solución**:
1. Verificar conexión a internet
2. Probar URL en navegador normal
3. Revisar logs de CEF para errores de red

#### Solo Genera Un Segmento

**Síntoma**: Solo aparece `segment000.ts`

**Causa**: `hls_flags` no configurado

**Solución**: Ya solucionado (automático para browser sources)

---

## Arquitectura General

```
Browser Input (CEF) → Frame Capture → Color Conversion → H.264 Encoder → HLS Muxer → Output
```

### Componentes Principales

| Componente | Descripción |
|------------|-------------|
| **CEF Backend** | Renderiza páginas web usando Chromium |
| **Browser Input** | Interfaz entre CEF y FFmpeg |
| **FFmpeg Wrapper** | Codificación H.264 y generación HLS |
| **obs-browser-page** | Subprocess helper para multi-proceso |

### Dependencias

- **libcef.so**: Biblioteca de Chromium (desde OBS)
- **libcef_dll_wrapper**: Wrapper C++ (compilado localmente)
- **FFmpeg**: Codificación y muxing
- **obs-browser-page**: Helper subprocess

---

## Desarrollo

### Compilar

```bash
mkdir build
cd build
cmake ..
make
```

### Estructura del Proyecto

```
hls-generator/
├── src/
│   ├── cef_backend.cpp          # Implementación CEF
│   ├── cef_backend.h            # Headers del proyecto
│   ├── browser_input.cpp        # Captura de frames
│   └── ffmpeg_wrapper.cpp       # Codificación HLS
├── third_party/
│   ├── cef/
│   │   ├── include/             # Headers CEF (3.5 MB)
│   │   └── libcef_dll/          # Wrapper source (188 archivos)
│   └── build/ffmpeg-headers-windows/  # Para cross-compilation
└── docs/
    ├── README.md                            # Este archivo
    ├── CEF-DYNAMIC-LOADING.md               # Guía dynamic loading
    └── CEF_BROWSER_SOURCE_IMPLEMENTATION.md # Implementación browser
```

### Modificar Configuración CEF

Editar `src/cef_backend.cpp`:

```cpp
class SimpleApp : public CefApp {
    void OnBeforeCommandLineProcessing(...) {
        // Agregar o modificar switches aquí
        command_line->AppendSwitch("mi-nuevo-switch");
    }
};
```

### Cambiar Resolución

```cpp
// src/browser_input.cpp
BrowserInput::BrowserInput(const std::string& url)
    : width_(1920),   // Cambiar aquí
      height_(1080)   // Cambiar aquí
{
    // ...
}
```

### Cambiar Frame Rate

```cpp
// src/cef_backend.cpp
browser_settings.windowless_frame_rate = 60;  // Cambiar de 30 a 60
```

---

## Tamaño del Proyecto

Optimizado para minimizar espacio en disco:

| Componente | Tamaño |
|------------|--------|
| Headers CEF | 3.5 MB |
| libcef_dll_wrapper (compilado) | 25 MB |
| build/ffmpeg-headers-windows | 183 MB |
| Binarios compilados | 10-15 MB |
| **Total** | **~228 MB** |

Comparado con incluir CEF standalone completo: **8 GB** → 97% de reducción.

---

## Preguntas Frecuentes

### ¿Necesito OBS instalado?

Sí, el programa usa `libcef.so` de OBS:
```bash
sudo apt install obs-studio
```

### ¿Funciona sin sistema gráfico (headless)?

Sí, CEF está configurado para rendering offscreen. No necesita X11 display activo.

### ¿Qué pasa si actualizo OBS?

El programa verifica automáticamente compatibilidad. Si el API hash cambia, te pedirá recompilar:
```bash
cd build && make
```

### ¿Puedo usar otro CEF?

Sí, pero necesitarás:
1. Descargar CEF binary distribution
2. Modificar `CMakeLists.txt` para apuntar a tu CEF
3. Configurar recursos y locales
4. Implementar subprocess handling

Más simple: usar CEF de OBS (ya configurado).

### ¿Por qué usar multi-proceso?

- Mayor estabilidad (crashes aislados)
- Mejor rendimiento (paralelización)
- Arquitectura recomendada por CEF
- Sandbox de seguridad

### ¿Cuánta CPU usa?

- Rendering: ~10-15% (single core)
- Codificación H.264: ~5-10% (single core)
- Total: ~15-25% en sistema moderno

### ¿Funciona en Windows?

Actualmente solo Linux. Para Windows necesitarías:
- Port de detección de OBS
- CEF para Windows
- Ajustes en command-line switches
- Compilación con MinGW o MSVC

---

## Contacto y Contribuciones

Para reportar problemas o sugerir mejoras:
- Revisar primero [CEF_BROWSER_SOURCE_IMPLEMENTATION.md](CEF_BROWSER_SOURCE_IMPLEMENTATION.md)
- Incluir logs completos
- Especificar versión de OBS y sistema operativo

---

**Última actualización**: Octubre 2024

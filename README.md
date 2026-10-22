# ClipSampler10 - VSTi & AAX YouTube Audio Sampler

**ClipSampler10** es un instrumento virtual multiplataforma (Windows y macOS) diseñado con el framework **JUCE (v7/v8)** y **CMake**, concebido para capturar fragmentos de audio de alta fidelidad desde YouTube con un **límite estricto e infranqueable de 10 segundos**, tocarlos melódicamente vía MIDI en el Piano Roll y soltarlos directamente en las pistas de tu DAW mediante drag-and-drop.

---

## 🚀 Características y Mejoras Implementadas

1. **Sampler Instrumental MIDI (`juce::Synthesiser`):**
   - Motor polifónico de 8 voces con `juce::SamplerSound`. Al extraer la muestra, se mapea automáticamente a todas las notas MIDI (0-127, raíz C3), permitiendo tocar el fragmento desde el Piano Roll o teclado controlador.
2. **Sincronización Dinámica de Sample Rate:**
   - Detecta la frecuencia de muestreo del DAW (`processor.getSampleRate()`) y la pasa dinámicamente al flag `-ar` de `ffmpeg` (44.1k, 48k, 96k, 192k) para evitar remuestreos innecesarios.
3. **Filtros Anti-Click y Normalización:**
   - Micro-fades DSP de 10ms aplicados al inicio y al final (`afade=t=in:ss=0:d=0.01,afade=t=out:st=9.99:d=0.01`) con normalización opcional a -1 dB, garantizando que el bucle o disparo no tenga pops ni clicks de DC offset.
4. **Nombres de Archivo Limpios y Reales:**
   - Usa `--restrict-filenames -o "%(title).40s_%(section_start)s.%(ext)s"`. Al arrastrar el `.wav` al DAW, la pista muestra el nombre real del video y el segundo exacto extraído.
5. **Persistencia de Sesión DAW (`juce::ValueTree`):**
   - Guarda y restaura automáticamente la última URL, el segundo de inicio y la ruta del último `.wav` al guardar el proyecto en el DAW.
6. **Mantenimiento Integrado en GUI:**
   - Botón **Actualizar yt-dlp (-U)** para mantener al día el motor de descarga frente a cambios en la plataforma YouTube.
   - Botón **Limpiar Caché** para borrar archivos `.wav` temporales acumulados.
7. **Formatos Exclusivos:**
   - **VST3** (64-bit) y **AAX** (64-bit Avid Pro Tools).
8. **Políticas de Usuario y EULA Integrados:**
   - Pantalla obligatoria de aceptación de términos de licencia y aviso legal de derechos de autor (*Fair Use Disclaimer*) en los instaladores.

---

## 📁 Estructura del Proyecto

```text
ClipSampler10/
├── CMakeLists.txt                    # Configuración CMake con JUCE, IS_SYNTH, LTO y VST3/AAX
├── README.md                         # Resumen técnico
├── documentacion.md                  # Documentación técnica exhaustiva
├── analicis.md                       # Informe de auditoría de arquitectura y calidad
├── build_windows.ps1                 # Script automatizado de compilación y empaquetado (1 clic)
├── Source/
│   ├── DSP/
│   │   ├── PluginProcessor.h         # Motor DSP, juce::Synthesiser MIDI, APVTS y coordinador
│   │   └── PluginProcessor.cpp       # processBlock wait-free (ScopedTryLock) y persistencia
│   ├── Services/
│   │   ├── AudioExtractorWorker.h    # Worker asíncrono (SR dinámico, anti-click, clean names)
│   │   └── AudioExtractorWorker.cpp  # yt-dlp, ffmpeg, caché de rutas y limpieza segura
│   └── UI/
│       ├── PluginEditor.h            # GUI dividida en 2 paneles desacoplada con SafePointer
│       ├── PluginEditor.cpp          # Sincronización reactiva a 30 FPS y drag dual
│       ├── WaveformComponent.h       # Componente interactivo de forma de onda
│       └── WaveformComponent.cpp     # Dibujo con juce::AudioThumbnail y badge dinámico
├── installer/
│   ├── license.txt                   # Políticas de usuario y EULA (Fair Use & Disclaimer)
│   ├── windows_installer.iss         # Script Inno Setup con salida unificada en dist_windows/
│   ├── macos_distribution.xml        # Distribution XML para productbuild
│   └── build_pkg.sh                  # Script de empaquetado para macOS
└── resources/
    ├── README.md                     # Guía de binarios auxiliares (yt-dlp, ffmpeg)
    └── tools/                        # Carpeta para colocar yt-dlp.exe y ffmpeg.exe portátiles
```

---

## 🛠️ Requisitos Previos e Instalación Rápida

### En Windows (PowerShell como Administrador):
```powershell
winget install Kitware.CMake yt-dlp.yt-dlp jrsoftware.InnoSetup Microsoft.VisualStudio.2022.BuildTools
```

### En macOS (Terminal con Homebrew):
```bash
brew install cmake yt-dlp ffmpeg
```

---

## 💻 Instrucciones de Compilación

### 1. Windows: 1 Clic Automatizado
```powershell
.\build_windows.ps1
```

O manual:
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
iscc installer\windows_installer.iss
```

### 2. macOS: Universal Binary
```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13"
cmake --build build --config Release
chmod +x installer/build_pkg.sh
./installer/build_pkg.sh
```

---

## 🎧 Uso en el DAW

1. Carga **ClipSampler10** en una pista de Instrumento Virtual / MIDI.
2. Pega una URL de YouTube y define el segundo de inicio.
3. Pulsa **Extraer Muestra de 10s**.
4. Escucha por el mezclador pulsando **Play** o **tocando notas en el Piano Roll**.
5. Arrastra la muestra directamente desde el visor de onda o el botón de arrastre hacia cualquier pista de audio del DAW.

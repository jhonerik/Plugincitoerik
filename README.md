# ClipSampler v2.4 - VSTi & AAX Multi-Platform Audio Sampler

**ClipSampler v2.4** es un instrumento virtual multiplataforma (Windows y macOS) desarrollado con **JUCE (v7/v8)** y **CMake**, diseñado para extraer fragmentos de audio de alta fidelidad (**3s, 10s o 30s**) desde **YouTube, TikTok e Instagram**, procesarlos con IA Anti-Copyright, rebanarlos automáticamente al estilo **MPC Auto-Chop**, convertirlos de **Audio a MIDI** y arrastrarlos directamente a las pistas de tu DAW.

> 📖 **¿Buscas cómo usar el plugin paso a paso?** Consulta el [**Manual de Usuario Completo (`MANUAL_DE_USUARIO.md`)**](./MANUAL_DE_USUARIO.md).

---

## 🚀 Características Principales (v2.4)

1. **Extracción Multi-Plataforma Inteligente (`YouTube`, `TikTok`, `Instagram`):**
   - Selector dedicado de plataforma y auto-detección al pegar enlaces de `youtube.com`, `youtu.be`, `tiktok.com` o `instagram.com/reel`.
   - Botón **🔍 Analizar** que consulta título y duración total del video sin descargarlo y ajusta automáticamente el deslizador de tiempo.
2. **Duraciones de Captura Flexibles (`3s`, `10s`, `30s`):**
   - Selector instantáneo de duración de recorte con micro-fades anti-click de 10 ms y normalización a -1 dB.
3. **Doble Visor de Onda Interactiva + IA Derechos:**
   - **Visor 1 (Muestra Original):** Muestra la onda recién extraída con detección automática de **BPM** y **Tonalidad (Key)**.
   - **Visor 2 (Audio Procesado IA Derechos):** Al pulsar **🤖 Derechos IA (±3 st)**, genera una versión transformada con pitch-shifting con preservación de formantes, micro-modulación y filtrado acústico.
   - **🛡️ Detectar Derechos:** Analiza la huella espectral del Visor 1 y Visor 2 para verificar el nivel de coincidencia frente a sistemas de Content ID.
4. **Modo MPC Auto-Chop + Rack de Efectos Macro:**
   - **🥁 Modo Auto-Chop (MPC):** Detecta transientes rítmicos y asigna hasta 16 rebanadas cromáticas (`C1` a `D#2`) para dispararlas desde tu controlador MIDI o Piano Roll.
   - **🎛️ FX Macro:** Perillas de **📻 Lo-Fi 12-Bit**, **🌀 Pitch (±12 semitonos)** y **🔗 Sync BPM al DAW** aplicables en tiempo real al Visor 1 o Visor 2.
5. **Conversor de Audio a MIDI + Drag & Drop Dual:**
   - Arrastra el archivo **`.WAV`** o la transcripción melódica **`.MID`** directamente desde el plugin hacia cualquier pista de tu DAW.
6. **Diseño Elegante Royal Sapphire & 100% Responsive:**
   - Interfaz redimensionable desde `680x540` hasta `1800x1300` con tipografía auto-adaptativa y acabado de estudio en tonos azul zafiro y titanio satinado.
7. **Seguridad Anti-Piratería de Grado Industrial:**
   - Prueba gratuita de 3 días con reloj monótono anti-rollback, vinculación criptográfica por Hardware ID (`MCH-XXXX-XXXX-XXXX`), bóvedas triplicadas cifradas con AES-CTR + HMAC-SHA256 y servidor local/web de licencias SQLite.

---

## 📁 Estructura del Proyecto

```text
ClipSampler/
├── CMakeLists.txt                        # Configuración CMake (objetivo ClipSampler -> build/ClipSampler_artefacts)
├── README.md                             # Resumen general del proyecto
├── MANUAL_DE_USUARIO.md                  # Manual de usuario completo en español
├── documentacion.md                      # Documentación técnica de arquitectura
├── analicis.md                           # Informe de auditoría técnica y seguridad
├── build_windows.ps1                     # Script automatizado de compilación y empaquetado (1 clic)
├── Source/
│   ├── DSP/
│   │   ├── PluginProcessor.h             # Motor DSP, síntesis MIDI, Auto-Chop, Audio-a-MIDI y APVTS
│   │   └── PluginProcessor.cpp           # Implementación comentada en español del procesador de audio
│   ├── Security/
│   │   ├── LicenseManager.h              # Gestor de licencias, Trial de 3 días y Hardware ID
│   │   └── LicenseManager.cpp            # Cifrado de bóvedas, anti-debug y validación criptográfica
│   ├── Services/
│   │   ├── AudioExtractorWorker.h        # Hilo trabajador asíncrono (YouTube/TikTok/Instagram + IA)
│   │   └── AudioExtractorWorker.cpp      # Ejecución de yt-dlp/ffmpeg, Derechos IA y huella espectral
│   └── UI/
│       ├── PluginEditor.h                # Interfaz gráfica responsive Royal Sapphire
│       ├── PluginEditor.cpp              # Layout adaptable, LookAndFeel auto-escalable y controles
│       ├── WaveformComponent.h           # Visor interactivo de forma de onda y marcadores MPC
│       ├── WaveformComponent.cpp         # Renderizado de onda, playhead y líneas de Auto-Chop
│       ├── LicenseOverlayComponent.h     # Modal de activación de licencia y estado de prueba
│       └── LicenseOverlayComponent.cpp   # Interfaz de activación Online/Offline
├── license_system/
│   ├── license_server.py                 # Servidor Web + Base de Datos SQLite + Generador de Licencias
│   └── Iniciar_Generador_Licencias.bat   # Lanzador en 1 clic del panel de licencias
├── installer/
│   ├── license.txt                       # Acuerdo de licencia EULA en español
│   ├── windows_installer.iss             # Script Inno Setup (genera ClipSampler_Windows_Installer_v2.4.0.exe)
│   ├── macos_distribution.xml            # Configuración de instalador macOS
│   └── build_pkg.sh                      # Script de empaquetado macOS (.pkg)
└── resources/
    ├── README.md                         # Guía de herramientas externas
    └── tools/                            # Binarios portátiles yt-dlp.exe y ffmpeg.exe
```

---

## 💻 Compilación Rápida en Windows

Ejecuta en PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_windows.ps1
```

Esto genera:
- **Plugin VST3:** `build\ClipSampler_artefacts\Release\VST3\ClipSampler.vst3`
- **Instalador Windows:** `dist_windows\ClipSampler_Windows_Installer_v2.4.0.exe`

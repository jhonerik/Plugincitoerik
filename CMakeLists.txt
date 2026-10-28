cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# ClipSampler v2.6 - Instrumento Virtual (VST3 de 64 bits)
# Extractor Multi-Plataforma (YouTube, TikTok, Instagram), Rebanador MPC,
# Procesador de Efectos Macro, Detector de BPM/Tonalidad, Conversor Audio a MIDI
# y Explorador Integrado Freesound API v2.
# ==============================================================================
project(ClipSampler VERSION 2.6.1 LANGUAGES C CXX)

# Estándar C++17 requerido por JUCE 8 y las utilidades modernas de concurrencia
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Soporte de Binario Universal para macOS (Intel x86_64 + Apple Silicon arm64)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquitecturas de macOS a compilar" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Versión mínima de macOS soportada")
endif()

# ------------------------------------------------------------------------------
# Integración Automática del Framework JUCE
# ------------------------------------------------------------------------------
# Busca primero una instalación local de JUCE; si no existe, la descarga con FetchContent
find_package(JUCE CONFIG QUIET)

if(NOT TARGET juce::juce_audio_utils)
    message(STATUS "JUCE local no encontrado. Descargando JUCE 8.0.4 mediante FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ------------------------------------------------------------------------------
# Formato Exclusivo del Plugin: VST3 (64 bits)
# ------------------------------------------------------------------------------
set(PLUGIN_FORMATS VST3)

# ------------------------------------------------------------------------------
# Declaración Principal del Plugin (Objetivo: ClipSampler)
# Genera la carpeta de artefactos: build/ClipSampler_artefacts/
# ------------------------------------------------------------------------------
juce_add_plugin(ClipSampler
    COMPANY_NAME                "AudioInnovations"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE    Clip
    PLUGIN_CODE                 Cs10
    FORMATS                     ${PLUGIN_FORMATS}
    PRODUCT_NAME                "ClipSampler"
    BUNDLE_ID                   "com.audioinnovations.clipsampler"
    VERSION                     "2.6.1"
)

# ------------------------------------------------------------------------------
# Archivos Fuente Modulares (DSP / Seguridad / Servicios / Interfaz Gráfica)
# ------------------------------------------------------------------------------
target_sources(ClipSampler
    PRIVATE
        Source/DSP/PluginProcessor.h
        Source/DSP/PluginProcessor.cpp
        Source/Security/LicenseManager.h
        Source/Security/LicenseManager.cpp
        Source/Services/AudioExtractorWorker.h
        Source/Services/AudioExtractorWorker.cpp
        Source/UI/FreesoundOverlayComponent.h
        Source/UI/FreesoundOverlayComponent.cpp
        Source/UI/LicenseOverlayComponent.h
        Source/UI/LicenseOverlayComponent.cpp
        Source/UI/PluginEditor.h
        Source/UI/PluginEditor.cpp
        Source/UI/WaveformComponent.h
        Source/UI/WaveformComponent.cpp
)

# Directorios de inclusión para organizar los encabezados por capas
target_include_directories(ClipSampler
    PRIVATE
        Source
        Source/DSP
        Source/Security
        Source/Services
        Source/UI
)

# ------------------------------------------------------------------------------
# Módulos de JUCE Vinculados
# ------------------------------------------------------------------------------
target_link_libraries(ClipSampler
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_cryptography
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

# ------------------------------------------------------------------------------
# Definiciones de Preprocesador y Optimización de Tamaño/Rendimiento
# ------------------------------------------------------------------------------
target_compile_definitions(ClipSampler
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_USE_FLAC=1
        JUCE_USE_OGGVORBIS=1
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_USE_LAME_AUDIO_FORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=1
)

# Opciones de seguridad de memoria y codificación UTF-8 en compilador MSVC (Windows)
if(MSVC)
    target_compile_options(ClipSampler PRIVATE /utf-8 /GS)
    target_link_options(ClipSampler PRIVATE /DYNAMICBASE /NXCOMPAT)
endif()

# Optimización de tiempo de enlace (LTO) en compilaciones Release
set_target_properties(ClipSampler PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)

message(STATUS "==========================================================")
message(STATUS " ClipSampler v2.6.1 configurado correctamente!")
message(STATUS " Formatos de destino: ${PLUGIN_FORMATS}")
message(STATUS " Instrumento Virtual (IS_SYNTH): TRUE")
message(STATUS "==========================================================")

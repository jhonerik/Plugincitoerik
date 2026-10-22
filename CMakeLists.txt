cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# ClipSampler10 - Professional 10-Second YouTube Sample Grabber VSTi / AAX
# ==============================================================================
project(ClipSampler10 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS Universal Binary support (Intel x86_64 + Apple Silicon arm64)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures to build" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment target")
endif()

# ------------------------------------------------------------------------------
# JUCE Dependency Integration
# ------------------------------------------------------------------------------
# Try finding a local JUCE installation first, otherwise fetch JUCE automatically
find_package(JUCE CONFIG QUIET)

if(NOT TARGET juce::juce_audio_utils)
    message(STATUS "Local JUCE not found. Fetching JUCE via FetchContent...")
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
# AAX SDK Setup
# ------------------------------------------------------------------------------
# AAX requires the Avid AAX Library SDK. Pass -DAAX_SDK_PATH="/path/to/aax-sdk"
if(DEFINED AAX_SDK_PATH)
    message(STATUS "Configuring AAX SDK path: ${AAX_SDK_PATH}")
    juce_set_aax_sdk_path("${AAX_SDK_PATH}")
endif()

# ------------------------------------------------------------------------------
# Formats Configuration: STRICTLY VST3 and AAX ONLY
# ------------------------------------------------------------------------------
set(PLUGIN_FORMATS VST3)

if(DEFINED AAX_SDK_PATH AND EXISTS "${AAX_SDK_PATH}")
    list(APPEND PLUGIN_FORMATS AAX)
else()
    message(WARNING "AAX_SDK_PATH is not defined or does not exist. AAX format target will be created if AAX_SDK_PATH is provided at build time.")
    # Always include AAX in the format list if AAX_SDK_PATH is set, or if requested by user
    if(DEFINED FORCE_AAX_BUILD AND FORCE_AAX_BUILD)
        list(APPEND PLUGIN_FORMATS AAX)
    endif()
endif()

# ------------------------------------------------------------------------------
# Plugin Declaration
# ------------------------------------------------------------------------------
juce_add_plugin(ClipSampler10
    COMPANY_NAME                "AudioInnovations"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE    Clip
    PLUGIN_CODE                 Cs10
    FORMATS                     ${PLUGIN_FORMATS}
    PRODUCT_NAME                "ClipSampler10"
    BUNDLE_ID                   "com.audioinnovations.clipsampler10"
    VERSION                     "1.0.0"
    AAX_CATEGORY                AAX_ePlugInCategory_SWGenerators
)

# ------------------------------------------------------------------------------
# Modular Source Files (DSP / Services / UI)
# ------------------------------------------------------------------------------
target_sources(ClipSampler10
    PRIVATE
        Source/DSP/PluginProcessor.h
        Source/DSP/PluginProcessor.cpp
        Source/Services/AudioExtractorWorker.h
        Source/Services/AudioExtractorWorker.cpp
        Source/UI/PluginEditor.h
        Source/UI/PluginEditor.cpp
        Source/UI/WaveformComponent.h
        Source/UI/WaveformComponent.cpp
)

target_include_directories(ClipSampler10
    PRIVATE
        Source
        Source/DSP
        Source/Services
        Source/UI
)

# ------------------------------------------------------------------------------
# Linked JUCE Modules (Trimmed for VST3 / AAX)
# ------------------------------------------------------------------------------
target_link_libraries(ClipSampler10
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

# ------------------------------------------------------------------------------
# Compile Definitions & Size/Performance Optimizations
# ------------------------------------------------------------------------------
target_compile_definitions(ClipSampler10
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_LAME_AUDIO_FORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0
)

if(MSVC)
    target_compile_options(ClipSampler10 PRIVATE /utf-8)
endif()

# Enable Link-Time Optimization (LTO) in Release builds
set_target_properties(ClipSampler10 PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)

# Output build artifacts message
message(STATUS "==========================================================")
message(STATUS " ClipSampler10 configured successfully!")
message(STATUS " Target Formats: ${PLUGIN_FORMATS}")
message(STATUS " Virtual Instrument (IS_SYNTH): TRUE")
message(STATUS "==========================================================")

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/PluginProcessor.h"
#include "WaveformComponent.h"
#include "LicenseOverlayComponent.h"

/**
 * Custom LookAndFeel to ensure button fonts (especially the 3-Day Trial button)
 * scale dynamically when the user resizes/stretches the plugin window.
 */
class ScalablePluginLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setUiScale(float scale) { uiScale = juce::jlimit(0.85f, 1.85f, scale); }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        float targetSize = juce::jlimit(12.5f, 20.0f, buttonHeight * 0.44f * uiScale);
        return juce::Font(targetSize, juce::Font::bold);
    }

private:
    float uiScale = 1.0f;
};

/**
 * ClipSampler10AudioProcessorEditor (ClipSampler v2.2)
 * 
 * Fully resizable, responsive interface with:
 * - Corner resizer to stretch or shrink the window freely.
 * - Automatic YouTube video duration detection (Start -> End time display).
 * - Multi-duration extraction buttons: 3s, 10s, and 30s.
 * - Split Dual Waveform Viewer:
 *     Visor 1: Original Extracted Audio
 *     Visor 2: "Derechos IA" Modified Audio (±3 Semitones + Phase Fingerprint Evasion)
 * - Copyright Detector (Audio 1), Derechos AI Transformer (±3 st), and Post-Transform Copyright Verifier (Audio 2).
 */
class ClipSampler10AudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::DragAndDropContainer,
                                          private juce::Timer
{
public:
    explicit ClipSampler10AudioProcessorEditor(ClipSampler10AudioProcessor&);
    ~ClipSampler10AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;

    // Action handlers
    void onPasteClicked();
    void onDetectDurationClicked();
    void onExtractDurationClicked(int durationSec);
    void onCheckCopyrightOriginalClicked();
    void onApplyDerechosClicked();
    void onVerifyDerechosFreeClicked();
    void onPlayClicked();
    void onStopClicked();
    void onExportWavClicked();
    void onDragDawClicked();
    void onUpdateExtractorClicked();
    void onCleanCacheClicked();
    void onLicenseBadgeClicked();
    void updateLicenseUiState();
    void refreshVideoTimeRangeLabel();

    ClipSampler10AudioProcessor& audioProcessor;
    ScalablePluginLookAndFeel scalableLnf;
    int licenseCheckTickCounter = 0;
    int lastKnownVideoDuration = -1;
    bool nextShiftPositive = true;

    // Header Maintenance & License Status Buttons
    juce::TextButton licenseBadgeButton;
    juce::TextButton updateExtractorButton { juce::String::fromUTF8("Actualizar yt-dlp") };
    juce::TextButton cleanCacheButton { juce::String::fromUTF8("Limpiar Caché") };

    // =========================================================================
    // Top Panel: Link, Video Duration Detection & Multi-Duration Capture (3s/10s/30s)
    // =========================================================================
    juce::GroupComponent topSectionGroup { "topGroup", juce::String::fromUTF8("1. CAPTURA DE YOUTUBE, DURACIÓN DEL VIDEO Y EXTRACCIÓN (3s / 10s / 30s)") };
    
    juce::Label urlLabel { {}, juce::String::fromUTF8("URL de YouTube:") };
    juce::TextEditor urlEditor;
    juce::TextButton pasteButton { juce::String::fromUTF8("📋 Pegar") };
    juce::TextButton detectDurationButton { juce::String::fromUTF8("🔍 Detectar Duración") };

    juce::Label startTimeLabel { {}, juce::String::fromUTF8("Inicio (MM:SS):") };
    juce::TextEditor startTimeEditor;
    juce::Slider startTimeSlider;
    juce::Label videoRangeBadgeLabel;

    // 3 Multi-Duration Extract Buttons (3s, 10s, 30s)
    juce::TextButton extract3sButton  { juce::String::fromUTF8("⚡ Extraer 3 Segundos") };
    juce::TextButton extract10sButton { juce::String::fromUTF8("⚡ Extraer 10 Segundos") };
    juce::TextButton extract30sButton { juce::String::fromUTF8("⚡ Extraer 30 Segundos") };

    // 3 Copyright & Derechos AI Buttons
    juce::TextButton detectCopyrightBtn { juce::String::fromUTF8("🔍 1. Detectar Copyright (Audio 1)") };
    juce::TextButton derechosAiBtn      { juce::String::fromUTF8("🛡️ 2. Derechos IA (Alterar ±3 Semitonos)") };
    juce::TextButton verifyFreeBtn      { juce::String::fromUTF8("✅ 3. Verificar Audio 2 (Sin Derechos)") };

    double currentProgressValue = 0.0;
    juce::ProgressBar progressBar { currentProgressValue };
    juce::Label statusLabel;
    juce::Label copyrightBadgeLabel;
    juce::String lastDisplayedStatus;

    // =========================================================================
    // Bottom Panel: Split Dual Waveform Viewer (Original vs. Derechos Modificado)
    // =========================================================================
    juce::GroupComponent bottomSectionGroup { "bottomGroup", juce::String::fromUTF8("2. VISOR DOBLE DE AUDIO (ORIGINAL vs. MODIFICADO SIN DERECHOS) Y EXPORTACIÓN") };
    
    WaveformComponent originalWaveformComponent;
    WaveformComponent transformedWaveformComponent;

    juce::TextButton playButton { juce::String::fromUTF8("▶ Reproducir Activo") };
    juce::TextButton stopButton { juce::String::fromUTF8("■ Detener") };
    juce::TextButton dragDawButton { juce::String::fromUTF8("⇲ Arrastrar Audio Activo al DAW") };
    juce::TextButton exportWavButton { juce::String::fromUTF8("Exportar .WAV...") };

    std::unique_ptr<juce::FileChooser> fileChooser;
    LicenseOverlayComponent licenseOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSampler10AudioProcessorEditor)
};

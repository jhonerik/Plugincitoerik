#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/PluginProcessor.h"
#include "WaveformComponent.h"
#include "LicenseOverlayComponent.h"

/**
 * Custom LookAndFeel to ensure button fonts scale dynamically when the user resizes
 * the plugin window, and to render sleek pro-audio rotary knobs for the FX Macro bar.
 */
class ScalablePluginLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setUiScale(float scale) { uiScale = juce::jlimit(0.85f, 1.85f, scale); }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        float targetSize = juce::jlimit(11.5f, 19.5f, buttonHeight * 0.42f * uiScale);
        return juce::Font(targetSize, juce::Font::bold);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 3.5f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Dark metallic knob body
        g.setColour(juce::Colour(0xff181c26));
        g.fillEllipse(rx, ry, rw, rw);

        // Background track arc
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff2b3346));
        g.strokePath(bgArc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active glowing value arc
        juce::Colour arcCol = slider.findColour(juce::Slider::rotarySliderFillColourId);
        juce::Path valArc;
        valArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(arcCol);
        g.strokePath(valArc, juce::PathStrokeType(3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer line
        juce::Path p;
        auto pointerLength = radius * 0.68f;
        auto pointerThickness = 2.4f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(p);
    }

private:
    float uiScale = 1.0f;
};

/**
 * ClipSampler10AudioProcessorEditor (ClipSampler v2.3)
 * 
 * Fully resizable, responsive interface with:
 * - Corner resizer to stretch or shrink the window freely.
 * - Automatic YouTube video duration detection (Start -> End time display).
 * - Multi-duration extraction buttons: 3s, 10s, and 30s.
 * - Conditional "🔄 Nueva Extracción" button (unlocks once both Audio 1 & Audio 2 are extracted/ready).
 * - MPC "Auto-Chop" Mode (transient slicer mapping C1..G2 on the waveform).
 * - FX Macro Rotary Knobs (Lo-Fi 12-Bit, Pitch Semitones, Tone Filter, Auto-Sync) selectable for Audio 1 or Audio 2.
 * - Automatic BPM + Musical Key (Tonalidad) Detector.
 * - Audio-to-MIDI Converter with direct "🎹 Arrastrar .MIDI" button next to "🎵 Arrastrar .WAV".
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
    void onNewExtractionClicked();
    void onCheckCopyrightOriginalClicked();
    void onApplyDerechosClicked();
    void onVerifyDerechosFreeClicked();
    void onSelectFxTargetSlot(int slotIndex);
    void onToggleAutoChopClicked();
    void onToggleAutoSyncClicked();
    void onBakeFxClicked();
    void onPlayClicked();
    void onStopClicked();
    void onExportWavClicked();
    void onDragDawClicked();
    void onDragMidiClicked();
    void onUpdateExtractorClicked();
    void onCleanCacheClicked();
    void onLicenseBadgeClicked();
    void updateLicenseUiState();
    void updateFxAndMpcUiState();
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
    // Top Panel: Link, Video Duration Detection, 3s/10s/30s & "Nueva Extracción"
    // =========================================================================
    juce::GroupComponent topSectionGroup { "topGroup", juce::String::fromUTF8("1. CAPTURA DE YOUTUBE, DURACIÓN, EXTRACCIÓN (3s/10s/30s) Y DERECHOS IA") };
    
    juce::Label urlLabel { {}, juce::String::fromUTF8("URL de YouTube:") };
    juce::TextEditor urlEditor;
    juce::TextButton pasteButton { juce::String::fromUTF8("📋 Pegar") };
    juce::TextButton detectDurationButton { juce::String::fromUTF8("🔍 Detectar Duración") };

    juce::Label startTimeLabel { {}, juce::String::fromUTF8("Inicio (MM:SS):") };
    juce::TextEditor startTimeEditor;
    juce::Slider startTimeSlider;
    juce::Label videoRangeBadgeLabel;

    // 3 Multi-Duration Extract Buttons (3s, 10s, 30s) + Conditional "Nueva Extracción" Button
    juce::TextButton extract3sButton     { juce::String::fromUTF8("⚡ Extraer 3 Segundos") };
    juce::TextButton extract10sButton    { juce::String::fromUTF8("⚡ Extraer 10 Segundos") };
    juce::TextButton extract30sButton    { juce::String::fromUTF8("⚡ Extraer 30 Segundos") };
    juce::TextButton newExtractionButton { juce::String::fromUTF8("🔒 Nueva Extracción (2 Audios)") };

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
    // Bottom Panel: Visor Auto-Chop (Modo MPC), FX Macro, BPM/Key & Drag WAV/MIDI
    // =========================================================================
    juce::GroupComponent bottomSectionGroup { "bottomGroup", juce::String::fromUTF8("2. VISOR AUTO-CHOP (MODO MPC C1-G2), PERILLAS FX MACRO, BPM/KEY Y AUDIO A MIDI") };

    // FX Target Selection (Audio 1 vs Audio 2)
    juce::Label fxMacroTitleLabel { {}, juce::String::fromUTF8("🎛️ FX Macro:") };
    juce::TextButton fxTargetAudio1Btn { juce::String::fromUTF8("🎯 Audio 1") };
    juce::TextButton fxTargetAudio2Btn { juce::String::fromUTF8("🎯 Audio 2") };

    // 3 Main Rotary FX Knobs + Live Readout Labels
    juce::Slider lofiKnob;
    juce::Label  lofiLabel { {}, juce::String::fromUTF8("📻 Lo-Fi 12-Bit: 0%") };

    juce::Slider pitchKnob;
    juce::Label  pitchLabel { {}, juce::String::fromUTF8("🌀 Pitch: 0 st") };

    juce::Slider toneKnob;
    juce::Label  toneLabel { {}, juce::String::fromUTF8("🎚️ Filtro: 100%") };

    // Auto-Sync, Auto-Chop MPC Mode, Bake FX & BPM/Key Detector Badge
    juce::TextButton autoSyncButton { juce::String::fromUTF8("🔗 Sync: OFF") };
    juce::TextButton autoChopButton { juce::String::fromUTF8("🥁 Auto-Chop MPC: ON") };
    juce::TextButton bakeFxButton   { juce::String::fromUTF8("💾 Aplicar FX") };
    juce::Label bpmKeyBadgeLabel;

    WaveformComponent originalWaveformComponent;
    WaveformComponent transformedWaveformComponent;

    // Bottom Transport + Drag .WAV + Drag .MIDI (Piano Roll) + Export
    juce::TextButton playButton      { juce::String::fromUTF8("▶ Reproducir") };
    juce::TextButton stopButton      { juce::String::fromUTF8("■ Detener") };
    juce::TextButton dragDawButton   { juce::String::fromUTF8("🎵 Arrastrar .WAV") };
    juce::TextButton dragMidiButton  { juce::String::fromUTF8("🎹 Arrastrar .MIDI") };
    juce::TextButton exportWavButton { juce::String::fromUTF8("Exportar .WAV...") };

    std::unique_ptr<juce::FileChooser> fileChooser;
    LicenseOverlayComponent licenseOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSampler10AudioProcessorEditor)
};

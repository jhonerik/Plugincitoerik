#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/PluginProcessor.h"
#include "WaveformComponent.h"
#include "LicenseOverlayComponent.h"

/**
 * ScalablePluginLookAndFeel (v2.4 Cyber-Studio Glassmorphic UI/UX)
 * 
 * Implements modern pro-audio UX/UI design patterns:
 * - Glassmorphic elevated section cards with glowing header pills.
 * - Metallic gradient buttons with specular top-edge highlights and hover neon halos.
 * - Brushed-metal rotary FX Macro knobs with radial tick marks and illuminated arcs.
 * - Glowing neon linear time-bar slider with dual-ringed thumb.
 * - Dynamic font scaling across all window sizes.
 */
class ScalablePluginLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setUiScale(float scale) { uiScale = juce::jlimit(0.85f, 1.85f, scale); }
    void setThemeAccent(juce::Colour accent) { themeAccent = accent; }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        float targetSize = juce::jlimit(11.5f, 19.5f, buttonHeight * 0.42f * uiScale);
        return juce::Font(targetSize, juce::Font::bold);
    }

    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        const float cornerSize = 6.5f;

        if (!button.isEnabled())
        {
            g.setColour(juce::Colour(0xff181c26).withAlpha(0.65f));
            g.fillRoundedRectangle(bounds, cornerSize);
            g.setColour(juce::Colour(0xff262c3a));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
            return;
        }

        juce::Colour base = backgroundColour;
        if (shouldDrawButtonAsDown)
            base = base.brighter(0.25f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter(0.12f);

        juce::Colour topCol = base.brighter(0.16f);
        juce::Colour botCol = base.darker(0.28f);

        juce::ColourGradient grad(topCol, bounds.getX(), bounds.getY(),
                                  botCol, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Specular top highlight + glowing border on hover
        juce::Colour borderCol = shouldDrawButtonAsHighlighted
            ? base.brighter(0.55f)
            : base.brighter(0.22f).withAlpha(0.75f);
        g.setColour(borderCol);
        g.drawRoundedRectangle(bounds, cornerSize, shouldDrawButtonAsHighlighted ? 1.6f : 1.1f);
    }

    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                   const juce::String& text, const juce::Justification&,
                                   juce::GroupComponent&) override
    {
        auto bounds = juce::Rectangle<float>(1.0f, 4.0f, (float)width - 2.0f, (float)height - 5.0f);
        const float corner = 10.0f;

        // Elevated Glassmorphic Card Surface
        juce::ColourGradient cardGrad(juce::Colour(0xff171b26), bounds.getX(), bounds.getY(),
                                      juce::Colour(0xff10131b), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(cardGrad);
        g.fillRoundedRectangle(bounds, corner);

        // Subtle outer border with accent tint
        g.setColour(themeAccent.withAlpha(0.28f));
        g.drawRoundedRectangle(bounds, corner, 1.2f);

        // Glowing top header badge bar
        if (text.isNotEmpty())
        {
            juce::Font f(12.0f * uiScale, juce::Font::bold);
            g.setFont(f);
            float textW = juce::jmin((float)width - 32.0f, f.getStringWidthFloat(text) + 24.0f);
            juce::Rectangle<float> pill(14.0f, 0.0f, textW, 20.0f * uiScale);

            g.setColour(juce::Colour(0xff1d2331));
            g.fillRoundedRectangle(pill, 5.0f);
            g.setColour(themeAccent.withAlpha(0.65f));
            g.drawRoundedRectangle(pill, 5.0f, 1.1f);

            g.setColour(juce::Colour(0xffeef4ff));
            g.drawText(text, pill.toNearestInt(), juce::Justification::centred, false);
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        auto trackBounds = juce::Rectangle<float>((float)x, (float)y + (float)height * 0.5f - 4.0f, (float)width, 8.0f);
        g.setColour(juce::Colour(0xff10131c));
        g.fillRoundedRectangle(trackBounds, 4.0f);
        g.setColour(juce::Colour(0xff262e42));
        g.drawRoundedRectangle(trackBounds, 4.0f, 1.0f);

        float fillW = juce::jlimit(0.0f, (float)width, sliderPos - (float)x);
        if (fillW > 2.0f)
        {
            auto fillRect = trackBounds.withWidth(fillW);
            juce::Colour trackCol = slider.findColour(juce::Slider::trackColourId);
            juce::ColourGradient fillGrad(trackCol.darker(0.2f), fillRect.getX(), fillRect.getY(),
                                          trackCol.brighter(0.2f), fillRect.getRight(), fillRect.getY(), false);
            g.setGradientFill(fillGrad);
            g.fillRoundedRectangle(fillRect, 4.0f);
        }

        // Neon Dual-Ring Thumb
        float thumbX = juce::jlimit((float)x + 6.0f, (float)(x + width) - 6.0f, sliderPos);
        float thumbY = (float)y + (float)height * 0.5f;
        juce::Colour thumbCol = slider.findColour(juce::Slider::thumbColourId);

        g.setColour(thumbCol.withAlpha(0.32f));
        g.fillEllipse(thumbX - 10.0f, thumbY - 10.0f, 20.0f, 20.0f);

        g.setColour(thumbCol);
        g.fillEllipse(thumbX - 6.5f, thumbY - 6.5f, 13.0f, 13.0f);

        g.setColour(juce::Colours::white);
        g.fillEllipse(thumbX - 2.5f, thumbY - 2.5f, 5.0f, 5.0f);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Outer tick marks
        g.setColour(juce::Colour(0xff384259));
        for (int i = 0; i <= 8; ++i)
        {
            float tickAngle = rotaryStartAngle + (i / 8.0f) * (rotaryEndAngle - rotaryStartAngle);
            float tx1 = centreX + (radius + 1.5f) * std::sin(tickAngle);
            float ty1 = centreY - (radius + 1.5f) * std::cos(tickAngle);
            float tx2 = centreX + (radius + 3.2f) * std::sin(tickAngle);
            float ty2 = centreY - (radius + 3.2f) * std::cos(tickAngle);
            g.drawLine(tx1, ty1, tx2, ty2, 1.2f);
        }

        // Brushed-metal 3D knob body
        juce::ColourGradient knobGrad(juce::Colour(0xff252b3b), rx, ry,
                                      juce::Colour(0xff11141d), rx, ry + rw, false);
        g.setGradientFill(knobGrad);
        g.fillEllipse(rx, ry, rw, rw);

        // Background track arc
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff262e40));
        g.strokePath(bgArc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active glowing value arc
        juce::Colour arcCol = slider.findColour(juce::Slider::rotarySliderFillColourId);
        juce::Path valArc;
        valArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(arcCol);
        g.strokePath(valArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer needle + glowing tip
        juce::Path p;
        auto pointerLength = radius * 0.68f;
        auto pointerThickness = 2.4f;
        p.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 1.5f, pointerThickness, pointerLength, 1.0f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(p);
    }

private:
    float uiScale = 1.0f;
    juce::Colour themeAccent { 0xff00e5ff };
};

/**
 * ClipSampler10AudioProcessorEditor (ClipSampler v2.4 Multi-Platform Cyber-Studio Edition)
 * 
 * Features:
 * - Multi-Platform Selector: YouTube, TikTok, and Instagram with automatic URL recognition & platform-tuned extraction.
 * - Elevated Glassmorphic UI/UX with dynamic platform color theming, custom rotary knobs, and neon transport controls.
 * - Corner resizer to stretch or shrink the window freely.
 * - Multi-duration extraction buttons: 3s, 10s, and 30s + Conditional "🔄 Nueva Extracción" button.
 * - MPC "Auto-Chop" Mode (transient slicer mapping C1..G2 on the waveform).
 * - FX Macro Rotary Knobs (Lo-Fi 12-Bit, Pitch Semitones, Tone Filter, Auto-Sync) selectable for Audio 1 or Audio 2.
 * - Automatic BPM + Musical Key (Tonalidad) Detector & Audio-to-MIDI Converter ("🎹 Arrastrar .MIDI").
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
    void onSelectPlatform(SourcePlatform platform, bool updateStatusMsg = true);
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
    void updatePlatformUiState();
    void updateFxAndMpcUiState();
    void refreshVideoTimeRangeLabel();

    ClipSampler10AudioProcessor& audioProcessor;
    ScalablePluginLookAndFeel scalableLnf;
    int licenseCheckTickCounter = 0;
    int lastKnownVideoDuration = -1;
    bool nextShiftPositive = true;

    // Header Maintenance & License Status Buttons
    juce::TextButton licenseBadgeButton;
    juce::TextButton updateExtractorButton { juce::String::fromUTF8("Actualizar Motor") };
    juce::TextButton cleanCacheButton { juce::String::fromUTF8("Limpiar Caché") };

    // =========================================================================
    // Top Panel: Platform Selector (YouTube / TikTok / Instagram), URL, Duration & Derechos IA
    // =========================================================================
    juce::GroupComponent topSectionGroup { "topGroup", juce::String::fromUTF8("1. PLATAFORMA (YOUTUBE / TIKTOK / INSTAGRAM), EXTRACCIÓN (3s/10s/30s) Y DERECHOS IA") };

    // Platform Selector Buttons
    juce::Label platformLabel { {}, juce::String::fromUTF8("Plataforma:") };
    juce::TextButton platformYoutubeBtn   { juce::String::fromUTF8("▶️ YouTube") };
    juce::TextButton platformTiktokBtn    { juce::String::fromUTF8("🎵 TikTok") };
    juce::TextButton platformInstagramBtn { juce::String::fromUTF8("📸 Instagram") };
    juce::Label platformHintLabel;

    juce::Label urlLabel { {}, juce::String::fromUTF8("Enlace (URL):") };
    juce::TextEditor urlEditor;
    juce::TextButton pasteButton { juce::String::fromUTF8("📋 Pegar") };
    juce::TextButton detectDurationButton { juce::String::fromUTF8("🔍 Detectar Duración") };

    juce::Label startTimeLabel { {}, juce::String::fromUTF8("Inicio (MM:SS):") };
    juce::TextEditor startTimeEditor;
    juce::Slider startTimeSlider;
    juce::Label videoRangeBadgeLabel;

    // 3 Multi-Duration Extract Buttons (3s, 10s, 30s) + Conditional "Nueva Extracción" Button
    juce::TextButton extract3sButton     { juce::String::fromUTF8("⚡ Extraer 3 Segundos") };
    juce::TextButton extract10sButton    { juce::String::fromUTF8("🔥 Extraer 10 Segundos") };
    juce::TextButton extract30sButton    { juce::String::fromUTF8("🎵 Extraer 30 Segundos") };
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

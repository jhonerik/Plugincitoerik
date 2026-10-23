#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DSP/PluginProcessor.h"

/**
 * WaveformComponent (v2.3)
 * 
 * Dual-slot interactive MPC Auto-Chop waveform visualizer:
 * - Slot 1: Displays the original extracted sample (3s / 10s / 30s).
 * - Slot 2: Displays the "Derechos IA" altered sample (±3 semitones + phase decorrelation).
 * - Draws MPC Auto-Chop transient divider lines and MIDI note headers (C1 | C#1 | D1 | D#1 | E1 | F1 | F#1 ...).
 * - Displays detected BPM & Musical Key (Tonalidad) badge in the header.
 * - Clicking a slice triggers that slice immediately; dragging exports the .WAV directly to the DAW.
 */
class WaveformComponent : public juce::Component,
                          public juce::ChangeListener
{
public:
    WaveformComponent(ClipSampler10AudioProcessor& processorToUse, int slotIndexToUse, const juce::String& titleText);
    ~WaveformComponent() override;

    void paint(juce::Graphics& g) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    ClipSampler10AudioProcessor& processor;
    int slotIndex = 1;
    juce::String viewerTitle;
    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

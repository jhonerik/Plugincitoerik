#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DSP/PluginProcessor.h"

/**
 * WaveformComponent
 * 
 * Dual-slot interactive waveform visualizer:
 * - Slot 1: Displays the original extracted sample (3s / 10s / 30s).
 * - Slot 2: Displays the "Derechos IA" altered sample (±3 semitones + phase decorrelation).
 * Supports clicking to select the active slot for playback/MIDI, and dragging either waveform directly to the DAW.
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

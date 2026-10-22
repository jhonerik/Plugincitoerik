#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DSP/PluginProcessor.h"

/**
 * WaveformComponent
 * 
 * Interactive waveform visualizer with real-time playhead tracking, dynamic format/SR badge
 * and external DAW drag-and-drop.
 * Supports dragging the rendered .wav directly onto any DAW audio track timeline.
 */
class WaveformComponent : public juce::Component,
                          public juce::ChangeListener
{
public:
    explicit WaveformComponent(ClipSampler10AudioProcessor& processorToUse);
    ~WaveformComponent() override;

    void paint(juce::Graphics& g) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Mouse interaction for external drag-and-drop directly into DAW
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    ClipSampler10AudioProcessor& processor;
    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

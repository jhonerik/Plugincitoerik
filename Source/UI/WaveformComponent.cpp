#include "WaveformComponent.h"

WaveformComponent::WaveformComponent(ClipSampler10AudioProcessor& processorToUse)
    : processor(processorToUse)
{
    processor.getThumbnail().addChangeListener(this);
}

WaveformComponent::~WaveformComponent()
{
    processor.getThumbnail().removeChangeListener(this);
}

void WaveformComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &processor.getThumbnail())
        repaint();
}

void WaveformComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging)
        return;

    auto currentFile = processor.getCurrentAudioFile();
    if (!currentFile.existsAsFile())
        return;

    // Minimum drag threshold to prevent accidental drags
    if (event.getDistanceFromDragStart() > 6)
    {
        auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this);
        if (container != nullptr)
        {
            isDragging = true;
            juce::StringArray files;
            files.add(currentFile.getFullPathName());

            juce::Component::SafePointer<WaveformComponent> safeThis(this);
            container->performExternalDragDropOfFiles(files, false, this, [safeThis] {
                if (safeThis != nullptr)
                    safeThis->isDragging = false;
            });
        }
    }
}

void WaveformComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Sleek Glassmorphism / Dark Obsidian container
    g.setColour(juce::Colour(0xff12141a));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(juce::Colour(0xff222733));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

    auto waveformArea = bounds.reduced(8.0f, 10.0f);

    // 2. Grid lines for quarter-duration divisions (0%, 25%, 50%, 75%, 100%)
    g.setColour(juce::Colour(0xff1a2130));
    for (int i = 1; i < 4; ++i)
    {
        float x = waveformArea.getX() + (waveformArea.getWidth() * (i / 4.0f));
        g.drawVerticalLine((int)x, waveformArea.getY(), waveformArea.getBottom());
    }

    double totalLength = processor.getThumbnail().getTotalLength();

    if (totalLength > 0.0)
    {
        // 3. Render Waveform with glowing cyan gradient
        juce::ColourGradient waveGrad(juce::Colour(0xff00e5ff), waveformArea.getX(), waveformArea.getY(),
                                     juce::Colour(0xff0088cc), waveformArea.getX(), waveformArea.getBottom(), false);
        g.setGradientFill(waveGrad);

        processor.getThumbnail().drawChannels(g, waveformArea.toNearestInt(), 0.0, totalLength, 1.0f);

        // 4. Subtle center line
        g.setColour(juce::Colour(0x3300e5ff));
        g.drawHorizontalLine((int)waveformArea.getCentreY(), waveformArea.getX(), waveformArea.getRight());

        // 5. Draw active playhead cursor
        if (processor.isAudioPlaying())
        {
            float progress = processor.getPlaybackProgress();
            float playheadX = waveformArea.getX() + (waveformArea.getWidth() * progress);

            // Playhead glow
            g.setColour(juce::Colour(0x66ffffff));
            g.drawVerticalLine((int)playheadX, waveformArea.getY(), waveformArea.getBottom());

            g.setColour(juce::Colour(0xffff0055)); // Vibrant Playhead needle
            g.fillRect(playheadX - 1.0f, waveformArea.getY(), 2.0f, waveformArea.getHeight());
        }

        // 6. Dynamic Time and Format Badge (shows real duration and real sample rate)
        double srKhz = processor.getLoadedFileSampleRate() / 1000.0;
        juce::String badgeText = juce::String(totalLength, 1) + "s | PCM 24-bit | " + juce::String(srKhz, 1) + " kHz";

        g.setColour(juce::Colour(0x99ffffff));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(badgeText, waveformArea.removeFromTop(18.0f).removeFromRight(220.0f), juce::Justification::topRight);
    }
    else
    {
        // Empty placeholder state
        g.setColour(juce::Colour(0x44ffffff));
        g.setFont(juce::Font(13.0f, juce::Font::italic));
        g.drawText(juce::String::fromUTF8("La forma de onda de 10s aparecerá aquí tras la extracción..."), bounds, juce::Justification::centred);
    }
}

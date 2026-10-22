#include "WaveformComponent.h"

WaveformComponent::WaveformComponent(ClipSampler10AudioProcessor& processorToUse, int slotIndexToUse, const juce::String& titleText)
    : processor(processorToUse),
      slotIndex((slotIndexToUse == 2) ? 2 : 1),
      viewerTitle(titleText)
{
    processor.getThumbnail(slotIndex).addChangeListener(this);
}

WaveformComponent::~WaveformComponent()
{
    processor.getThumbnail(slotIndex).removeChangeListener(this);
}

void WaveformComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &processor.getThumbnail(slotIndex))
        repaint();
}

void WaveformComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown())
    {
        auto slotFile = processor.getSlotAudioFile(slotIndex);
        if (slotFile.existsAsFile())
        {
            processor.setActiveSlot(slotIndex);
            if (auto* parent = getParentComponent())
                parent->repaint();
            else
                repaint();
        }
    }
}

void WaveformComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging)
        return;

    auto slotFile = processor.getSlotAudioFile(slotIndex);
    if (!slotFile.existsAsFile())
        return;

    if (event.getDistanceFromDragStart() > 6)
    {
        auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this);
        if (container != nullptr)
        {
            processor.setActiveSlot(slotIndex);
            isDragging = true;
            juce::StringArray files;
            files.add(slotFile.getFullPathName());

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
    const bool isActiveSlot = (processor.getActiveSlot() == slotIndex);
    auto& thumb = processor.getThumbnail(slotIndex);
    const double totalLength = thumb.getTotalLength();

    // 1. Sleek Glassmorphism / Dark Obsidian container
    g.setColour(juce::Colour(0xff12141a));
    g.fillRoundedRectangle(bounds, 8.0f);

    juce::Colour accentCol = (slotIndex == 2) ? juce::Colour(0xff00e676) : juce::Colour(0xff00e5ff);
    juce::Colour borderCol = (isActiveSlot && totalLength > 0.0) ? accentCol.withAlpha(0.85f) : juce::Colour(0xff252b3b);
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds.reduced(0.75f), 8.0f, isActiveSlot ? 1.8f : 1.0f);

    auto headerBar = bounds.removeFromTop(22.0f).reduced(10.0f, 2.0f);
    g.setColour(accentCol);
    g.setFont(juce::Font(11.5f, juce::Font::bold));
    g.drawText(viewerTitle + (isActiveSlot && totalLength > 0.0 ? juce::String::fromUTF8("  [● ACTIVO EN DAW / MIDI]") : ""),
               headerBar.removeFromLeft(headerBar.getWidth() * 0.65f),
               juce::Justification::centredLeft);

    auto waveformArea = bounds.reduced(8.0f, 6.0f);

    // 2. Grid lines for quarter-duration divisions
    g.setColour(juce::Colour(0xff1a2130));
    for (int i = 1; i < 4; ++i)
    {
        float x = waveformArea.getX() + (waveformArea.getWidth() * (i / 4.0f));
        g.drawVerticalLine((int)x, waveformArea.getY(), waveformArea.getBottom());
    }

    if (totalLength > 0.0)
    {
        // 3. Render Waveform (Slot 1 = Cyan, Slot 2 = Neon Emerald/Purple for Derechos AI)
        juce::Colour topWave = (slotIndex == 2) ? juce::Colour(0xff00e676) : juce::Colour(0xff00e5ff);
        juce::Colour botWave = (slotIndex == 2) ? juce::Colour(0xff7c4dff) : juce::Colour(0xff0088cc);

        juce::ColourGradient waveGrad(topWave, waveformArea.getX(), waveformArea.getY(),
                                     botWave, waveformArea.getX(), waveformArea.getBottom(), false);
        g.setGradientFill(waveGrad);

        thumb.drawChannels(g, waveformArea.toNearestInt(), 0.0, totalLength, 1.0f);

        // 4. Subtle center line
        g.setColour(topWave.withAlpha(0.22f));
        g.drawHorizontalLine((int)waveformArea.getCentreY(), waveformArea.getX(), waveformArea.getRight());

        // 5. Draw active playhead cursor if this slot is currently playing
        if (isActiveSlot && processor.isAudioPlaying())
        {
            float progress = processor.getPlaybackProgress();
            float playheadX = waveformArea.getX() + (waveformArea.getWidth() * progress);

            g.setColour(juce::Colour(0x66ffffff));
            g.drawVerticalLine((int)playheadX, waveformArea.getY(), waveformArea.getBottom());

            g.setColour(juce::Colour(0xffff0055));
            g.fillRect(playheadX - 1.0f, waveformArea.getY(), 2.0f, waveformArea.getHeight());
        }

        // 6. Dynamic Time and Format Badge in top-right of viewer
        double srKhz = processor.getLoadedFileSampleRate() / 1000.0;
        juce::String badgeText = juce::String(totalLength, 1) + "s | 24-bit | " + juce::String(srKhz, 1) + " kHz";
        if (slotIndex == 2)
        {
            int st = processor.getLastAppliedSemitones();
            badgeText = ((st > 0) ? ("+" + juce::String(st)) : juce::String(st)) + " st IA | " + badgeText;
        }

        g.setColour(juce::Colour(0xccffffff));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(badgeText, headerBar, juce::Justification::centredRight);
    }
    else
    {
        g.setColour(juce::Colour(0x55ffffff));
        g.setFont(juce::Font(12.5f, juce::Font::italic));
        juce::String placeholder = (slotIndex == 1)
            ? juce::String::fromUTF8("1. Aquí aparecerá el audio original tras pulsar Extraer 3s, 10s o 30s...")
            : juce::String::fromUTF8("2. Aquí aparecerá el audio modificado al pulsar el botón '🛡️ Derechos (±3 st IA)'...");
        g.drawText(placeholder, waveformArea, juce::Justification::centred);
    }
}

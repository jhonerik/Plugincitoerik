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

            // If Auto-Chop (MPC Mode) is enabled, clicking on a slice auditions that slice!
            if (processor.isAutoChopEnabled())
            {
                auto slices = processor.getChopSlices(slotIndex);
                if (!slices.empty())
                {
                    auto waveBounds = getLocalBounds().toFloat();
                    waveBounds.removeFromTop(22.0f);
                    waveBounds = waveBounds.reduced(8.0f, 4.0f);

                    if (waveBounds.getWidth() > 10.0f)
                    {
                        float clickNorm = juce::jlimit(0.0f, 0.9999f, (event.position.x - waveBounds.getX()) / waveBounds.getWidth());
                        for (size_t i = 0; i < slices.size(); ++i)
                        {
                            if (clickNorm >= slices[i].startNorm && clickNorm <= slices[i].endNorm)
                            {
                                processor.triggerChopSlice((int)i);
                                processor.setStatusMessage(juce::String::fromUTF8("🥁 Auto-Chop MPC [Audio ")
                                                           + juce::String(slotIndex) + "]: Disparando slice "
                                                           + slices[i].noteName + " (Nota MIDI " + juce::String(slices[i].midiNote) + ")");
                                break;
                            }
                        }
                    }
                }
            }

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

    if (event.getDistanceFromDragStart() > 7)
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

    // 1. Cyber-Studio Glassmorphic Obsidian Container
    juce::ColourGradient cardGrad(juce::Colour(0xff0b0e18), bounds.getX(), bounds.getY(),
                                  juce::Colour(0xff111624), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(cardGrad);
    g.fillRoundedRectangle(bounds, 9.0f);

    juce::Colour accentCol = (slotIndex == 2) ? juce::Colour(0xff00e676) : juce::Colour(0xff00e5ff);
    if (isActiveSlot && totalLength > 0.0)
    {
        g.setColour(accentCol.withAlpha(0.14f));
        g.drawRoundedRectangle(bounds.reduced(1.5f), 9.0f, 3.5f);
    }

    juce::Colour borderCol = (isActiveSlot && totalLength > 0.0) ? accentCol.withAlpha(0.92f) : juce::Colour(0xff232b3e);
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds.reduced(0.75f), 9.0f, isActiveSlot ? 1.8f : 1.0f);

    auto headerBar = bounds.removeFromTop(22.0f).reduced(10.0f, 1.0f);
    g.setColour(accentCol);
    g.setFont(juce::Font(11.5f, juce::Font::bold));
    g.drawText(viewerTitle + (isActiveSlot && totalLength > 0.0 ? juce::String::fromUTF8("  [● ACTIVO EN FX / MIDI]") : ""),
               headerBar.removeFromLeft(headerBar.getWidth() * 0.50f),
               juce::Justification::centredLeft);

    auto waveformArea = bounds.reduced(8.0f, 4.0f);

    if (totalLength > 0.0)
    {
        const bool chopMode = processor.isAutoChopEnabled();
        auto slices = processor.getChopSlices(slotIndex);
        const int playingSlice = isActiveSlot && processor.isAudioPlaying() ? processor.getActivePlayingSlice() : -1;

        // If Auto-Chop MPC Mode is active, reserve top 18px strip inside waveform for "| C1 | C#1 | D1 | D#1 | E1 | F1 | F#1 |"
        juce::Rectangle<float> mpcNoteBar;
        if (chopMode && !slices.empty())
        {
            mpcNoteBar = waveformArea.removeFromTop(18.0f);
            g.setColour(juce::Colour(0xff151a28));
            g.fillRect(mpcNoteBar);
            g.setColour(juce::Colour(0xff2a334a));
            g.drawHorizontalLine((int)mpcNoteBar.getBottom(), mpcNoteBar.getX(), mpcNoteBar.getRight());
        }

        // Highlight active playing MPC slice background
        if (chopMode && playingSlice >= 0 && playingSlice < (int)slices.size())
        {
            const auto& activeS = slices[(size_t)playingSlice];
            float sx = waveformArea.getX() + activeS.startNorm * waveformArea.getWidth();
            float sw = (activeS.endNorm - activeS.startNorm) * waveformArea.getWidth();
            g.setColour(juce::Colour(0x38ffab00));
            g.fillRect(sx, mpcNoteBar.getY(), sw, waveformArea.getBottom() - mpcNoteBar.getY());
        }

        // 2. Render Waveform (Slot 1 = Electric Cyan/Blue, Slot 2 = Neon Emerald/Violet for Derechos AI)
        juce::Colour topWave = (slotIndex == 2) ? juce::Colour(0xff00e676) : juce::Colour(0xff00f2fe);
        juce::Colour botWave = (slotIndex == 2) ? juce::Colour(0xff7c4dff) : juce::Colour(0xff0072ff);

        // Ambient glow behind waveform
        g.setColour(topWave.withAlpha(0.10f));
        thumb.drawChannels(g, waveformArea.expanded(0.0f, 1.5f).toNearestInt(), 0.0, totalLength, 1.0f);

        juce::ColourGradient waveGrad(topWave, waveformArea.getX(), waveformArea.getY(),
                                     botWave, waveformArea.getX(), waveformArea.getBottom(), false);
        g.setGradientFill(waveGrad);

        thumb.drawChannels(g, waveformArea.toNearestInt(), 0.0, totalLength, 1.0f);

        // 3. Subtle center line
        g.setColour(topWave.withAlpha(0.24f));
        g.drawHorizontalLine((int)waveformArea.getCentreY(), waveformArea.getX(), waveformArea.getRight());

        // 4. Draw MPC Auto-Chop Slice Divider Lines & MIDI Note Headers (C1 .. G2)
        if (chopMode && !slices.empty())
        {
            g.setFont(juce::Font(10.5f, juce::Font::bold));
            for (size_t i = 0; i < slices.size(); ++i)
            {
                const auto& s = slices[i];
                float xStart = waveformArea.getX() + s.startNorm * waveformArea.getWidth();
                float xEnd   = waveformArea.getX() + s.endNorm * waveformArea.getWidth();
                float sliceW = xEnd - xStart;

                // Vertical slice divider line
                if (i > 0)
                {
                    g.setColour(juce::Colour(0xd8ffab00));
                    const float dashLengths[2] = { 4.0f, 3.0f };
                    g.drawDashedLine(juce::Line<float>(xStart, mpcNoteBar.getY(), xStart, waveformArea.getBottom()),
                                     dashLengths, 2, 1.3f);
                }

                // Note label in MPC header strip ("C1", "C#1", "D1", etc.)
                juce::Rectangle<float> padBox(xStart, mpcNoteBar.getY(), sliceW, mpcNoteBar.getHeight());
                bool isThisSlicePlaying = ((int)i == playingSlice);
                g.setColour(isThisSlicePlaying ? juce::Colour(0xffffab00) : juce::Colour(0xffe0e6ed));
                if (sliceW >= 20.0f)
                {
                    g.drawText(s.noteName, padBox.toNearestInt(), juce::Justification::centred, false);
                }
            }
        }
        else
        {
            // Standard quarter-duration grid lines when Auto-Chop is off
            g.setColour(juce::Colour(0xff192030));
            for (int i = 1; i < 4; ++i)
            {
                float x = waveformArea.getX() + (waveformArea.getWidth() * (i / 4.0f));
                g.drawVerticalLine((int)x, waveformArea.getY(), waveformArea.getBottom());
            }
        }

        // 5. Draw active playhead cursor if this slot is currently playing
        if (isActiveSlot && processor.isAudioPlaying())
        {
            float progress = processor.getPlaybackProgress();
            float playheadX = waveformArea.getX() + (waveformArea.getWidth() * progress);

            g.setColour(juce::Colour(0x44ff0055));
            g.fillRect(playheadX - 3.5f, waveformArea.getY(), 7.0f, waveformArea.getHeight());

            g.setColour(juce::Colour(0xffff1744));
            g.fillRect(playheadX - 1.2f, waveformArea.getY(), 2.4f, waveformArea.getHeight());

            juce::Path playCap;
            playCap.addTriangle(playheadX - 5.0f, waveformArea.getY(),
                                playheadX + 5.0f, waveformArea.getY(),
                                playheadX, waveformArea.getY() + 6.0f);
            g.setColour(juce::Colours::white);
            g.fillPath(playCap);
        }

        // 6. Dynamic Platform + BPM + Musical Key (Tonalidad) + Duration Badge in top-right of viewer
        double bpm = processor.getDetectedBpm(slotIndex);
        juce::String key = processor.getDetectedKey(slotIndex);
        juce::String platBadge = AudioExtractorWorker::getPlatformName(processor.getSelectedPlatform()).toUpperCase();
        juce::String badgeText = "[" + platBadge + "]  "
                               + juce::String::fromUTF8("🥁 ") + juce::String(bpm, 1) + " BPM | "
                               + juce::String::fromUTF8("🎹 Key: ") + key + " | "
                               + juce::String(totalLength, 1) + "s";
        if (slotIndex == 2)
        {
            int st = processor.getLastAppliedSemitones();
            badgeText = ((st > 0) ? ("+" + juce::String(st)) : juce::String(st)) + " st IA | " + badgeText;
        }

        g.setColour(juce::Colour(0xf0ffffff));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(badgeText, headerBar, juce::Justification::centredRight);
    }
    else
    {
        g.setColour(juce::Colour(0x6698a6bd));
        g.setFont(juce::Font(12.0f, juce::Font::italic));
        juce::String placeholder = (slotIndex == 1)
            ? juce::String::fromUTF8("1. Aquí aparecerá el Audio 1 (YouTube / TikTok / Instagram) con Auto-Chop MPC (C1-G2), BPM y Key...")
            : juce::String::fromUTF8("2. Aquí aparecerá el Audio 2 modificado al pulsar '🛡️ Derechos (±3 st IA)'...");
        g.drawText(placeholder, waveformArea, juce::Justification::centred);
    }
}

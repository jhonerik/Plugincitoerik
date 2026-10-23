#include "WaveformComponent.h"

// ==============================================================================
// Constructor y Destructor del Visor de Onda
// ==============================================================================
WaveformComponent::WaveformComponent(ClipSamplerAudioProcessor& processorToUse, int slotIndexToUse, const juce::String& titleText)
    : processor(processorToUse),
      slotIndex((slotIndexToUse == 2) ? 2 : 1),
      viewerTitle(titleText)
{
    // Nos suscribimos a las notificaciones del generador de miniaturas de onda (AudioThumbnail)
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

// ==============================================================================
// Interacción con el Ratón: Audición de Rodajas MPC y Selección de Ranura Activa
// ==============================================================================
void WaveformComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown())
    {
        auto slotFile = processor.getSlotAudioFile(slotIndex);
        if (slotFile.existsAsFile())
        {
            // Activamos esta ranura (Audio 1 o Audio 2) para efectos, reproducción y exportación MIDI/WAV
            processor.setActiveSlot(slotIndex);

            // Si el modo Rebanador MPC está activo, reproducimos inmediatamente la rodaja donde se hizo clic
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
                                processor.setStatusMessage(juce::String::fromUTF8("Rebanador MPC [Audio ")
                                                           + juce::String(slotIndex) + "]: Reproduciendo rodaja "
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

// ==============================================================================
// Arrastrar y Soltar (Drag & Drop) del Archivo .WAV Directamente desde la Onda
// ==============================================================================
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

// ==============================================================================
// Renderizado Gráfico Adaptable (Responsive) con Estética Azul Zafiro de Estudio
// ==============================================================================
void WaveformComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const bool isActiveSlot = (processor.getActiveSlot() == slotIndex);
    auto& thumb = processor.getThumbnail(slotIndex);
    const double totalLength = thumb.getTotalLength();

    // 1. Fondo elegante Azul Zafiro Acústico (evita el fondo negro plano)
    juce::ColourGradient cardGrad(juce::Colour(0xff162238), bounds.getX(), bounds.getY(),
                                  juce::Colour(0xff1e2f4d), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(cardGrad);
    g.fillRoundedRectangle(bounds, 9.0f);

    // Cuadrícula acústica sutil de fondo estilo osciloscopio de estudio
    g.setColour(juce::Colour(0x12ffffff));
    for (float gx = bounds.getX() + 40.0f; gx < bounds.getRight(); gx += 40.0f)
        g.drawVerticalLine((int)gx, bounds.getY() + 22.0f, bounds.getBottom() - 2.0f);

    juce::Colour accentCol = (slotIndex == 2) ? juce::Colour(0xff00e676) : juce::Colour(0xff38bdf8);
    if (isActiveSlot && totalLength > 0.0)
    {
        g.setColour(accentCol.withAlpha(0.18f));
        g.drawRoundedRectangle(bounds.reduced(1.5f), 9.0f, 3.2f);
    }

    juce::Colour borderCol = (isActiveSlot && totalLength > 0.0) ? accentCol.withAlpha(0.92f) : juce::Colour(0xff364b70);
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds.reduced(0.75f), 9.0f, isActiveSlot ? 1.8f : 1.1f);

    // Barra superior de título adaptable (usa drawFittedText para no cortar texto cuando la ventana es pequeña)
    const float headerH = juce::jlimit(18.0f, 24.0f, bounds.getHeight() * 0.22f);
    auto headerBar = bounds.removeFromTop(headerH).reduced(10.0f, 1.0f);

    g.setColour(accentCol.brighter(0.15f));
    g.setFont(juce::Font(juce::jlimit(10.0f, 12.5f, headerH * 0.54f), juce::Font::bold));

    juce::String fullTitle = viewerTitle + (isActiveSlot && totalLength > 0.0 ? juce::String::fromUTF8("  [ACTIVO EN EFECTOS / MIDI]") : "");
    auto leftHeaderRect = headerBar.removeFromLeft(headerBar.getWidth() * 0.52f).toNearestInt();
    g.drawFittedText(fullTitle, leftHeaderRect, juce::Justification::centredLeft, 1, 0.68f);

    auto waveformArea = bounds.reduced(8.0f, 4.0f);

    if (totalLength > 0.0)
    {
        const bool chopMode = processor.isAutoChopEnabled();
        auto slices = processor.getChopSlices(slotIndex);
        const int playingSlice = isActiveSlot && processor.isAudioPlaying() ? processor.getActivePlayingSlice() : -1;

        // 2. Franja superior de notas MIDI del Rebanador MPC ("| C1 | C#1 | D1 | D#1 | E1 | F1 | F#1 |")
        juce::Rectangle<float> mpcNoteBar;
        if (chopMode && !slices.empty())
        {
            const float barH = juce::jlimit(14.0f, 19.0f, waveformArea.getHeight() * 0.22f);
            mpcNoteBar = waveformArea.removeFromTop(barH);
            g.setColour(juce::Colour(0xff223354));
            g.fillRect(mpcNoteBar);
            g.setColour(juce::Colour(0xff3b527d));
            g.drawHorizontalLine((int)mpcNoteBar.getBottom(), mpcNoteBar.getX(), mpcNoteBar.getRight());
        }

        // Iluminar la rodaja MPC que se está reproduciendo actualmente
        if (chopMode && playingSlice >= 0 && playingSlice < (int)slices.size())
        {
            const auto& activeS = slices[(size_t)playingSlice];
            float sx = waveformArea.getX() + activeS.startNorm * waveformArea.getWidth();
            float sw = (activeS.endNorm - activeS.startNorm) * waveformArea.getWidth();
            g.setColour(juce::Colour(0x40ffab00));
            g.fillRect(sx, mpcNoteBar.getY(), sw, waveformArea.getBottom() - mpcNoteBar.getY());
        }

        // 3. Dibujo de la Forma de Onda con Resplandor y Gradiente Luminoso
        juce::Colour topWave = (slotIndex == 2) ? juce::Colour(0xff00e676) : juce::Colour(0xff38bdf8);
        juce::Colour botWave = (slotIndex == 2) ? juce::Colour(0xffa78bfa) : juce::Colour(0xff0284c7);

        g.setColour(topWave.withAlpha(0.12f));
        thumb.drawChannels(g, waveformArea.expanded(0.0f, 1.5f).toNearestInt(), 0.0, totalLength, 1.0f);

        juce::ColourGradient waveGrad(topWave, waveformArea.getX(), waveformArea.getY(),
                                     botWave, waveformArea.getX(), waveformArea.getBottom(), false);
        g.setGradientFill(waveGrad);
        thumb.drawChannels(g, waveformArea.toNearestInt(), 0.0, totalLength, 1.0f);

        // Línea central de referencia (0 dB / cruce por cero)
        g.setColour(topWave.withAlpha(0.28f));
        g.drawHorizontalLine((int)waveformArea.getCentreY(), waveformArea.getX(), waveformArea.getRight());

        // 4. Líneas divisorias verticales del Rebanador MPC y etiquetas de notas (C1 .. G2)
        if (chopMode && !slices.empty())
        {
            g.setFont(juce::Font(juce::jlimit(9.0f, 11.0f, mpcNoteBar.getHeight() * 0.62f), juce::Font::bold));
            for (size_t i = 0; i < slices.size(); ++i)
            {
                const auto& s = slices[i];
                float xStart = waveformArea.getX() + s.startNorm * waveformArea.getWidth();
                float xEnd   = waveformArea.getX() + s.endNorm * waveformArea.getWidth();
                float sliceW = xEnd - xStart;

                if (i > 0)
                {
                    g.setColour(juce::Colour(0xd8ffca28));
                    const float dashLengths[2] = { 4.0f, 3.0f };
                    g.drawDashedLine(juce::Line<float>(xStart, mpcNoteBar.getY(), xStart, waveformArea.getBottom()),
                                     dashLengths, 2, 1.3f);
                }

                juce::Rectangle<float> padBox(xStart, mpcNoteBar.getY(), sliceW, mpcNoteBar.getHeight());
                bool isThisSlicePlaying = ((int)i == playingSlice);
                g.setColour(isThisSlicePlaying ? juce::Colour(0xffffca28) : juce::Colour(0xffeef4ff));
                if (sliceW >= 16.0f)
                {
                    g.drawFittedText(s.noteName, padBox.toNearestInt(), juce::Justification::centred, 1, 0.7f);
                }
            }
        }

        // 5. Cursor de reproducción activo con halo luminoso
        if (isActiveSlot && processor.isAudioPlaying())
        {
            float progress = processor.getPlaybackProgress();
            float playheadX = waveformArea.getX() + (waveformArea.getWidth() * progress);

            g.setColour(juce::Colour(0x44ff2a55));
            g.fillRect(playheadX - 3.5f, waveformArea.getY(), 7.0f, waveformArea.getHeight());

            g.setColour(juce::Colour(0xffff2a55));
            g.fillRect(playheadX - 1.2f, waveformArea.getY(), 2.4f, waveformArea.getHeight());

            juce::Path playCap;
            playCap.addTriangle(playheadX - 5.0f, waveformArea.getY(),
                                playheadX + 5.0f, waveformArea.getY(),
                                playheadX, waveformArea.getY() + 6.0f);
            g.setColour(juce::Colours::white);
            g.fillPath(playCap);
        }

        // 6. Etiqueta derecha adaptable: Plataforma + BPM + Tono + Duración
        double bpm = processor.getDetectedBpm(slotIndex);
        juce::String key = processor.getDetectedKey(slotIndex);
        juce::String platBadge = AudioExtractorWorker::getPlatformName(processor.getSelectedPlatform()).toUpperCase();
        juce::String badgeText = "[" + platBadge + "]  "
                               + juce::String(bpm, 1) + " BPM | "
                               + juce::String::fromUTF8("Tono: ") + key + " | "
                               + juce::String(totalLength, 1) + "s";
        if (slotIndex == 2)
        {
            int st = processor.getLastAppliedSemitones();
            badgeText = ((st > 0) ? ("+" + juce::String(st)) : juce::String(st)) + " st IA | " + badgeText;
        }

        g.setColour(juce::Colour(0xf2ffffff));
        g.drawFittedText(badgeText, headerBar.toNearestInt(), juce::Justification::centredRight, 1, 0.68f);
    }
    else
    {
        // Texto guía adaptable cuando aún no se ha cargado audio en el visor
        g.setColour(juce::Colour(0xff9cb0cf));
        g.setFont(juce::Font(juce::jlimit(10.5f, 13.0f, waveformArea.getHeight() * 0.16f), juce::Font::italic));
        juce::String placeholder = (slotIndex == 1)
            ? juce::String::fromUTF8("1. Aquí aparecerá el Audio 1 (YouTube / TikTok / Instagram) con Rebanador MPC (C1-G2), BPM y Tono...")
            : juce::String::fromUTF8("2. Aquí aparecerá el Audio 2 modificado al pulsar '2. Derechos IA (±3 Semitonos)'...");
        g.drawFittedText(placeholder, waveformArea.reduced(12.0f, 4.0f).toNearestInt(), juce::Justification::centred, 2, 0.72f);
    }
}

#include "WaveformComponent.h"

// ==============================================================================
// Constructor y Destructor del Visor de Onda
// ==============================================================================
WaveformComponent::WaveformComponent(ClipSamplerAudioProcessor& processorToUse, int slotIndexToUse, const juce::String& titleText)
    : processor(processorToUse),
      slotIndex((slotIndexToUse == 2) ? 2 : 1),
      viewerTitle(titleText)
{
    setWantsKeyboardFocus(true);
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

bool WaveformComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".aiff" || ext == ".aif" || ext == ".flac" || ext == ".ogg")
            return true;
    }
    return false;
}

void WaveformComponent::fileDragEnter(const juce::StringArray&, int, int)
{
    isHoveringFileDrag = true;
    repaint();
}

void WaveformComponent::fileDragExit(const juce::StringArray&)
{
    isHoveringFileDrag = false;
    repaint();
}

void WaveformComponent::filesDropped(const juce::StringArray& files, int, int)
{
    isHoveringFileDrag = false;
    for (const auto& f : files)
    {
        juce::File file(f);
        auto ext = file.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".aiff" || ext == ".aif" || ext == ".flac" || ext == ".ogg")
        {
            if (processor.loadAudioFileIntoSlot(file, slotIndex))
            {
                processor.setStatusMessage(juce::String::fromUTF8("¡Audio local cargado en Visor ")
                                           + juce::String(slotIndex) + ": " + file.getFileName());
                repaint();
                if (auto* parent = getParentComponent())
                    parent->repaint();
            }
            break;
        }
    }
    repaint();
}

float WaveformComponent::getNormalizedXFromEvent(const juce::MouseEvent& event) const
{
    auto waveBounds = getLocalBounds().toFloat();
    const float headerH = juce::jlimit(18.0f, 24.0f, waveBounds.getHeight() * 0.22f);
    waveBounds.removeFromTop(headerH);
    waveBounds = waveBounds.reduced(8.0f, 4.0f);

    if (waveBounds.getWidth() <= 4.0f)
        return 0.0f;

    return juce::jlimit(0.0f, 0.9999f, (event.position.x - waveBounds.getX()) / waveBounds.getWidth());
}

int WaveformComponent::getDividerIndexNearX(float mouseX) const
{
    auto waveBounds = getLocalBounds().toFloat();
    const float headerH = juce::jlimit(18.0f, 24.0f, waveBounds.getHeight() * 0.22f);
    waveBounds.removeFromTop(headerH);
    auto waveformArea = waveBounds.reduced(8.0f, 4.0f);
    if (waveformArea.getWidth() <= 4.0f)
        return -1;

    auto slices = processor.getChopSlices(slotIndex);
    if (slices.size() < 2)
        return -1;

    for (size_t i = 1; i < slices.size(); ++i)
    {
        float lineX = waveformArea.getX() + slices[i].startNorm * waveformArea.getWidth();
        if (std::abs(mouseX - lineX) <= 6.5f)
            return (int)i;
    }
    return -1;
}

void WaveformComponent::mouseMove(const juce::MouseEvent& event)
{
    if (!processor.hasSlotAudio(slotIndex))
    {
        if (hoveredDividerIndex != -1)
        {
            hoveredDividerIndex = -1;
            repaint();
        }
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    int dIdx = getDividerIndexNearX(event.position.x);
    if (dIdx != hoveredDividerIndex)
    {
        hoveredDividerIndex = dIdx;
        repaint();
    }

    if (hoveredDividerIndex >= 1)
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void WaveformComponent::mouseExit(const juce::MouseEvent& /*event*/)
{
    if (hoveredDividerIndex != -1)
    {
        hoveredDividerIndex = -1;
        repaint();
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

bool WaveformComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        if (processor.isAudioPlaying())
            processor.stopPlayback();
        else
            processor.startPlayback();
        return true;
    }
    return false;
}

// ==============================================================================
// Interacción con el Ratón:
// - Arrastre sobre línea divisoria: ajusta la posición milimétrica del corte.
// - 1 Clic: Selecciona TODO el audio completo.
// - 2 Clics seguidos (Doble Clic): Selecciona el CORTE EXACTO (C1, C#1, D1...) bajo el cursor.
// ==============================================================================
void WaveformComponent::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();

    auto slotFile = processor.getSlotAudioFile(slotIndex);
    if (!slotFile.existsAsFile())
        return;

    // Clic Secundario / Derecho: Menú contextual rápido para Reverse y Half-Time de la rodaja bajo el cursor
    if (event.mods.isPopupMenu())
    {
        float clickNorm = getNormalizedXFromEvent(event);
        ChopSlice chosenSlice;
        if (processor.selectSliceAtNormalizedPosition(slotIndex, clickNorm, chosenSlice))
        {
            int sIdx = chosenSlice.sliceIndex;
            bool isRev = chosenSlice.isReversed;
            bool isHalf = chosenSlice.isHalfTime;

            juce::PopupMenu menu;
            menu.addItem(1, juce::String::fromUTF8("◀ Invertir Rodaja [") + chosenSlice.noteName + juce::String::fromUTF8("] (Reverse)"), true, isRev);
            menu.addItem(2, juce::String::fromUTF8("⏳ Media Velocidad [") + chosenSlice.noteName + juce::String::fromUTF8("] (Half-Time 0.5x)"), true, isHalf);

            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                [this, sIdx](int result) {
                    if (result == 1)
                    {
                        processor.toggleSliceReverse(slotIndex, sIdx);
                        repaint();
                        if (auto* parent = getParentComponent())
                            parent->repaint();
                    }
                    else if (result == 2)
                    {
                        processor.toggleSliceHalfTime(slotIndex, sIdx);
                        repaint();
                        if (auto* parent = getParentComponent())
                            parent->repaint();
                    }
                });
        }
        return;
    }

    if (!event.mods.isLeftButtonDown())
        return;

    // Si el usuario hace clic cerca de una línea divisoria entre rodajas, activamos el arrastre manual milimétrico
    int nearDivider = getDividerIndexNearX(event.position.x);
    if (nearDivider >= 1)
    {
        isDraggingDivider = true;
        draggedDividerIndex = nearDivider;
        processor.setActiveSlot(slotIndex);
        processor.setStatusMessage(juce::String::fromUTF8("Ajustando manualmente transiente / corte ")
                                   + juce::String(draggedDividerIndex) + juce::String::fromUTF8(" con precisión milimétrica..."));
        repaint();
        return;
    }

    // Si es el segundo clic de un doble clic, dejamos que mouseDoubleClick seleccione el corte exacto
    if (event.getNumberOfClicks() >= 2)
    {
        pendingSelectFullOnMouseUp = false;
        return;
    }

    processor.setActiveSlot(slotIndex);
    const int currentlySelectedSlice = processor.getSelectedSliceIndex(slotIndex);

    // Si ya había un corte exacto seleccionado (ej. C1), esperamos a mouseUp para saber si el usuario
    // está iniciando un arrastre (Drag & Drop) de ese corte hacia el DAW o si dio 1 clic para volver a seleccionar todo.
    if (currentlySelectedSlice >= 0)
    {
        pendingSelectFullOnMouseUp = true;
        return;
    }

    // 1 Clic normal: selecciona TODO el audio completo y lo reproduce desde el principio
    pendingSelectFullOnMouseUp = false;
    processor.selectFullAudioInSlot(slotIndex);
    processor.startPlayback();
    processor.setStatusMessage(juce::String::fromUTF8("Audio ") + juce::String(slotIndex)
                               + juce::String::fromUTF8(" completo seleccionado (1 clic). Da 2 clics seguidos sobre una nota para seleccionar ese corte, o arrastra las líneas divisorias para ajustar transientes."));

    if (auto* parent = getParentComponent())
        parent->repaint();
    else
        repaint();
}

void WaveformComponent::mouseUp(const juce::MouseEvent& event)
{
    if (isDraggingDivider)
    {
        isDraggingDivider = false;
        int finishedDivider = draggedDividerIndex;
        draggedDividerIndex = -1;
        processor.setStatusMessage(juce::String::fromUTF8("¡Línea divisoria ") + juce::String(finishedDivider)
                                   + juce::String::fromUTF8(" fijada con precisión milimétrica!"));
        repaint();
        return;
    }

    if (pendingSelectFullOnMouseUp && !isDragging && event.getNumberOfClicks() == 1)
    {
        pendingSelectFullOnMouseUp = false;
        processor.selectFullAudioInSlot(slotIndex);
        processor.startPlayback();
        processor.setStatusMessage(juce::String::fromUTF8("Audio ") + juce::String(slotIndex)
                                   + juce::String::fromUTF8(" completo seleccionado (1 clic). Da 2 clics seguidos sobre una nota para seleccionar ese corte."));

        if (auto* parent = getParentComponent())
            parent->repaint();
        else
            repaint();
    }
    pendingSelectFullOnMouseUp = false;
}

void WaveformComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (!event.mods.isLeftButtonDown())
        return;

    auto slotFile = processor.getSlotAudioFile(slotIndex);
    if (!slotFile.existsAsFile())
        return;

    pendingSelectFullOnMouseUp = false;
    const float clickNorm = getNormalizedXFromEvent(event);

    ChopSlice chosenSlice;
    if (processor.selectSliceAtNormalizedPosition(slotIndex, clickNorm, chosenSlice))
    {
        processor.setStatusMessage(juce::String::fromUTF8("✂ Corte Exacto seleccionado [Audio ")
                                   + juce::String(slotIndex) + " - Nota " + chosenSlice.noteName
                                   + juce::String::fromUTF8("]: Arrastra desde aquí o desde el botón '✂ Corte Exacto' hacia tu DAW (1 clic = seleccionar todo)."));
    }

    if (auto* parent = getParentComponent())
        parent->repaint();
    else
        repaint();
}

// ==============================================================================
// Arrastrar y Soltar (Drag & Drop) hacia el DAW:
// - Si hay un Corte Exacto seleccionado (doble clic), arrastra SOLO ese trozo .WAV.
// - Si está seleccionado todo el audio (1 clic), arrastra el archivo .WAV completo.
// ==============================================================================
void WaveformComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Si el usuario está arrastrando una línea divisoria de transiente, la movemos en tiempo real
    if (isDraggingDivider && draggedDividerIndex >= 1)
    {
        float normX = getNormalizedXFromEvent(event);
        if (processor.moveSliceDividerPosition(slotIndex, draggedDividerIndex, normX))
        {
            processor.setStatusMessage(juce::String::fromUTF8("Ajustando manualmente línea de corte ")
                                       + juce::String(draggedDividerIndex) + juce::String::fromUTF8(" con precisión milimétrica..."));
            repaint();
        }
        return;
    }

    if (isDragging)
        return;

    auto slotFile = processor.getSlotAudioFile(slotIndex);
    if (!slotFile.existsAsFile())
        return;

    if (event.getDistanceFromDragStart() > 6)
    {
        pendingSelectFullOnMouseUp = false;
        auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this);
        if (container != nullptr)
        {
            processor.setActiveSlot(slotIndex);

            const int selectedSliceIdx = processor.getSelectedSliceIndex(slotIndex);
            juce::File fileToDrag = (selectedSliceIdx >= 0)
                ? processor.getOrCreateSelectedSliceWavFile(slotIndex)
                : slotFile;

            if (!fileToDrag.existsAsFile())
                fileToDrag = slotFile;

            if (selectedSliceIdx >= 0)
            {
                juce::String sliceName = processor.getSelectedSliceName(slotIndex);
                processor.setStatusMessage(juce::String::fromUTF8("Arrastrando Corte Exacto [Nota ") + sliceName
                                           + juce::String::fromUTF8("] de Audio ") + juce::String(slotIndex) + " al DAW...");
            }
            else
            {
                processor.setStatusMessage(juce::String::fromUTF8("Arrastrando Audio ") + juce::String(slotIndex)
                                           + juce::String::fromUTF8(" completo al DAW..."));
            }

            isDragging = true;
            juce::StringArray files;
            files.add(fileToDrag.getFullPathName());

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
    const int selectedSliceIdx = processor.getSelectedSliceIndex(slotIndex);
    const juce::String selectedSliceName = processor.getSelectedSliceName(slotIndex);

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

    if (isHoveringFileDrag)
    {
        g.setColour(juce::Colour(0xff00e5ff));
        g.drawRoundedRectangle(bounds.reduced(1.5f), 9.0f, 2.5f);
    }

    // Barra superior de título adaptable (usa drawFittedText para no cortar texto cuando la ventana es pequeña)
    const float headerH = juce::jlimit(18.0f, 24.0f, bounds.getHeight() * 0.22f);
    auto headerBar = bounds.removeFromTop(headerH).reduced(10.0f, 1.0f);

    g.setColour(selectedSliceIdx >= 0 ? juce::Colour(0xffffd54f) : accentCol.brighter(0.15f));
    g.setFont(juce::Font(juce::jlimit(10.0f, 12.5f, headerH * 0.54f), juce::Font::bold));

    juce::String selectionBadge;
    if (totalLength > 0.0)
    {
        if (selectedSliceIdx >= 0 && selectedSliceName.isNotEmpty())
            selectionBadge = juce::String::fromUTF8("  [✂ CORTE EXACTO: ") + selectedSliceName + "]";
        else if (isActiveSlot)
            selectionBadge = juce::String::fromUTF8("  [TODO EL AUDIO | 2 CLICS = CORTE]");
    }

    juce::String baseTitle = viewerTitle;
    if (slotIndex == 2 && processor.hasSlotAudio(2))
        baseTitle = "VISOR 2: " + processor.getSlot2ContentTypeString();

    juce::String fullTitle = baseTitle + selectionBadge;
    auto leftHeaderRect = headerBar.removeFromLeft(headerBar.getWidth() * 0.56f).toNearestInt();
    g.drawFittedText(fullTitle, leftHeaderRect, juce::Justification::centredLeft, 1, 0.65f);

    auto waveformArea = bounds.reduced(8.0f, 4.0f);

    if (totalLength > 0.0)
    {
        const bool chopMode = processor.isAutoChopEnabled();
        auto slices = processor.getChopSlices(slotIndex);
        const int playingSlice = isActiveSlot && processor.isAudioPlaying() ? processor.getActivePlayingSlice() : -1;

        // 2. Franja superior de notas MIDI del Rebanador MPC ("| C1 | C#1 | D1 | D#1 | E1 | F1 | F#1 |")
        juce::Rectangle<float> mpcNoteBar;
        if ((chopMode || selectedSliceIdx >= 0) && !slices.empty())
        {
            const float barH = juce::jlimit(14.0f, 19.0f, waveformArea.getHeight() * 0.22f);
            mpcNoteBar = waveformArea.removeFromTop(barH);
            g.setColour(juce::Colour(0xff223354));
            g.fillRect(mpcNoteBar);
            g.setColour(juce::Colour(0xff3b527d));
            g.drawHorizontalLine((int)mpcNoteBar.getBottom(), mpcNoteBar.getX(), mpcNoteBar.getRight());
        }

        // Resaltar el Corte Exacto seleccionado con doble clic (o la rodaja que está sonando)
        if (!slices.empty())
        {
            if (selectedSliceIdx >= 0 && selectedSliceIdx < (int)slices.size())
            {
                const auto& selS = slices[(size_t)selectedSliceIdx];
                float sx = waveformArea.getX() + selS.startNorm * waveformArea.getWidth();
                float sw = (selS.endNorm - selS.startNorm) * waveformArea.getWidth();
                float topY = mpcNoteBar.isEmpty() ? waveformArea.getY() : mpcNoteBar.getY();

                // Atenuamos ligeramente el resto del audio para que destaque claramente el trozo seleccionado
                g.setColour(juce::Colour(0x55091220));
                if (sx > waveformArea.getX())
                    g.fillRect(waveformArea.getX(), waveformArea.getY(), sx - waveformArea.getX(), waveformArea.getHeight());
                if (sx + sw < waveformArea.getRight())
                    g.fillRect(sx + sw, waveformArea.getY(), waveformArea.getRight() - (sx + sw), waveformArea.getHeight());

                // Halo y borde brillante sobre la nota seleccionada
                g.setColour(juce::Colour(0x4400e5ff));
                g.fillRect(sx, topY, sw, waveformArea.getBottom() - topY);

                g.setColour(juce::Colour(0xffffd54f));
                g.drawRect(juce::Rectangle<float>(sx, topY, sw, waveformArea.getBottom() - topY), 2.0f);
            }
            else if (chopMode && playingSlice >= 0 && playingSlice < (int)slices.size())
            {
                const auto& activeS = slices[(size_t)playingSlice];
                float sx = waveformArea.getX() + activeS.startNorm * waveformArea.getWidth();
                float sw = (activeS.endNorm - activeS.startNorm) * waveformArea.getWidth();
                g.setColour(juce::Colour(0x40ffab00));
                g.fillRect(sx, mpcNoteBar.getY(), sw, waveformArea.getBottom() - mpcNoteBar.getY());
            }
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
        if ((chopMode || selectedSliceIdx >= 0) && !slices.empty())
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
                    const bool isDividerHighlighted = (hoveredDividerIndex == (int)i || draggedDividerIndex == (int)i);
                    if (isDividerHighlighted)
                    {
                        // Resplandor cian brillante al pasar el cursor o arrastrar la línea de corte
                        g.setColour(juce::Colour(0xff00e5ff));
                        g.drawLine(xStart, mpcNoteBar.getY(), xStart, waveformArea.getBottom(), 2.4f);

                        // Tirador / Rombo de ajuste en la barra superior de notas
                        g.setColour(juce::Colour(0xffffca28));
                        g.fillEllipse(xStart - 4.5f, mpcNoteBar.getBottom() - 4.5f, 9.0f, 9.0f);
                        g.setColour(juce::Colours::black);
                        g.drawEllipse(xStart - 4.5f, mpcNoteBar.getBottom() - 4.5f, 9.0f, 9.0f, 1.2f);
                    }
                    else
                    {
                        g.setColour(juce::Colour(0xd8ffca28));
                        const float dashLengths[2] = { 4.0f, 3.0f };
                        g.drawDashedLine(juce::Line<float>(xStart, mpcNoteBar.getY(), xStart, waveformArea.getBottom()),
                                         dashLengths, 2, 1.3f);
                    }
                }

                juce::Rectangle<float> padBox(xStart, mpcNoteBar.getY(), sliceW, mpcNoteBar.getHeight());
                const bool isThisSliceSelected = ((int)i == selectedSliceIdx);
                const bool isThisSlicePlaying = ((int)i == playingSlice);

                if (isThisSliceSelected)
                {
                    g.setColour(juce::Colour(0xffffca28));
                    g.fillRect(padBox.reduced(0.5f));
                    g.setColour(juce::Colours::black);
                }
                else
                {
                    g.setColour(isThisSlicePlaying ? juce::Colour(0xffffca28) : juce::Colour(0xffeef4ff));
                }

                if (sliceW >= 14.0f)
                {
                    juce::String labelTxt = isThisSliceSelected
                        ? (juce::String::fromUTF8("✂") + s.noteName)
                        : s.noteName;
                    if (s.isReversed)
                        labelTxt += juce::String::fromUTF8(" ◀");
                    if (s.isHalfTime)
                        labelTxt += juce::String::fromUTF8(" ½");
                    g.drawFittedText(labelTxt, padBox.toNearestInt(), juce::Justification::centred, 1, 0.68f);
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
            ? juce::String::fromUTF8("📂 SAMPLER LOCAL: Arrastra un archivo de audio (WAV, MP3, AIFF, FLAC) aquí\n"
                                     "o pulsa 'Cargar Audio Local' para procesar tus propios samples...\n"
                                     "(1 clic = Reproducir todo | 2 clics = Corte Exacto C1..G2 | Arrastrar al DAW)")
            : juce::String::fromUTF8("📂 Arrastra un segundo archivo de audio aquí, o pulsa 'Calor Analógico' / 'Alterar Tonos'...");
        g.drawFittedText(placeholder, waveformArea.reduced(12.0f, 4.0f).toNearestInt(), juce::Justification::centred, 2, 0.72f);
    }
}

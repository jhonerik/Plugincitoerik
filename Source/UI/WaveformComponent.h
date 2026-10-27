#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DSP/PluginProcessor.h"

/**
 * @class WaveformComponent
 * @brief Visor interactivo de forma de onda con soporte para el Rebanador MPC (Auto-Chop C1-G2)
 *        y Selección de Corte Exacto (1 clic = Todo el audio | Doble clic = Nota/Trozo bajo el cursor).
 *
 * Características:
 * - Visor 1: Muestra el audio original extraído desde YouTube, TikTok o Instagram (3s / 10s / 30s).
 * - Visor 2: Muestra el audio modificado por la función "Derechos IA" (±3 semitonos + dispersión de fase).
 * - 1 Clic sobre el visor selecciona el audio completo; 2 clics seguidos (doble clic) selecciona
 *   únicamente la nota/trozo donde está el cursor (C1, C#1, D1...) para arrastrar solo ese Corte Exacto al DAW.
 */
class WaveformComponent : public juce::Component,
                          public juce::ChangeListener,
                          public juce::FileDragAndDropTarget
{
public:
    WaveformComponent(ClipSamplerAudioProcessor& processorToUse, int slotIndexToUse, const juce::String& titleText);
    ~WaveformComponent() override;

    /** Soporte para arrastrar archivos WAV/MP3 directamente desde el disco duro o navegador del DAW. */
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    /** Dibuja el contenedor elegante azul zafiro, la onda, las divisiones MPC, el corte seleccionado y el cabezal. */
    void paint(juce::Graphics& g) override;

    /** Escucha cambios en el AudioThumbnail para redibujar automáticamente la onda al cargar un archivo. */
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    /** 1 Clic: selecciona todo el audio (o prepara el arrastre si se presiona sobre el corte ya seleccionado). */
    void mouseDown(const juce::MouseEvent& event) override;

    /** Confirma la selección de todo el audio cuando fue 1 solo clic sin arrastrar. */
    void mouseUp(const juce::MouseEvent& event) override;

    /** 2 Clics seguidos (Doble Clic): selecciona la nota/corte exacto (C1, C#1, D1...) donde está el cursor. */
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    /** Arrastra hacia el DAW el Corte Exacto seleccionado (si hay uno) o el archivo .WAV completo. */
    void mouseDrag(const juce::MouseEvent& event) override;

    /** Detecta cuando el cursor se aproxima a una línea divisoria para mostrar el cursor de ajuste (<->). */
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    /** Permite usar la barra espaciadora para reproducir/detener el audio. */
    bool keyPressed(const juce::KeyPress& key) override;

private:
    float getNormalizedXFromEvent(const juce::MouseEvent& event) const;
    int getDividerIndexNearX(float mouseX) const;

    ClipSamplerAudioProcessor& processor;
    int slotIndex = 1;
    juce::String viewerTitle;
    bool isDragging = false;
    bool pendingSelectFullOnMouseUp = false;
    int hoveredDividerIndex = -1;
    int draggedDividerIndex = -1;
    bool isDraggingDivider = false;
    bool isHoveringFileDrag = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

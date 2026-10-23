#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DSP/PluginProcessor.h"

/**
 * @class WaveformComponent
 * @brief Visor interactivo de forma de onda con soporte para el Rebanador MPC (Auto-Chop C1-G2).
 *
 * Características:
 * - Visor 1: Muestra el audio original extraído desde YouTube, TikTok o Instagram (3s / 10s / 30s).
 * - Visor 2: Muestra el audio modificado por la función "Derechos IA" (±3 semitonos + dispersión de fase).
 * - Dibuja las líneas divisorias de transientes y la cabecera de notas MIDI (C1 | C#1 | D1 | ... | G2).
 * - Muestra en tiempo real la plataforma de origen, el Tempo (BPM) y la Tonalidad Musical (Tono).
 * - Permite disparar cualquier rodaja haciendo clic sobre ella o arrastrar el archivo .WAV directamente al DAW.
 */
class WaveformComponent : public juce::Component,
                          public juce::ChangeListener
{
public:
    WaveformComponent(ClipSamplerAudioProcessor& processorToUse, int slotIndexToUse, const juce::String& titleText);
    ~WaveformComponent() override;

    /** Dibuja el contenedor elegante azul zafiro, la onda, las divisiones MPC y el cabezal de reproducción. */
    void paint(juce::Graphics& g) override;

    /** Escucha cambios en el AudioThumbnail para redibujar automáticamente la onda al cargar un archivo. */
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    /** Selecciona la ranura activa y dispara la rodaja MPC sobre la que hizo clic el usuario. */
    void mouseDown(const juce::MouseEvent& event) override;

    /** Inicia la operación de arrastrar y soltar (Drag & Drop) del archivo .WAV hacia el DAW. */
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    ClipSamplerAudioProcessor& processor;
    int slotIndex = 1;
    juce::String viewerTitle;
    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

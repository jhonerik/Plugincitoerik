#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/PluginProcessor.h"
#include "WaveformComponent.h"
#include "LicenseOverlayComponent.h"

/**
 * ClipSampler10AudioProcessorEditor
 * 
 * Modern two-panel user interface for ClipSampler10.
 * Decoupled from background threads: observes ClipSampler10AudioProcessor state safely
 * via SafePointer and timer synchronization so closing the window never crashes or aborts jobs.
 */
class ClipSampler10AudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::DragAndDropContainer,
                                          private juce::Timer
{
public:
    explicit ClipSampler10AudioProcessorEditor(ClipSampler10AudioProcessor&);
    ~ClipSampler10AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;

    // Action handlers
    void onPasteClicked();
    void onExtractClicked();
    void onPlayClicked();
    void onStopClicked();
    void onExportWavClicked();
    void onDragDawClicked();
    void onUpdateExtractorClicked();
    void onCleanCacheClicked();
    void onLicenseBadgeClicked();
    void updateLicenseUiState();

    ClipSampler10AudioProcessor& audioProcessor;
    int licenseCheckTickCounter = 0;

    // Header Maintenance & License Status Buttons
    juce::TextButton licenseBadgeButton;
    juce::TextButton updateExtractorButton { juce::String::fromUTF8("Actualizar yt-dlp (-U)") };
    juce::TextButton cleanCacheButton { juce::String::fromUTF8("Limpiar Caché") };

    // =========================================================================
    // Top Panel: Link & Capture Section
    // =========================================================================
    juce::GroupComponent topSectionGroup { "topGroup", juce::String::fromUTF8("1. CAPTURA Y ENLACE DE AUDIO") };
    
    juce::Label urlLabel { {}, juce::String::fromUTF8("URL de YouTube:") };
    juce::TextEditor urlEditor;
    juce::TextButton pasteButton { juce::String::fromUTF8("Pegar") };

    juce::Label startTimeLabel { {}, juce::String::fromUTF8("Inicio (MM:SS):") };
    juce::TextEditor startTimeEditor;
    juce::Slider startTimeSlider;

    juce::Label durationBadgeLabel;

    juce::TextButton extractButton { juce::String::fromUTF8("Extraer Muestra de 10s") };
    double currentProgressValue = 0.0;
    juce::ProgressBar progressBar { currentProgressValue };
    juce::Label statusLabel;
    juce::String lastDisplayedStatus;

    // =========================================================================
    // Bottom Panel: Waveform & Export Section
    // =========================================================================
    juce::GroupComponent bottomSectionGroup { "bottomGroup", juce::String::fromUTF8("2. VISOR DE AUDIO Y EXPORTACIÓN AL DAW") };
    
    WaveformComponent waveformComponent;

    juce::TextButton playButton { juce::String::fromUTF8("▶ Reproducir en DAW") };
    juce::TextButton stopButton { juce::String::fromUTF8("■ Detener") };
    juce::TextButton dragDawButton { juce::String::fromUTF8("⇲ Arrastrar al DAW (Timeline / Pista)") };
    juce::TextButton exportWavButton { juce::String::fromUTF8("Exportar .WAV...") };

    // FileChooser must be retained asynchronously
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Modal License & 3-Day Trial Lock Overlay
    LicenseOverlayComponent licenseOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSampler10AudioProcessorEditor)
};

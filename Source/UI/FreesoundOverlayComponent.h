#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DSP/PluginProcessor.h"
#include <vector>
#include <memory>
#include <atomic>

/**
 * @struct FreesoundItem
 * @brief Representa un archivo de sonido individual devuelto por la API de Freesound.org.
 */
struct FreesoundItem
{
    juce::String id;
    juce::String name;
    juce::String username;
    double duration = 0.0;
    juce::String license;
    juce::String previewHqMp3;
    juce::String previewLqMp3;
};

/**
 * @class FreesoundOverlayComponent
 * @brief Ventana modal / overlay integrada para buscar, preescuchar y cargar sonidos
 *        directamente desde la biblioteca abierta de Freesound.org mediante su API oficial v2.
 */
class FreesoundOverlayComponent : public juce::Component,
                                  private juce::ListBoxModel,
                                  private juce::ChangeListener
{
public:
    explicit FreesoundOverlayComponent(ClipSamplerAudioProcessor& processorToUse);
    ~FreesoundOverlayComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Actualiza la vista comprobando si existe una API Key guardada. */
    void refreshView();

private:
    // ListBoxModel callbacks para la lista de resultados
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;

    // Manejador de eventos de audio preview
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Métodos de gestión de API Key
    juce::File getApiKeyStorageFile() const;
    void loadStoredApiKey();
    void saveApiKey(const juce::String& key);
    void onConnectApiKeyClicked();
    void onPasteApiKeyClicked();
    void onChangeApiKeyClicked();
    void onOpenApiKeyWebClicked();

    // Búsqueda en segundo plano
    void triggerSearch();
    void performSearchAsync(const juce::String& query, const juce::String& filter, const juce::String& sort);
    void downloadAndLoadSample(const FreesoundItem& sound);
    void previewSound(const FreesoundItem& sound);
    void stopPreview();

    ClipSamplerAudioProcessor& processor;

    bool hasValidApiKey = false;
    juce::String currentApiKey;

    // =========================================================================
    // Estado A: Cuadro inicial "Pega tu API Key de Freesound aquí"
    // =========================================================================
    juce::Label introTitleLabel;
    juce::Label introSubtitleLabel;
    juce::Label apiKeyInputLabel;
    juce::TextEditor apiKeyEditor;
    juce::TextButton pasteKeyButton       { juce::String::fromUTF8("📋 Pegar") };
    juce::TextButton connectButton        { juce::String::fromUTF8("🔓 Conectar y Cargar Buscador") };
    juce::TextButton getApiKeyHelpButton  { juce::String::fromUTF8("🌐 ¿Cómo obtener una API Key gratuita? (freesound.org)") };
    juce::Label introStatusLabel;

    // =========================================================================
    // Estado B: Buscador con opciones de filtro y lista de resultados
    // =========================================================================
    juce::Label searchHeaderTitleLabel;
    juce::Label searchStatusBadge;
    juce::TextButton changeKeyButton      { juce::String::fromUTF8("🔑 Cambiar API Key") };
    juce::TextButton closeOverlayButton   { juce::String::fromUTF8("✕ Volver a ClipSampler") };

    // Barra de búsqueda
    juce::TextEditor searchBox;
    juce::TextButton searchButton         { juce::String::fromUTF8("🔍 Buscar Sonidos") };

    // Opciones de filtro
    juce::Label durationFilterLabel       { {}, juce::String::fromUTF8("Duración:") };
    juce::ComboBox durationFilterBox;
    juce::Label sortFilterLabel           { {}, juce::String::fromUTF8("Ordenar por:") };
    juce::ComboBox sortFilterBox;
    juce::Label licenseFilterLabel        { {}, juce::String::fromUTF8("Licencia:") };
    juce::ComboBox licenseFilterBox;

    // Lista de resultados
    juce::ListBox resultsListBox;
    std::vector<FreesoundItem> soundResults;
    juce::Label resultsFeedbackLabel;
    std::atomic<bool> isSearching { false };
    std::atomic<bool> isDownloading { false };

    // Preescucha de audio
    juce::AudioFormatManager previewFormatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> previewReaderSource;
    juce::AudioTransportSource previewTransport;
    juce::AudioSourcePlayer previewPlayer;
    juce::AudioDeviceManager previewDeviceManager;
    juce::String currentlyPreviewingId;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreesoundOverlayComponent)
};

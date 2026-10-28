#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DSP/PluginProcessor.h"
#include <vector>
#include <memory>
#include <atomic>

/**
 * @struct FreesoundItem
 * @brief Representa un archivo de sonido individual devuelto por la API de Freesound.org
 *        con descriptores de audio avanzados (BPM, formato, frecuencia de muestreo y tags).
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
    double bpm = 0.0;
    juce::String type;
    int samplerate = 0;
    int channels = 0;
    juce::StringArray tags;
};

/**
 * @class FreesoundOverlayComponent
 * @brief Ventana modal / overlay integrada para buscar, filtrar por formato/duración,
 *        preescuchar, explorar sonidos similares y cargar muestras directamente desde
 *        Freesound.org mediante su API oficial v2 canónica (/apiv2/search/).
 */
class FreesoundOverlayComponent : public juce::Component,
                                  private juce::ListBoxModel,
                                  private juce::Timer
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
    void timerCallback() override;

    // Métodos de gestión de API Key
    juce::File getApiKeyStorageFile() const;
    void loadStoredApiKey();
    void saveApiKey(const juce::String& key);
    void onConnectApiKeyClicked();
    void onPasteApiKeyClicked();
    void onChangeApiKeyClicked();
    void onOpenApiKeyWebClicked();

    // Búsqueda y descubrimiento de sonidos
    void triggerSearch(int page = 1);
    void performSearchAsync(const juce::String& query, const juce::String& filter, const juce::String& sort, int page);
    void searchSimilarSounds(const FreesoundItem& targetSound, int page = 1);
    void updatePaginationUi();

    void downloadAndLoadSample(const FreesoundItem& sound);
    void previewSound(const FreesoundItem& sound);
    void stopPreview();

    ClipSamplerAudioProcessor& processor;

    bool hasValidApiKey = false;
    juce::String currentApiKey;

    // Paginación y estado de búsqueda
    int currentPage = 1;
    int totalPages = 1;
    int totalResultCount = 0;
    bool isViewingSimilar = false;
    FreesoundItem similarTargetItem;
    juce::String lastExecutedQuery;
    juce::String lastExecutedFilter;
    juce::String lastExecutedSort;

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
    // Estado B: Buscador con opciones de filtro, lista y paginación
    // =========================================================================
    juce::Label searchHeaderTitleLabel;
    juce::Label searchStatusBadge;
    juce::TextButton exitSimilarButton    { juce::String::fromUTF8("✕ Volver a la Búsqueda") };
    juce::TextButton changeKeyButton      { juce::String::fromUTF8("🔑 Cambiar API Key") };
    juce::TextButton closeOverlayButton   { juce::String::fromUTF8("✕ Volver a ClipSampler") };

    // Barra de búsqueda
    juce::TextEditor searchBox;
    juce::TextButton searchButton         { juce::String::fromUTF8("🔍 Buscar Sonidos") };

    // Opciones de filtro
    juce::Label durationFilterLabel       { {}, juce::String::fromUTF8("Duración:") };
    juce::ComboBox durationFilterBox;
    juce::Label sortFilterLabel           { {}, juce::String::fromUTF8("Ordenar:") };
    juce::ComboBox sortFilterBox;
    juce::Label licenseFilterLabel        { {}, juce::String::fromUTF8("Licencia:") };
    juce::ComboBox licenseFilterBox;
    juce::Label typeFilterLabel           { {}, juce::String::fromUTF8("Formato:") };
    juce::ComboBox typeFilterBox;

    // Lista de resultados
    juce::ListBox resultsListBox;
    std::vector<FreesoundItem> soundResults;
    juce::Label resultsFeedbackLabel;
    std::atomic<bool> isSearching { false };
    std::atomic<bool> isDownloading { false };

    // Barra inferior de paginación
    juce::TextButton prevPageButton       { juce::String::fromUTF8("◀ Anterior") };
    juce::Label pageInfoLabel;
    juce::TextButton nextPageButton       { juce::String::fromUTF8("Siguiente ▶") };

    // Preescucha de audio
    // Estado de preescucha de audio enrutada al DAW
    enum class AudioOutputMode
    {
        Auto = 0,
        DawMixer = 1,
        DirectWindows = 2
    };

    AudioOutputMode currentAudioMode = AudioOutputMode::Auto;
    juce::TextButton audioRouteButton { juce::String::fromUTF8("Salida: Auto") };

    // Reproductor directo para Windows (DirectSound / WASAPI)
    std::unique_ptr<juce::AudioDeviceManager> directDeviceManager;
    std::unique_ptr<juce::AudioSourcePlayer> directSourcePlayer;
    std::unique_ptr<juce::AudioTransportSource> directTransportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> directReaderSource;
    juce::String directPlayingSoundId;

    void initDirectAudioIfNeeded();
    void startDirectPlayback(const juce::File& file, const juce::String& soundId);
    void stopDirectPlayback();
    bool isDirectPlaying() const;
    bool isCurrentSoundPlaying(const juce::String& soundId) const;

    bool lastKnownPreviewPlaying = false;
    juce::String lastKnownPreviewId;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreesoundOverlayComponent)
};

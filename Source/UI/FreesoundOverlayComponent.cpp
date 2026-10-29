#include "FreesoundOverlayComponent.h"

// ==============================================================================
// Componente de Fila Individual para cada Resultado de Sonido de Freesound
// ==============================================================================
class FreesoundItemRowComponent : public juce::Component
{
public:
    FreesoundItemRowComponent(std::function<void(const FreesoundItem&)> onPreviewCb,
                              std::function<void(const FreesoundItem&)> onLoadCb,
                              std::function<void(const FreesoundItem&)> onSimilarCb)
        : onPreview(std::move(onPreviewCb)),
          onLoad(std::move(onLoadCb)),
          onSimilar(std::move(onSimilarCb))
    {
        similarBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a1d96)); // Púrpura sofisticado
        similarBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe9d5ff));
        similarBtn.onClick = [this] { if (onSimilar) onSimilar(item); };
        addAndMakeVisible(similarBtn);

        previewBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e3a5f));
        previewBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7dd3fc));
        previewBtn.onClick = [this] { if (onPreview) onPreview(item); };
        addAndMakeVisible(previewBtn);

        loadBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0d9488)); // Teal brillante
        loadBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        loadBtn.onClick = [this] { if (onLoad) onLoad(item); };
        addAndMakeVisible(loadBtn);
    }

    void updateItem(const FreesoundItem& newItem, bool isCurrentlyPlaying)
    {
        item = newItem;
        previewBtn.setButtonText(isCurrentlyPlaying ? juce::String::fromUTF8("⏹ Detener") : juce::String::fromUTF8("▶ Escuchar"));
        previewBtn.setColour(juce::TextButton::buttonColourId, isCurrentlyPlaying ? juce::Colour(0xffdc2626) : juce::Colour(0xff1e3a5f));
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f, 2.0f);
        g.setColour(juce::Colour(0xff18243c));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(juce::Colour(0xff2a3f65));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        // Área reservada para textos a la izquierda de los 3 botones
        auto textArea = bounds.reduced(10.0f, 4.0f);
        auto leftArea = textArea.removeFromLeft(textArea.getWidth() - 315);

        // Línea 1: Nombre + Badges (BPM nativo + Formato original)
        auto topRow = leftArea.removeFromTop(leftArea.getHeight() * 0.52f);

        juce::String titleWithBadges = item.name;
        if (item.bpm > 10.0)
            titleWithBadges += "  [" + juce::String(juce::roundToInt(item.bpm)) + " BPM]";
        if (item.type.isNotEmpty())
            titleWithBadges += "  (" + item.type.toUpperCase() + ")";

        g.setColour(juce::Colour(0xfff8fafc));
        g.setFont(juce::Font(12.8f, juce::Font::bold));
        g.drawFittedText(titleWithBadges, topRow.toNearestInt(), juce::Justification::centredLeft, 1, 0.75f);

        // Línea 2: Subtítulo con metadata enriquecida (duración, usuario, licencia, specs y tags)
        juce::String licBadge = item.license.containsIgnoreCase("zero") ? "CC0" : "CC-BY";
        juce::String formatSpec;
        if (item.channels > 0)
            formatSpec += (item.channels == 2 ? "Stereo" : "Mono");
        if (item.samplerate > 0)
            formatSpec += " " + juce::String(item.samplerate / 1000) + "kHz";

        juce::String tagsStr;
        int maxTags = juce::jmin(3, item.tags.size());
        for (int i = 0; i < maxTags; ++i)
            tagsStr += " #" + item.tags[i];

        juce::String metaStr = juce::String(item.duration, 1) + "s  |  @" + item.username + "  |  " + licBadge;
        if (formatSpec.isNotEmpty())
            metaStr += "  |  " + formatSpec;
        if (tagsStr.isNotEmpty())
            metaStr += "  | " + tagsStr;

        g.setColour(juce::Colour(0xff94a3b8));
        g.setFont(juce::Font(10.8f, juce::Font::plain));
        g.drawFittedText(metaStr, leftArea.toNearestInt(), juce::Justification::centredLeft, 1, 0.70f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8, 6);
        loadBtn.setBounds(area.removeFromRight(120));
        area.removeFromRight(5);
        previewBtn.setBounds(area.removeFromRight(88));
        area.removeFromRight(5);
        similarBtn.setBounds(area.removeFromRight(85));
    }

private:
    FreesoundItem item;
    std::function<void(const FreesoundItem&)> onPreview;
    std::function<void(const FreesoundItem&)> onLoad;
    std::function<void(const FreesoundItem&)> onSimilar;
    juce::TextButton similarBtn { juce::String::fromUTF8("🔗 Similares") };
    juce::TextButton previewBtn { juce::String::fromUTF8("▶ Escuchar") };
    juce::TextButton loadBtn    { juce::String::fromUTF8("⚡ Cargar en Pad") };
};

// ==============================================================================
// FreesoundOverlayComponent
// ==============================================================================
FreesoundOverlayComponent::FreesoundOverlayComponent(ClipSamplerAudioProcessor& processorToUse)
    : processor(processorToUse)
{
    startTimerHz(15);

    // =========================================================================
    // Estado A: Cuadro inicial "Pega tu API Key de Freesound aquí"
    // =========================================================================
    introTitleLabel.setText(juce::String::fromUTF8("FREESOUND.ORG — INTEGRACIÓN DIRECTA DE MUESTRAS"), juce::dontSendNotification);
    introTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    introTitleLabel.setJustificationType(juce::Justification::centred);
    introTitleLabel.setFont(juce::Font(17.5f, juce::Font::bold));
    addAndMakeVisible(introTitleLabel);

    introSubtitleLabel.setText(juce::String::fromUTF8(
        "Accede a más de 500,000 sonidos, one-shots y loops libres de derechos (Creative Commons)\n"
        "directamente en ClipSampler sin salir de tu DAW. Para comenzar, introduce tu clave de API gratuita."),
        juce::dontSendNotification);
    introSubtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    introSubtitleLabel.setJustificationType(juce::Justification::centred);
    introSubtitleLabel.setFont(juce::Font(12.5f, juce::Font::plain));
    addAndMakeVisible(introSubtitleLabel);

    apiKeyInputLabel.setText(juce::String::fromUTF8("Pega tu API Key de Freesound aquí:"), juce::dontSendNotification);
    apiKeyInputLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff8fafc));
    apiKeyInputLabel.setFont(juce::Font(13.5f, juce::Font::bold));
    addAndMakeVisible(apiKeyInputLabel);

    apiKeyEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff152036));
    apiKeyEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    apiKeyEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3b527d));
    apiKeyEditor.setTextToShowWhenEmpty(juce::String::fromUTF8("Ejemplo: a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0"), juce::Colour(0xff64748b));
    apiKeyEditor.onReturnKey = [this] { onConnectApiKeyClicked(); };
    addAndMakeVisible(apiKeyEditor);

    pasteKeyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff30446e));
    pasteKeyButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pasteKeyButton.onClick = [this] { onPasteApiKeyClicked(); };
    addAndMakeVisible(pasteKeyButton);

    connectButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0d9488));
    connectButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    connectButton.onClick = [this] { onConnectApiKeyClicked(); };
    addAndMakeVisible(connectButton);

    getApiKeyHelpButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e293b));
    getApiKeyHelpButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff38bdf8));
    getApiKeyHelpButton.onClick = [this] { onOpenApiKeyWebClicked(); };
    addAndMakeVisible(getApiKeyHelpButton);

    introStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff59e0b));
    introStatusLabel.setJustificationType(juce::Justification::centred);
    introStatusLabel.setFont(juce::Font(12.0f, juce::Font::italic));
    addAndMakeVisible(introStatusLabel);

    // =========================================================================
    // Estado B: Buscador de Sonidos, Filtros y Paginación
    // =========================================================================
    searchHeaderTitleLabel.setText(juce::String::fromUTF8("🎵 EXPLORADOR DE SAMPLES FREESOUND (API v2)"), juce::dontSendNotification);
    searchHeaderTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    searchHeaderTitleLabel.setFont(juce::Font(15.5f, juce::Font::bold));
    addChildComponent(searchHeaderTitleLabel);

    searchStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
    searchStatusBadge.setFont(juce::Font(11.5f, juce::Font::bold));
    addChildComponent(searchStatusBadge);

    exitSimilarButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff581c87));
    exitSimilarButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff3e8ff));
    exitSimilarButton.onClick = [this] {
        isViewingSimilar = false;
        exitSimilarButton.setVisible(false);
        triggerSearch(1);
    };
    addChildComponent(exitSimilarButton);

    changeKeyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff334155));
    changeKeyButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
    changeKeyButton.onClick = [this] { onChangeApiKeyClicked(); };
    addChildComponent(changeKeyButton);

    audioRouteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e3a5f));
    audioRouteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7dd3fc));
    audioRouteButton.setTooltip(juce::String::fromUTF8("Haz clic para alternar: Modo Auto (reproduce en DAW o altavoces segÃºn actividad), Solo DAW o Solo Directo"));
    audioRouteButton.onClick = [this] {
        if (currentAudioMode == AudioOutputMode::Auto)
        {
            currentAudioMode = AudioOutputMode::DawMixer;
            audioRouteButton.setButtonText(juce::String::fromUTF8("ðŸŽ›ï¸ Salida: Solo DAW"));
        }
        else if (currentAudioMode == AudioOutputMode::DawMixer)
        {
            currentAudioMode = AudioOutputMode::DirectWindows;
            audioRouteButton.setButtonText(juce::String::fromUTF8("ðŸ”ˆ Salida: Solo Directo"));
        }
        else
        {
            currentAudioMode = AudioOutputMode::Auto;
            audioRouteButton.setButtonText(juce::String::fromUTF8("ðŸ”Š Salida: Auto"));
        }
    };
    addChildComponent(audioRouteButton);

    closeOverlayButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff475569));
    closeOverlayButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    closeOverlayButton.onClick = [this] {
        stopPreview();
        setVisible(false);
    };
    addChildComponent(closeOverlayButton);

    // Barra de búsqueda
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff152036));
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3b527d));
    searchBox.setTextToShowWhenEmpty(juce::String::fromUTF8("Escribe un término (ej. kick, 808, snare, vinyl loop, vocal, piano, riser)..."), juce::Colour(0xff64748b));
    searchBox.onReturnKey = [this] { triggerSearch(1); };
    addChildComponent(searchBox);

    searchButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0284c7));
    searchButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    searchButton.onClick = [this] { triggerSearch(1); };
    addChildComponent(searchButton);

    // Filtros: Duración, Orden, Licencia, Formato
    durationFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    durationFilterLabel.setFont(juce::Font(11.2f, juce::Font::bold));
    addChildComponent(durationFilterLabel);

    durationFilterBox.addItem(juce::String::fromUTF8("Cualquier Duración"), 1);
    durationFilterBox.addItem(juce::String::fromUTF8("One-Shots (< 2s)"), 2);
    durationFilterBox.addItem(juce::String::fromUTF8("Loops Cortos (2s - 6s)"), 3);
    durationFilterBox.addItem(juce::String::fromUTF8("Frases (6s - 15s)"), 4);
    durationFilterBox.addItem(juce::String::fromUTF8("Muestras Largas (> 15s)"), 5);
    durationFilterBox.setSelectedId(1, juce::dontSendNotification);
    durationFilterBox.onChange = [this] { triggerSearch(1); };
    addChildComponent(durationFilterBox);

    sortFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    sortFilterLabel.setFont(juce::Font(11.2f, juce::Font::bold));
    addChildComponent(sortFilterLabel);

    sortFilterBox.addItem(juce::String::fromUTF8("Relevancia (Score)"), 1);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Descargados"), 2);
    sortFilterBox.addItem(juce::String::fromUTF8("Mejor Calificados"), 3);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Recientes"), 4);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Cortos"), 5);
    sortFilterBox.setSelectedId(1, juce::dontSendNotification);
    sortFilterBox.onChange = [this] { triggerSearch(1); };
    addChildComponent(sortFilterBox);

    licenseFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    licenseFilterLabel.setFont(juce::Font(11.2f, juce::Font::bold));
    addChildComponent(licenseFilterLabel);

    licenseFilterBox.addItem(juce::String::fromUTF8("Todas las Licencias"), 1);
    licenseFilterBox.addItem(juce::String::fromUTF8("Creative Commons 0 (CC0)"), 2);
    licenseFilterBox.addItem(juce::String::fromUTF8("Attribution (CC-BY)"), 3);
    licenseFilterBox.setSelectedId(1, juce::dontSendNotification);
    licenseFilterBox.onChange = [this] { triggerSearch(1); };
    addChildComponent(licenseFilterBox);

    typeFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    typeFilterLabel.setFont(juce::Font(11.2f, juce::Font::bold));
    addChildComponent(typeFilterLabel);

    typeFilterBox.addItem(juce::String::fromUTF8("Cualquier Formato"), 1);
    typeFilterBox.addItem(juce::String::fromUTF8("Solo WAV"), 2);
    typeFilterBox.addItem(juce::String::fromUTF8("Solo FLAC"), 3);
    typeFilterBox.addItem(juce::String::fromUTF8("WAV o FLAC"), 4);
    typeFilterBox.addItem(juce::String::fromUTF8("Solo AIFF"), 5);
    typeFilterBox.setSelectedId(1, juce::dontSendNotification);
    typeFilterBox.onChange = [this] { triggerSearch(1); };
    addChildComponent(typeFilterBox);

    // ListBox de resultados
    resultsListBox.setModel(this);
    resultsListBox.setRowHeight(58);
    resultsListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff131c2e));
    resultsListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff2a3f65));
    addChildComponent(resultsListBox);

    resultsFeedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    resultsFeedbackLabel.setJustificationType(juce::Justification::centred);
    resultsFeedbackLabel.setFont(juce::Font(13.0f, juce::Font::italic));
    resultsFeedbackLabel.setText(juce::String::fromUTF8("Ingresa un término arriba y pulsa 'Buscar Sonidos'."), juce::dontSendNotification);
    addChildComponent(resultsFeedbackLabel);

    // Barra de Paginación Inferior
    prevPageButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e293b));
    prevPageButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff94a3b8));
    prevPageButton.onClick = [this] {
        if (currentPage > 1)
        {
            if (isViewingSimilar)
                searchSimilarSounds(similarTargetItem, currentPage - 1);
            else
                performSearchAsync(lastExecutedQuery, lastExecutedFilter, lastExecutedSort, currentPage - 1);
        }
    };
    addChildComponent(prevPageButton);

    nextPageButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e293b));
    nextPageButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff94a3b8));
    nextPageButton.onClick = [this] {
        if (currentPage < totalPages)
        {
            if (isViewingSimilar)
                searchSimilarSounds(similarTargetItem, currentPage + 1);
            else
                performSearchAsync(lastExecutedQuery, lastExecutedFilter, lastExecutedSort, currentPage + 1);
        }
    };
    addChildComponent(nextPageButton);

    pageInfoLabel.setJustificationType(juce::Justification::centred);
    pageInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    pageInfoLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    addChildComponent(pageInfoLabel);

    loadStoredApiKey();
}

FreesoundOverlayComponent::~FreesoundOverlayComponent()
{
    stopTimer();
    stopPreview();
    if (directDeviceManager != nullptr)
    {
        if (directSourcePlayer != nullptr)
        {
            directSourcePlayer->setSource(nullptr);
            directDeviceManager->removeAudioCallback(directSourcePlayer.get());
        }
        directReaderSource.reset();
        directTransportSource.reset();
        directSourcePlayer.reset();
        directDeviceManager.reset();
    }
}

void FreesoundOverlayComponent::initDirectAudioIfNeeded()
{
    if (directDeviceManager == nullptr)
    {
        directDeviceManager = std::make_unique<juce::AudioDeviceManager>();
        directDeviceManager->initialiseWithDefaultDevices(0, 2);

        directTransportSource = std::make_unique<juce::AudioTransportSource>();
        directSourcePlayer = std::make_unique<juce::AudioSourcePlayer>();
        directSourcePlayer->setSource(directTransportSource.get());
        directDeviceManager->addAudioCallback(directSourcePlayer.get());
    }
}

void FreesoundOverlayComponent::startDirectPlayback(const juce::File& file, const juce::String& soundId)
{
    initDirectAudioIfNeeded();
    if (directTransportSource == nullptr)
        return;

    directTransportSource->stop();
    directTransportSource->setSource(nullptr);
    directReaderSource.reset();

    auto* reader = processor.getFormatManager().createReaderFor(file);
    if (reader != nullptr)
    {
        directReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        directTransportSource->setSource(directReaderSource.get(), 0, nullptr, reader->sampleRate);
        directTransportSource->setPosition(0.0);
        directTransportSource->start();
        directPlayingSoundId = soundId;
    }
}

void FreesoundOverlayComponent::stopDirectPlayback()
{
    if (directTransportSource != nullptr)
    {
        directTransportSource->stop();
        directTransportSource->setSource(nullptr);
    }
    directReaderSource.reset();
    directPlayingSoundId.clear();
}

bool FreesoundOverlayComponent::isDirectPlaying() const
{
    return directTransportSource != nullptr && directTransportSource->isPlaying();
}

bool FreesoundOverlayComponent::isCurrentSoundPlaying(const juce::String& soundId) const
{
    if (processor.isFreesoundPreviewPlaying() && processor.getActivePreviewSoundId() == soundId)
        return true;
    if (isDirectPlaying() && directPlayingSoundId == soundId)
        return true;
    return false;
}

void FreesoundOverlayComponent::timerCallback()
{
    const bool isPlaying = processor.isFreesoundPreviewPlaying() || isDirectPlaying();
    const juce::String activeId = processor.isFreesoundPreviewPlaying() ? processor.getActivePreviewSoundId() : directPlayingSoundId;
    if (isPlaying != lastKnownPreviewPlaying || activeId != lastKnownPreviewId)
    {
        lastKnownPreviewPlaying = isPlaying;
        lastKnownPreviewId = activeId;
        resultsListBox.repaint();
    }
}

juce::File FreesoundOverlayComponent::getApiKeyStorageFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ClipSampler")
        .getChildFile("freesound_api_key.txt");
}

void FreesoundOverlayComponent::loadStoredApiKey()
{
    auto file = getApiKeyStorageFile();
    if (file.existsAsFile())
    {
        currentApiKey = file.loadFileAsString().trim();
        hasValidApiKey = currentApiKey.isNotEmpty();
    }
    else
    {
        currentApiKey.clear();
        hasValidApiKey = false;
    }
    refreshView();
}

void FreesoundOverlayComponent::saveApiKey(const juce::String& key)
{
    currentApiKey = key.trim();
    auto file = getApiKeyStorageFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(currentApiKey);
    hasValidApiKey = currentApiKey.isNotEmpty();
    refreshView();
}

void FreesoundOverlayComponent::refreshView()
{
    const bool showSearch = hasValidApiKey;

    // Componentes del Estado A (Pide clave)
    introTitleLabel.setVisible(!showSearch);
    introSubtitleLabel.setVisible(!showSearch);
    apiKeyInputLabel.setVisible(!showSearch);
    apiKeyEditor.setVisible(!showSearch);
    pasteKeyButton.setVisible(!showSearch);
    connectButton.setVisible(!showSearch);
    getApiKeyHelpButton.setVisible(!showSearch);
    introStatusLabel.setVisible(!showSearch);

    // Componentes del Estado B (Buscador, Filtros y Paginación)
    searchHeaderTitleLabel.setVisible(showSearch);
    searchStatusBadge.setVisible(showSearch);
    exitSimilarButton.setVisible(showSearch && isViewingSimilar);
    changeKeyButton.setVisible(showSearch);
    audioRouteButton.setVisible(showSearch);
    closeOverlayButton.setVisible(true);
    searchBox.setVisible(showSearch);
    searchButton.setVisible(showSearch);
    durationFilterLabel.setVisible(showSearch);
    durationFilterBox.setVisible(showSearch);
    sortFilterLabel.setVisible(showSearch);
    sortFilterBox.setVisible(showSearch);
    licenseFilterLabel.setVisible(showSearch);
    licenseFilterBox.setVisible(showSearch);
    typeFilterLabel.setVisible(showSearch);
    typeFilterBox.setVisible(showSearch);
    resultsListBox.setVisible(showSearch);
    resultsFeedbackLabel.setVisible(showSearch && soundResults.empty());
    prevPageButton.setVisible(showSearch);
    nextPageButton.setVisible(showSearch);
    pageInfoLabel.setVisible(showSearch);

    if (showSearch)
    {
        juce::String masked = currentApiKey.length() > 8
            ? (currentApiKey.substring(0, 4) + "..." + currentApiKey.substring(currentApiKey.length() - 4))
            : currentApiKey;
        searchStatusBadge.setText(juce::String::fromUTF8("● Conectado (Token: ") + masked + ")", juce::dontSendNotification);
    }

    resized();
    repaint();
}

void FreesoundOverlayComponent::onPasteApiKeyClicked()
{
    juce::String clip = juce::SystemClipboard::getTextFromClipboard().trim();
    if (clip.isNotEmpty())
    {
        apiKeyEditor.setText(clip, false);
        onConnectApiKeyClicked();
    }
}

void FreesoundOverlayComponent::onConnectApiKeyClicked()
{
    juce::String key = apiKeyEditor.getText().trim();
    if (key.length() < 10)
    {
        introStatusLabel.setText(juce::String::fromUTF8("Por favor ingresa un token válido de Freesound (mínimo 10 caracteres)."), juce::dontSendNotification);
        return;
    }

    saveApiKey(key);
    triggerSearch(1); // Búsqueda inicial automática
}

void FreesoundOverlayComponent::onChangeApiKeyClicked()
{
    hasValidApiKey = false;
    apiKeyEditor.setText(currentApiKey, false);
    refreshView();
}

void FreesoundOverlayComponent::onOpenApiKeyWebClicked()
{
    juce::URL("https://freesound.org/apiv2/apply/").launchInDefaultBrowser();
}

// ==============================================================================
// Búsqueda Asíncrona en Freesound API v2 (Endpoint Canónico /apiv2/search/)
// ==============================================================================
void FreesoundOverlayComponent::triggerSearch(int page)
{
    if (!hasValidApiKey)
        return;

    stopPreview();
    isViewingSimilar = false;
    exitSimilarButton.setVisible(false);

    juce::String query = searchBox.getText().trim();
    if (query.isEmpty())
        query = "sound"; // Término por defecto para explorar

    // Construcción de filtros Solr combinados
    juce::String filter;
    int durId = durationFilterBox.getSelectedId();
    if (durId == 2) filter = "duration:[0.0 TO 2.0]";
    else if (durId == 3) filter = "duration:[2.0 TO 6.0]";
    else if (durId == 4) filter = "duration:[6.0 TO 15.0]";
    else if (durId == 5) filter = "duration:[15.0 TO 300.0]";

    int licId = licenseFilterBox.getSelectedId();
    if (licId == 2)
    {
        if (filter.isNotEmpty()) filter += " ";
        filter += "license:\"Creative Commons 0\"";
    }
    else if (licId == 3)
    {
        if (filter.isNotEmpty()) filter += " ";
        filter += "license:\"Attribution\"";
    }

    int typeId = typeFilterBox.getSelectedId();
    if (typeId == 2)
    {
        if (filter.isNotEmpty()) filter += " ";
        filter += "type:wav";
    }
    else if (typeId == 3)
    {
        if (filter.isNotEmpty()) filter += " ";
        filter += "type:flac";
    }
    else if (typeId == 4)
    {
        if (filter.isNotEmpty()) filter += " ";
        filter += "(type:wav OR type:flac)";
    }
    else if (typeId == 5)
    {
        if (filter.isNotEmpty()) filter += " ";
        filter += "type:aiff";
    }

    // Ordenamiento
    juce::String sort = "score";
    int sortId = sortFilterBox.getSelectedId();
    if (sortId == 2) sort = "downloads_desc";
    else if (sortId == 3) sort = "rating_desc";
    else if (sortId == 4) sort = "created_desc";
    else if (sortId == 5) sort = "duration_asc";

    resultsFeedbackLabel.setText(juce::String::fromUTF8("Conectando con Freesound API v2 (/search/)..."), juce::dontSendNotification);
    resultsFeedbackLabel.setVisible(true);

    performSearchAsync(query, filter, sort, page);
}

void FreesoundOverlayComponent::performSearchAsync(const juce::String& query, const juce::String& filter, const juce::String& sort, int page)
{
    isSearching.store(true);
    lastExecutedQuery = query;
    lastExecutedFilter = filter;
    lastExecutedSort = sort;

    auto safeKey = currentApiKey;
    std::thread([this, safeKey, query, filter, sort, page]() {
        // Endpoint canónico de Freesound API v2 con descriptores de audio avanzados
        juce::String endpoint = "https://freesound.org/apiv2/search/?token=" + juce::URL::addEscapeChars(safeKey, true)
                              + "&query=" + juce::URL::addEscapeChars(query, true)
                              + "&fields=id,name,username,duration,license,previews,bpm,type,samplerate,channels,tags"
                              + "&page_size=20"
                              + "&page=" + juce::String(page);

        if (filter.isNotEmpty())
            endpoint += "&filter=" + juce::URL::addEscapeChars(filter, true);
        if (sort.isNotEmpty())
            endpoint += "&sort=" + juce::URL::addEscapeChars(sort, true);

        juce::URL url(endpoint);
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                .withConnectionTimeoutMs(10000)
                                                .withNumRedirectsToFollow(5));

        std::vector<FreesoundItem> newResults;
        juce::String errorMsg;
        int parsedTotalCount = 0;

        if (stream != nullptr)
        {
            juce::String jsonText = stream->readEntireStreamAsString();
            auto parsedJson = juce::JSON::parse(jsonText);

            if (auto* rootObj = parsedJson.getDynamicObject())
            {
                if (rootObj->hasProperty("detail"))
                {
                    errorMsg = "Freesound API: " + rootObj->getProperty("detail").toString();
                }
                else
                {
                    parsedTotalCount = static_cast<int>(rootObj->getProperty("count"));
                    if (auto* resArr = rootObj->getProperty("results").getArray())
                    {
                        for (const auto& itemVar : *resArr)
                        {
                            if (auto* itemObj = itemVar.getDynamicObject())
                            {
                                FreesoundItem item;
                                item.id = itemObj->getProperty("id").toString();
                                item.name = itemObj->getProperty("name").toString();
                                item.username = itemObj->getProperty("username").toString();
                                item.duration = static_cast<double>(itemObj->getProperty("duration"));
                                item.license = itemObj->getProperty("license").toString();

                                if (itemObj->hasProperty("bpm"))
                                    item.bpm = static_cast<double>(itemObj->getProperty("bpm"));
                                if (itemObj->hasProperty("type"))
                                    item.type = itemObj->getProperty("type").toString();
                                if (itemObj->hasProperty("samplerate"))
                                    item.samplerate = static_cast<int>(itemObj->getProperty("samplerate"));
                                if (itemObj->hasProperty("channels"))
                                    item.channels = static_cast<int>(itemObj->getProperty("channels"));

                                if (auto* tagsArr = itemObj->getProperty("tags").getArray())
                                {
                                    for (const auto& tagVar : *tagsArr)
                                        item.tags.add(tagVar.toString());
                                }

                                if (auto* prevObj = itemObj->getProperty("previews").getDynamicObject())
                                {
                                    item.previewHqMp3 = prevObj->getProperty("preview-hq-mp3").toString();
                                    item.previewLqMp3 = prevObj->getProperty("preview-lq-mp3").toString();
                                }
                                newResults.push_back(item);
                            }
                        }
                    }
                }
            }
            else
            {
                errorMsg = juce::String::fromUTF8("Respuesta no válida del servidor de Freesound.");
            }
        }
        else
        {
            errorMsg = juce::String::fromUTF8("No se pudo conectar con freesound.org. Verifica tu conexión a internet o tu API Key.");
        }

        juce::MessageManager::callAsync([this, newResults, errorMsg, parsedTotalCount, page]() {
            isSearching.store(false);
            soundResults = newResults;
            currentPage = page;
            totalResultCount = parsedTotalCount;
            totalPages = juce::jmax(1, (parsedTotalCount + 19) / 20);

            updatePaginationUi();
            resultsListBox.updateContent();
            resultsListBox.repaint();

            if (errorMsg.isNotEmpty())
            {
                resultsFeedbackLabel.setText(errorMsg, juce::dontSendNotification);
                resultsFeedbackLabel.setVisible(true);
            }
            else if (soundResults.empty())
            {
                resultsFeedbackLabel.setText(juce::String::fromUTF8("No se encontraron resultados para los filtros seleccionados."), juce::dontSendNotification);
                resultsFeedbackLabel.setVisible(true);
            }
            else
            {
                resultsFeedbackLabel.setVisible(false);
            }
        });
    }).detach();
}

// ==============================================================================
// Búsqueda de Sonidos Acústicamente Similares (/apiv2/sounds/<id>/similar/)
// ==============================================================================
void FreesoundOverlayComponent::searchSimilarSounds(const FreesoundItem& targetSound, int page)
{
    if (!hasValidApiKey)
        return;

    stopPreview();
    isViewingSimilar = true;
    similarTargetItem = targetSound;
    exitSimilarButton.setVisible(true);

    resultsFeedbackLabel.setText(juce::String::fromUTF8("Buscando sonidos similares a '") + targetSound.name + "'...", juce::dontSendNotification);
    resultsFeedbackLabel.setVisible(true);

    isSearching.store(true);
    auto safeKey = currentApiKey;

    std::thread([this, safeKey, targetSound, page]() {
        juce::String endpoint = "https://freesound.org/apiv2/sounds/" + targetSound.id + "/similar/?token=" + juce::URL::addEscapeChars(safeKey, true)
                              + "&fields=id,name,username,duration,license,previews,bpm,type,samplerate,channels,tags"
                              + "&page_size=20"
                              + "&page=" + juce::String(page);

        juce::URL url(endpoint);
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                .withConnectionTimeoutMs(10000)
                                                .withNumRedirectsToFollow(5));

        std::vector<FreesoundItem> newResults;
        juce::String errorMsg;
        int parsedTotalCount = 0;

        if (stream != nullptr)
        {
            juce::String jsonText = stream->readEntireStreamAsString();
            auto parsedJson = juce::JSON::parse(jsonText);

            if (auto* rootObj = parsedJson.getDynamicObject())
            {
                if (rootObj->hasProperty("detail"))
                {
                    errorMsg = "Freesound API: " + rootObj->getProperty("detail").toString();
                }
                else
                {
                    parsedTotalCount = static_cast<int>(rootObj->getProperty("count"));
                    if (auto* resArr = rootObj->getProperty("results").getArray())
                    {
                        for (const auto& itemVar : *resArr)
                        {
                            if (auto* itemObj = itemVar.getDynamicObject())
                            {
                                FreesoundItem item;
                                item.id = itemObj->getProperty("id").toString();
                                item.name = itemObj->getProperty("name").toString();
                                item.username = itemObj->getProperty("username").toString();
                                item.duration = static_cast<double>(itemObj->getProperty("duration"));
                                item.license = itemObj->getProperty("license").toString();

                                if (itemObj->hasProperty("bpm"))
                                    item.bpm = static_cast<double>(itemObj->getProperty("bpm"));
                                if (itemObj->hasProperty("type"))
                                    item.type = itemObj->getProperty("type").toString();
                                if (itemObj->hasProperty("samplerate"))
                                    item.samplerate = static_cast<int>(itemObj->getProperty("samplerate"));
                                if (itemObj->hasProperty("channels"))
                                    item.channels = static_cast<int>(itemObj->getProperty("channels"));

                                if (auto* tagsArr = itemObj->getProperty("tags").getArray())
                                {
                                    for (const auto& tagVar : *tagsArr)
                                        item.tags.add(tagVar.toString());
                                }

                                if (auto* prevObj = itemObj->getProperty("previews").getDynamicObject())
                                {
                                    item.previewHqMp3 = prevObj->getProperty("preview-hq-mp3").toString();
                                    item.previewLqMp3 = prevObj->getProperty("preview-lq-mp3").toString();
                                }
                                newResults.push_back(item);
                            }
                        }
                    }
                }
            }
            else
            {
                errorMsg = juce::String::fromUTF8("Respuesta no válida al buscar sonidos similares.");
            }
        }
        else
        {
            errorMsg = juce::String::fromUTF8("Error de conexión al buscar sonidos similares.");
        }

        juce::MessageManager::callAsync([this, newResults, errorMsg, parsedTotalCount, page]() {
            isSearching.store(false);
            soundResults = newResults;
            currentPage = page;
            totalResultCount = parsedTotalCount;
            totalPages = juce::jmax(1, (parsedTotalCount + 19) / 20);

            updatePaginationUi();
            resultsListBox.updateContent();
            resultsListBox.repaint();

            if (errorMsg.isNotEmpty())
            {
                resultsFeedbackLabel.setText(errorMsg, juce::dontSendNotification);
                resultsFeedbackLabel.setVisible(true);
            }
            else if (soundResults.empty())
            {
                resultsFeedbackLabel.setText(juce::String::fromUTF8("No se encontraron sonidos similares en Freesound."), juce::dontSendNotification);
                resultsFeedbackLabel.setVisible(true);
            }
            else
            {
                resultsFeedbackLabel.setVisible(false);
            }
        });
    }).detach();
}

void FreesoundOverlayComponent::updatePaginationUi()
{
    prevPageButton.setEnabled(currentPage > 1);
    nextPageButton.setEnabled(currentPage < totalPages);

    if (isViewingSimilar)
    {
        exitSimilarButton.setVisible(true);
        pageInfoLabel.setText(juce::String::fromUTF8("🔗 Similares a '") + similarTargetItem.name.substring(0, 22) + "...'  |  Pág. "
                              + juce::String(currentPage) + " de " + juce::String(totalPages) + " (" + juce::String(totalResultCount) + " sonidos)",
                              juce::dontSendNotification);
    }
    else
    {
        exitSimilarButton.setVisible(false);
        pageInfoLabel.setText(juce::String::fromUTF8("Página ") + juce::String(currentPage) + " de " + juce::String(totalPages)
                              + "  (" + juce::String(totalResultCount) + juce::String::fromUTF8(" sonidos encontrados)"),
                              juce::dontSendNotification);
    }
}

// ==============================================================================
// Preescucha de Audio y Descarga al Plugin
// ==============================================================================
void FreesoundOverlayComponent::previewSound(const FreesoundItem& sound)
{
    if (isCurrentSoundPlaying(sound.id))
    {
        stopPreview();
        return;
    }

    stopPreview();

    juce::String previewUrl = sound.previewHqMp3.isNotEmpty() ? sound.previewHqMp3 : sound.previewLqMp3;
    if (previewUrl.isEmpty())
        return;

    auto cacheDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ClipSampler").getChildFile("AudioCache");
    cacheDir.createDirectory();
    auto tempFile = cacheDir.getChildFile("preview_" + sound.id + ".mp3");

    auto executePlayback = [this, tempFile, sound]() {
        if (currentAudioMode == AudioOutputMode::DirectWindows)
        {
            startDirectPlayback(tempFile, sound.id);
        }
        else if (currentAudioMode == AudioOutputMode::DawMixer)
        {
            processor.startFreesoundPreview(tempFile, sound.id);
        }
        else
        {
            processor.startFreesoundPreview(tempFile, sound.id);
            if (!processor.isDawActivelyProcessing())
            {
                startDirectPlayback(tempFile, sound.id);
            }
        }
        resultsListBox.repaint();
    };

    bool fileValidInCache = false;
    if (tempFile.existsAsFile() && tempFile.getSize() > 2048)
    {
        std::unique_ptr<juce::AudioFormatReader> testReader(processor.getFormatManager().createReaderFor(tempFile));
        if (testReader != nullptr && testReader->lengthInSamples > 0)
            fileValidInCache = true;
        else
            tempFile.deleteFile();
    }

    if (fileValidInCache)
    {
        executePlayback();
        return;
    }

    std::thread([this, previewUrl, tempFile, sound, executePlayback]() {
        juce::URL url(previewUrl);
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                .withConnectionTimeoutMs(10000)
                                                .withNumRedirectsToFollow(5)
                                                .withExtraHeaders("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) ClipSampler/2.6.1\r\n"));
        if (stream != nullptr)
        {
            tempFile.deleteFile();
            {
                juce::FileOutputStream out(tempFile);
                if (out.openedOk())
                {
                    out.writeFromInputStream(*stream, -1);
                    out.flush();
                }
            }

            juce::MessageManager::callAsync([this, tempFile, sound, executePlayback]() {
                if (tempFile.existsAsFile() && tempFile.getSize() > 2048)
                {
                    executePlayback();
                }
            });
        }
    }).detach();
}

void FreesoundOverlayComponent::stopPreview()
{
    processor.stopFreesoundPreview();
    stopDirectPlayback();
    resultsListBox.repaint();
}
void FreesoundOverlayComponent::downloadAndLoadSample(const FreesoundItem& sound)
{
    stopPreview();

    juce::String downloadUrl = sound.previewHqMp3.isNotEmpty() ? sound.previewHqMp3 : sound.previewLqMp3;
    if (downloadUrl.isEmpty())
        return;

    auto cacheDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ClipSampler").getChildFile("AudioCache");
    cacheDir.createDirectory();

    juce::String cleanName = juce::File::createLegalFileName(sound.name);
    if (!cleanName.endsWithIgnoreCase(".mp3") && !cleanName.endsWithIgnoreCase(".wav"))
        cleanName += ".mp3";

    auto targetFile = cacheDir.getChildFile("freesound_" + sound.id + "_" + cleanName);

    resultsFeedbackLabel.setText(juce::String::fromUTF8("Descargando sample y cargando en pads de ClipSampler..."), juce::dontSendNotification);
    resultsFeedbackLabel.setVisible(true);

    std::thread([this, downloadUrl, targetFile, sound]() {
        juce::URL url(downloadUrl);
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(15000));
        if (stream != nullptr)
        {
            targetFile.deleteFile();
            {
                juce::FileOutputStream out(targetFile);
                if (out.openedOk())
                {
                    out.writeFromInputStream(*stream, -1);
                    out.flush();
                }
            }

            juce::MessageManager::callAsync([this, targetFile, sound]() {
                if (targetFile.existsAsFile() && targetFile.getSize() > 1024)
                {
                    processor.loadAudioFileIntoSlot(targetFile, 1);
                    processor.setStatusMessage(juce::String::fromUTF8("¡Sample de Freesound cargado!: ") + sound.name);
                    setVisible(false); // Volvemos a ClipSampler con el sample listo para rebanar y tocar
                }
                else
                {
                    resultsFeedbackLabel.setText(juce::String::fromUTF8("Error al guardar el archivo de Freesound."), juce::dontSendNotification);
                    resultsFeedbackLabel.setVisible(true);
                }
            });
        }
        else
        {
            juce::MessageManager::callAsync([this]() {
                resultsFeedbackLabel.setText(juce::String::fromUTF8("No se pudo descargar el sample de Freesound."), juce::dontSendNotification);
                resultsFeedbackLabel.setVisible(true);
            });
        }
    }).detach();
}

// ==============================================================================
// ListBoxModel Implementation
// ==============================================================================
int FreesoundOverlayComponent::getNumRows()
{
    return static_cast<int>(soundResults.size());
}

void FreesoundOverlayComponent::paintListBoxItem(int, juce::Graphics&, int, int, bool)
{
    // Las filas son componentes interactivos mediante refreshComponentForRow
}

juce::Component* FreesoundOverlayComponent::refreshComponentForRow(int rowNumber, bool, juce::Component* existingComponentToUpdate)
{
    auto* rowComp = dynamic_cast<FreesoundItemRowComponent*>(existingComponentToUpdate);
    if (rowComp == nullptr)
    {
        rowComp = new FreesoundItemRowComponent(
            [this](const FreesoundItem& item) { previewSound(item); },
            [this](const FreesoundItem& item) { downloadAndLoadSample(item); },
            [this](const FreesoundItem& item) { searchSimilarSounds(item, 1); }
        );
    }

    if (rowNumber >= 0 && rowNumber < static_cast<int>(soundResults.size()))
    {
        const auto& sound = soundResults[(size_t)rowNumber];
        rowComp->updateItem(sound, isCurrentSoundPlaying(sound.id));
    }

    return rowComp;
}

// ==============================================================================
// Renderizado Gráfico y Maquetación
// ==============================================================================
void FreesoundOverlayComponent::paint(juce::Graphics& g)
{
    // Fondo translúcido profundo (Dark Mode por capas - diseno-gui-pc-pro)
    g.fillAll(juce::Colour(0xe4050912));

    auto cardBounds = getLocalBounds().reduced(24, 18).toFloat();
    const float corner = 12.0f;

    // Sombra exterior del modal
    g.setColour(juce::Colour(0x77000000));
    g.fillRoundedRectangle(cardBounds.translated(0.0f, 4.0f), corner);

    // Tarjeta elevada Nivel 1
    juce::ColourGradient cardGrad(juce::Colour(0xfc111b30), cardBounds.getX(), cardBounds.getY(),
                                  juce::Colour(0xfc09101f), cardBounds.getX(), cardBounds.getBottom(), false);
    g.setGradientFill(cardGrad);
    g.fillRoundedRectangle(cardBounds, corner);

    // Bisel especular interno superior
    g.setColour(juce::Colour(0x22ffffff));
    g.drawRoundedRectangle(cardBounds.reduced(1.0f), corner, 1.0f);

    g.setColour(juce::Colour(0xff38bdf8).withAlpha(0.48f));
    g.drawRoundedRectangle(cardBounds, corner, 1.4f);
}

void FreesoundOverlayComponent::resized()
{
    auto area = getLocalBounds().reduced(36, 28);

    if (!hasValidApiKey)
    {
        // Layout Estado A: Pide clave
        const int midW = juce::jlimit(380, 580, area.getWidth() - 80);
        auto centerCol = area.withSizeKeepingCentre(midW, 320);

        introTitleLabel.setBounds(centerCol.removeFromTop(32));
        centerCol.removeFromTop(6);
        introSubtitleLabel.setBounds(centerCol.removeFromTop(44));
        centerCol.removeFromTop(18);

        apiKeyInputLabel.setBounds(centerCol.removeFromTop(24));
        centerCol.removeFromTop(6);

        auto inputRow = centerCol.removeFromTop(36);
        pasteKeyButton.setBounds(inputRow.removeFromRight(85));
        inputRow.removeFromRight(6);
        apiKeyEditor.setBounds(inputRow);

        centerCol.removeFromTop(14);
        connectButton.setBounds(centerCol.removeFromTop(38));

        centerCol.removeFromTop(12);
        getApiKeyHelpButton.setBounds(centerCol.removeFromTop(28));

        centerCol.removeFromTop(8);
        introStatusLabel.setBounds(centerCol);

        closeOverlayButton.setBounds(area.getRight() - 150, area.getY(), 150, 30);
    }
    else
    {
        // Layout Estado B: Buscador, Filtros y Paginación
        auto topRow = area.removeFromTop(32);
        closeOverlayButton.setBounds(topRow.removeFromRight(150));
        topRow.removeFromRight(8);
        changeKeyButton.setBounds(topRow.removeFromRight(130));
        topRow.removeFromRight(8);
        audioRouteButton.setBounds(topRow.removeFromRight(150));
        topRow.removeFromRight(8);
        if (isViewingSimilar)
        {
            exitSimilarButton.setBounds(topRow.removeFromRight(170));
            topRow.removeFromRight(8);
        }
        searchStatusBadge.setBounds(topRow.removeFromRight(140));
        searchHeaderTitleLabel.setBounds(topRow);

        area.removeFromTop(10);

        // Barra de búsqueda
        auto searchRow = area.removeFromTop(34);
        searchButton.setBounds(searchRow.removeFromRight(145));
        searchRow.removeFromRight(8);
        searchBox.setBounds(searchRow);

        area.removeFromTop(10);

        // Fila de 4 Filtros: Duración, Orden, Licencia, Formato
        auto filterRow = area.removeFromTop(28);
        const int filterColW = (filterRow.getWidth() - 36) / 4;

        auto durArea = filterRow.removeFromLeft(filterColW);
        durationFilterLabel.setBounds(durArea.removeFromLeft(58));
        durationFilterBox.setBounds(durArea);

        filterRow.removeFromLeft(12);

        auto sortArea = filterRow.removeFromLeft(filterColW);
        sortFilterLabel.setBounds(sortArea.removeFromLeft(58));
        sortFilterBox.setBounds(sortArea);

        filterRow.removeFromLeft(12);

        auto licArea = filterRow.removeFromLeft(filterColW);
        licenseFilterLabel.setBounds(licArea.removeFromLeft(58));
        licenseFilterBox.setBounds(licArea);

        filterRow.removeFromLeft(12);

        auto typeArea = filterRow;
        typeFilterLabel.setBounds(typeArea.removeFromLeft(58));
        typeFilterBox.setBounds(typeArea);

        area.removeFromTop(10);

        // Barra inferior de paginación
        auto pageNavArea = area.removeFromBottom(28);
        prevPageButton.setBounds(pageNavArea.removeFromLeft(105));
        nextPageButton.setBounds(pageNavArea.removeFromRight(105));
        pageInfoLabel.setBounds(pageNavArea);

        area.removeFromBottom(8);

        // Lista de resultados
        resultsListBox.setBounds(area);
        resultsFeedbackLabel.setBounds(area);
    }
}

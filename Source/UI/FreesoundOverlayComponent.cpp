#include "FreesoundOverlayComponent.h"

// ==============================================================================
// Componente de Fila Individual para cada Resultado de Sonido de Freesound
// ==============================================================================
class FreesoundItemRowComponent : public juce::Component
{
public:
    FreesoundItemRowComponent(std::function<void(const FreesoundItem&)> onPreviewCb,
                              std::function<void(const FreesoundItem&)> onLoadCb)
        : onPreview(std::move(onPreviewCb)), onLoad(std::move(onLoadCb))
    {
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

        // Nombre del sonido
        auto textArea = bounds.reduced(10.0f, 4.0f);
        auto leftArea = textArea.removeFromLeft(textArea.getWidth() - 250);

        g.setColour(juce::Colour(0xfff8fafc));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawFittedText(item.name, leftArea.removeFromTop(leftArea.getHeight() * 0.55f).toNearestInt(),
                         juce::Justification::centredLeft, 1, 0.75f);

        // Subtítulo con metadata (duración, usuario, licencia)
        juce::String licBadge = item.license.containsIgnoreCase("zero") ? "CC0 (Dominio Público)" : "CC-BY";
        juce::String metaStr = juce::String(item.duration, 1) + "s  |  @" + item.username + "  |  " + licBadge;
        g.setColour(juce::Colour(0xff94a3b8));
        g.setFont(juce::Font(11.0f, juce::Font::plain));
        g.drawFittedText(metaStr, leftArea.toNearestInt(), juce::Justification::centredLeft, 1, 0.70f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8, 6);
        loadBtn.setBounds(area.removeFromRight(145));
        area.removeFromRight(6);
        previewBtn.setBounds(area.removeFromRight(95));
    }

private:
    FreesoundItem item;
    std::function<void(const FreesoundItem&)> onPreview;
    std::function<void(const FreesoundItem&)> onLoad;
    juce::TextButton previewBtn { juce::String::fromUTF8("▶ Escuchar") };
    juce::TextButton loadBtn    { juce::String::fromUTF8("⚡ Cargar en Pad") };
};

// ==============================================================================
// FreesoundOverlayComponent
// ==============================================================================
FreesoundOverlayComponent::FreesoundOverlayComponent(ClipSamplerAudioProcessor& processorToUse)
    : processor(processorToUse)
{
    previewFormatManager.registerBasicFormats();
    previewTransport.addChangeListener(this);
    previewDeviceManager.initialiseWithDefaultDevices(0, 2);
    previewDeviceManager.addAudioCallback(&previewPlayer);
    previewPlayer.setSource(&previewTransport);

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
    // Estado B: Buscador de Sonidos y Filtros
    // =========================================================================
    searchHeaderTitleLabel.setText(juce::String::fromUTF8("🎵 EXPLORADOR DE SAMPLES FREESOUND"), juce::dontSendNotification);
    searchHeaderTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    searchHeaderTitleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addChildComponent(searchHeaderTitleLabel);

    searchStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
    searchStatusBadge.setFont(juce::Font(11.5f, juce::Font::bold));
    addChildComponent(searchStatusBadge);

    changeKeyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff334155));
    changeKeyButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
    changeKeyButton.onClick = [this] { onChangeApiKeyClicked(); };
    addChildComponent(changeKeyButton);

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
    searchBox.onReturnKey = [this] { triggerSearch(); };
    addChildComponent(searchBox);

    searchButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0284c7));
    searchButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    searchButton.onClick = [this] { triggerSearch(); };
    addChildComponent(searchButton);

    // Filtros
    durationFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    durationFilterLabel.setFont(juce::Font(11.5f, juce::Font::bold));
    addChildComponent(durationFilterLabel);

    durationFilterBox.addItem(juce::String::fromUTF8("Cualquier Duración"), 1);
    durationFilterBox.addItem(juce::String::fromUTF8("One-Shots (< 2s)"), 2);
    durationFilterBox.addItem(juce::String::fromUTF8("Loops Cortos (2s - 6s)"), 3);
    durationFilterBox.addItem(juce::String::fromUTF8("Frases Musicales (6s - 15s)"), 4);
    durationFilterBox.addItem(juce::String::fromUTF8("Muestras Largas (> 15s)"), 5);
    durationFilterBox.setSelectedId(1, juce::dontSendNotification);
    durationFilterBox.onChange = [this] { triggerSearch(); };
    addChildComponent(durationFilterBox);

    sortFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    sortFilterLabel.setFont(juce::Font(11.5f, juce::Font::bold));
    addChildComponent(sortFilterLabel);

    sortFilterBox.addItem(juce::String::fromUTF8("Relevancia (Score)"), 1);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Descargados"), 2);
    sortFilterBox.addItem(juce::String::fromUTF8("Mejor Calificados"), 3);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Recientes"), 4);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Cortos"), 5);
    sortFilterBox.setSelectedId(1, juce::dontSendNotification);
    sortFilterBox.onChange = [this] { triggerSearch(); };
    addChildComponent(sortFilterBox);

    licenseFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    licenseFilterLabel.setFont(juce::Font(11.5f, juce::Font::bold));
    addChildComponent(licenseFilterLabel);

    licenseFilterBox.addItem(juce::String::fromUTF8("Todas las Licencias"), 1);
    licenseFilterBox.addItem(juce::String::fromUTF8("Creative Commons 0 (Dominio Público)"), 2);
    licenseFilterBox.addItem(juce::String::fromUTF8("Attribution (CC-BY)"), 3);
    licenseFilterBox.setSelectedId(1, juce::dontSendNotification);
    licenseFilterBox.onChange = [this] { triggerSearch(); };
    addChildComponent(licenseFilterBox);

    // ListBox de resultados
    resultsListBox.setModel(this);
    resultsListBox.setRowHeight(52);
    resultsListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff131c2e));
    resultsListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff2a3f65));
    addChildComponent(resultsListBox);

    resultsFeedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    resultsFeedbackLabel.setJustificationType(juce::Justification::centred);
    resultsFeedbackLabel.setFont(juce::Font(13.0f, juce::Font::italic));
    resultsFeedbackLabel.setText(juce::String::fromUTF8("Ingresa un término arriba y pulsa 'Buscar Sonidos'."), juce::dontSendNotification);
    addChildComponent(resultsFeedbackLabel);

    loadStoredApiKey();
}

FreesoundOverlayComponent::~FreesoundOverlayComponent()
{
    previewTransport.removeChangeListener(this);
    previewTransport.stop();
    previewTransport.setSource(nullptr);
    previewPlayer.setSource(nullptr);
    previewDeviceManager.removeAudioCallback(&previewPlayer);
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

    // Componentes del Estado B (Buscador y Filtros)
    searchHeaderTitleLabel.setVisible(showSearch);
    searchStatusBadge.setVisible(showSearch);
    changeKeyButton.setVisible(showSearch);
    closeOverlayButton.setVisible(true); // Siempre accesible para cerrar
    searchBox.setVisible(showSearch);
    searchButton.setVisible(showSearch);
    durationFilterLabel.setVisible(showSearch);
    durationFilterBox.setVisible(showSearch);
    sortFilterLabel.setVisible(showSearch);
    sortFilterBox.setVisible(showSearch);
    licenseFilterLabel.setVisible(showSearch);
    licenseFilterBox.setVisible(showSearch);
    resultsListBox.setVisible(showSearch);
    resultsFeedbackLabel.setVisible(showSearch && soundResults.empty());

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
    triggerSearch(); // Búsqueda inicial automática
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
// Búsqueda Asíncrona en Freesound API v2
// ==============================================================================
void FreesoundOverlayComponent::triggerSearch()
{
    if (!hasValidApiKey)
        return;

    stopPreview();

    juce::String query = searchBox.getText().trim();
    if (query.isEmpty())
        query = "sound"; // Término por defecto para explorar

    // Construcción de filtros
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

    // Ordenamiento
    juce::String sort = "score";
    int sortId = sortFilterBox.getSelectedId();
    if (sortId == 2) sort = "downloads_desc";
    else if (sortId == 3) sort = "rating_desc";
    else if (sortId == 4) sort = "created_desc";
    else if (sortId == 5) sort = "duration_asc";

    resultsFeedbackLabel.setText(juce::String::fromUTF8("Conectando con Freesound y buscando samples..."), juce::dontSendNotification);
    resultsFeedbackLabel.setVisible(true);

    performSearchAsync(query, filter, sort);
}

void FreesoundOverlayComponent::performSearchAsync(const juce::String& query, const juce::String& filter, const juce::String& sort)
{
    isSearching.store(true);

    auto safeKey = currentApiKey;
    std::thread([this, safeKey, query, filter, sort]() {
        juce::String endpoint = "https://freesound.org/apiv2/search/text/?token=" + juce::URL::addEscapeChars(safeKey, true)
                              + "&query=" + juce::URL::addEscapeChars(query, true)
                              + "&fields=id,name,username,duration,license,previews"
                              + "&page_size=20";

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
                else if (auto* resArr = rootObj->getProperty("results").getArray())
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
            else
            {
                errorMsg = juce::String::fromUTF8("Respuesta no válida del servidor de Freesound.");
            }
        }
        else
        {
            errorMsg = juce::String::fromUTF8("No se pudo conectar con freesound.org. Verifica tu conexión a internet o tu API Key.");
        }

        juce::MessageManager::callAsync([this, newResults, errorMsg]() {
            isSearching.store(false);
            soundResults = newResults;
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
// Preescucha de Audio y Descarga al Plugin
// ==============================================================================
void FreesoundOverlayComponent::previewSound(const FreesoundItem& sound)
{
    if (currentlyPreviewingId == sound.id && previewTransport.isPlaying())
    {
        stopPreview();
        return;
    }

    stopPreview();
    currentlyPreviewingId = sound.id;

    juce::String previewUrl = sound.previewHqMp3.isNotEmpty() ? sound.previewHqMp3 : sound.previewLqMp3;
    if (previewUrl.isEmpty())
        return;

    auto cacheDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ClipSampler").getChildFile("AudioCache");
    cacheDir.createDirectory();
    auto tempFile = cacheDir.getChildFile("preview_" + sound.id + ".mp3");

    // Si ya lo tenemos en caché, reproducimos al instante
    if (tempFile.existsAsFile() && tempFile.getSize() > 2048)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(previewFormatManager.createReaderFor(tempFile));
        if (reader != nullptr)
        {
            previewReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
            previewTransport.setSource(previewReaderSource.get(), 0, nullptr, previewReaderSource->getAudioFormatReader()->sampleRate);
            previewTransport.start();
            resultsListBox.repaint();
            return;
        }
    }

    // Si no, lo descargamos asíncronamente y lo reproducimos
    std::thread([this, previewUrl, tempFile, sound]() {
        juce::URL url(previewUrl);
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(8000));
        if (stream != nullptr)
        {
            tempFile.deleteFile();
            juce::FileOutputStream out(tempFile);
            out.writeFromInputStream(*stream, -1);
            out.flush();

            juce::MessageManager::callAsync([this, tempFile, sound]() {
                if (currentlyPreviewingId == sound.id && tempFile.existsAsFile())
                {
                    std::unique_ptr<juce::AudioFormatReader> reader(previewFormatManager.createReaderFor(tempFile));
                    if (reader != nullptr)
                    {
                        previewReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
                        previewTransport.setSource(previewReaderSource.get(), 0, nullptr, previewReaderSource->getAudioFormatReader()->sampleRate);
                        previewTransport.start();
                        resultsListBox.repaint();
                    }
                }
            });
        }
    }).detach();
}

void FreesoundOverlayComponent::stopPreview()
{
    previewTransport.stop();
    previewTransport.setSource(nullptr);
    previewReaderSource.reset();
    currentlyPreviewingId.clear();
    resultsListBox.repaint();
}

void FreesoundOverlayComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &previewTransport)
    {
        if (!previewTransport.isPlaying())
        {
            currentlyPreviewingId.clear();
            resultsListBox.repaint();
        }
    }
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
            juce::FileOutputStream out(targetFile);
            out.writeFromInputStream(*stream, -1);
            out.flush();

            juce::MessageManager::callAsync([this, targetFile, sound]() {
                if (targetFile.existsAsFile() && targetFile.getSize() > 1024)
                {
                    processor.loadAudioFileIntoSlot(targetFile, 1);
                    processor.setStatusMessage(juce::String::fromUTF8("¡Sample de Freesound cargado!: ") + sound.name);
                    setVisible(false); // Cerramos el overlay y volvemos a ClipSampler con el sample listo para tocar
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
            [this](const FreesoundItem& item) { downloadAndLoadSample(item); }
        );
    }

    if (rowNumber >= 0 && rowNumber < static_cast<int>(soundResults.size()))
    {
        const auto& sound = soundResults[(size_t)rowNumber];
        rowComp->updateItem(sound, currentlyPreviewingId == sound.id);
    }

    return rowComp;
}

// ==============================================================================
// Renderizado Gráfico y Maquetación
// ==============================================================================
void FreesoundOverlayComponent::paint(juce::Graphics& g)
{
    // Fondo translúcido con desenfoque zafiro profundo
    g.fillAll(juce::Colour(0xd80b1326));

    auto cardBounds = getLocalBounds().reduced(24, 18).toFloat();
    const float corner = 12.0f;

    // Tarjeta elegante principal
    juce::ColourGradient cardGrad(juce::Colour(0xfa18243c), cardBounds.getX(), cardBounds.getY(),
                                  juce::Colour(0xfa121b2d), cardBounds.getX(), cardBounds.getBottom(), false);
    g.setGradientFill(cardGrad);
    g.fillRoundedRectangle(cardBounds, corner);

    g.setColour(juce::Colour(0xff38bdf8).withAlpha(0.35f));
    g.drawRoundedRectangle(cardBounds, corner, 1.3f);
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
        // Layout Estado B: Buscador y Filtros
        auto topRow = area.removeFromTop(32);
        closeOverlayButton.setBounds(topRow.removeFromRight(150));
        topRow.removeFromRight(8);
        changeKeyButton.setBounds(topRow.removeFromRight(130));
        topRow.removeFromRight(12);
        searchStatusBadge.setBounds(topRow.removeFromRight(200));
        searchHeaderTitleLabel.setBounds(topRow);

        area.removeFromTop(12);

        // Barra de búsqueda
        auto searchRow = area.removeFromTop(34);
        searchButton.setBounds(searchRow.removeFromRight(145));
        searchRow.removeFromRight(8);
        searchBox.setBounds(searchRow);

        area.removeFromTop(10);

        // Fila de Filtros
        auto filterRow = area.removeFromTop(28);
        const int filterColW = (filterRow.getWidth() - 24) / 3;

        auto durArea = filterRow.removeFromLeft(filterColW);
        durationFilterLabel.setBounds(durArea.removeFromLeft(60));
        durationFilterBox.setBounds(durArea);

        filterRow.removeFromLeft(12);

        auto sortArea = filterRow.removeFromLeft(filterColW);
        sortFilterLabel.setBounds(sortArea.removeFromLeft(75));
        sortFilterBox.setBounds(sortArea);

        filterRow.removeFromLeft(12);

        auto licArea = filterRow;
        licenseFilterLabel.setBounds(licArea.removeFromLeft(58));
        licenseFilterBox.setBounds(licArea);

        area.removeFromTop(10);

        // Lista de resultados
        resultsListBox.setBounds(area);
        resultsFeedbackLabel.setBounds(area);
    }
}

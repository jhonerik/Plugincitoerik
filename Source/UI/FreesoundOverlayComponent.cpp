#include "FreesoundOverlayComponent.h"

// ==============================================================================
// Componente de Fila Individual para cada Resultado de Sonido de Freesound
// (Diseño Pro: diseno-gui-pc-pro = desktop-ergonomics-hig + juce-custom-lookandfeel)
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
        similarBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6d28d9)); // Violeta Real Pro
        similarBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff5f3ff));
        similarBtn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        similarBtn.setTooltip(juce::String::fromUTF8("Buscar muestras acústicamente similares en Freesound"));
        similarBtn.onClick = [this] { if (onSimilar) onSimilar(item); };
        addAndMakeVisible(similarBtn);

        previewBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0284c7)); // Azul Zafiro Cian
        previewBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        previewBtn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        previewBtn.setTooltip(juce::String::fromUTF8("Preescuchar muestra en tiempo real"));
        previewBtn.onClick = [this] { if (onPreview) onPreview(item); };
        addAndMakeVisible(previewBtn);

        loadBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff059669)); // Esmeralda Studio Pro
        loadBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        loadBtn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        loadBtn.setTooltip(juce::String::fromUTF8("Descargar muestra y cargarla directamente en el Visor 1 y Pads MPC"));
        loadBtn.onClick = [this] { if (onLoad) onLoad(item); };
        addAndMakeVisible(loadBtn);
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override  { repaint(); }

    void updateItem(const FreesoundItem& newItem, bool isCurrentlyPlaying, int newRowIndex)
    {
        item = newItem;
        playingState = isCurrentlyPlaying;
        rowIdx = newRowIndex;
        previewBtn.setButtonText(isCurrentlyPlaying
                                     ? juce::String::fromUTF8("⏹ Detener")
                                     : juce::String::fromUTF8("▶ Escuchar"));
        previewBtn.setColour(juce::TextButton::buttonColourId,
                             isCurrentlyPlaying ? juce::Colour(0xffea580c) : juce::Colour(0xff0284c7));
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(3.0f, 3.0f);
        const float corner = 9.0f;
        const bool isHovered = isMouseOver(true);

        // 1. Sombra proyectada inferior de la tarjeta de fila
        g.setColour(juce::Colour(0x44000000));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), corner);

        // 2. Superficie multicapa según estado (Reproduciendo / Hover / Alternancia Par-Impar)
        juce::Colour topBg, botBg, borderCol, accentBarCol;
        if (playingState)
        {
            topBg        = juce::Colour(0xff0f3532);
            botBg        = juce::Colour(0xff092226);
            borderCol    = juce::Colour(0xff10b981);
            accentBarCol = juce::Colour(0xff10b981);
        }
        else if (isHovered)
        {
            topBg        = juce::Colour(0xff192c4e);
            botBg        = juce::Colour(0xff101d36);
            borderCol    = juce::Colour(0xff38bdf8);
            accentBarCol = juce::Colour(0xff38bdf8);
        }
        else
        {
            const bool even = (rowIdx % 2 == 0);
            topBg        = even ? juce::Colour(0xff111d35) : juce::Colour(0xff15233f);
            botBg        = even ? juce::Colour(0xff0b1324) : juce::Colour(0xff0e182c);
            borderCol    = juce::Colour(0xff253b63);
            accentBarCol = (item.bpm > 10.0) ? juce::Colour(0xfff59e0b) : juce::Colour(0xff0284c7);
        }

        juce::ColourGradient rowGrad(topBg, bounds.getX(), bounds.getY(),
                                     botBg, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(rowGrad);
        g.fillRoundedRectangle(bounds, corner);

        // Reflejo especular superior sutil
        g.setColour(juce::Colours::white.withAlpha(isHovered ? 0.12f : 0.06f));
        g.drawHorizontalLine((int)bounds.getY() + 1, bounds.getX() + 6.0f, bounds.getRight() - 6.0f);

        // Borde perimetral
        g.setColour(borderCol.withAlpha((playingState || isHovered) ? 0.90f : 0.72f));
        g.drawRoundedRectangle(bounds, corner, (playingState || isHovered) ? 1.4f : 1.0f);

        // 3. Barra LED lateral izquierda en cada fila
        auto ledBar = juce::Rectangle<float>(bounds.getX() + 3.5f, bounds.getY() + 6.0f, 3.8f, bounds.getHeight() - 12.0f);
        g.setColour(accentBarCol);
        g.fillRoundedRectangle(ledBar, 1.9f);

        // Área reservada para textos y píldoras a la izquierda de los 3 botones de acción
        auto textArea = bounds.reduced(16.0f, 9.0f);
        textArea.removeFromLeft(4.0f);
        auto leftArea = textArea.removeFromLeft(textArea.getWidth() - 366.0f);

        // =====================================================================
        // LÍNEA 1: Título de la Muestra + Píldoras de Color ([BPM], Formato, Licencia)
        // =====================================================================
        auto topRow = leftArea.removeFromTop(leftArea.getHeight() * 0.53f);

        auto drawBadgePill = [&](juce::Rectangle<float>& rowRect, const juce::String& badgeText,
                                 juce::Colour bgCol, juce::Colour outlineCol, juce::Colour txtCol)
        {
            juce::Font bf(12.4f, juce::Font::bold);
            float bw = bf.getStringWidthFloat(badgeText) + 18.0f;
            float bh = juce::jmin(24.0f, rowRect.getHeight() - 4.0f);
            auto pillRect = juce::Rectangle<float>(rowRect.getX(), rowRect.getCentreY() - bh * 0.5f, bw, bh);
            if (pillRect.getRight() > rowRect.getRight())
                return;

            g.setColour(bgCol);
            g.fillRoundedRectangle(pillRect, 4.5f);
            g.setColour(outlineCol);
            g.drawRoundedRectangle(pillRect, 4.5f, 1.0f);
            g.setColour(txtCol);
            g.setFont(bf);
            g.drawFittedText(badgeText, pillRect.toNearestInt(), juce::Justification::centred, 1);

            rowRect.removeFromLeft(bw + 8.0f);
        };

        // Medimos el título para colocar las píldoras justo después
        juce::Font titleFont(16.8f, juce::Font::bold);
        g.setFont(titleFont);
        const float maxTitleW = juce::jmax(110.0f, topRow.getWidth() - 230.0f);
        const float actualTitleW = juce::jmin(maxTitleW, titleFont.getStringWidthFloat(item.name) + 8.0f);

        auto titleRect = topRow.removeFromLeft(actualTitleW);
        g.setColour(playingState ? juce::Colour(0xff6ee7b7) : juce::Colour(0xfff8fafc));
        g.drawFittedText(item.name, titleRect.toNearestInt(), juce::Justification::centredLeft, 1, 0.82f);

        topRow.removeFromLeft(6.0f);

        // Píldora 1: BPM en Ámbar Dorado
        if (item.bpm > 10.0)
        {
            juce::String bpmTxt = juce::String(juce::roundToInt(item.bpm)) + " BPM";
            drawBadgePill(topRow, bpmTxt,
                          juce::Colour(0xff451a03), juce::Colour(0xfff59e0b), juce::Colour(0xfffde68a));
        }

        // Píldora 2: Formato (WAV/FLAC en Cian; M4A/MP3/OGG en Índigo)
        if (item.type.isNotEmpty())
        {
            juce::String fmtTxt = item.type.toUpperCase();
            const bool isLossless = (fmtTxt == "WAV" || fmtTxt == "FLAC" || fmtTxt == "AIFF");
            drawBadgePill(topRow, fmtTxt,
                          isLossless ? juce::Colour(0xff083344) : juce::Colour(0xff2e1065),
                          isLossless ? juce::Colour(0xff06b6d4) : juce::Colour(0xff8b5cf6),
                          isLossless ? juce::Colour(0xff67e8f9) : juce::Colour(0xffddd6fe));
        }

        // Píldora 3: Licencia (CC0 en Esmeralda; CC-BY en Azul Cielo)
        const bool isCc0 = item.license.containsIgnoreCase("zero");
        drawBadgePill(topRow, isCc0 ? "CC0 LIBRE" : "CC-BY",
                      isCc0 ? juce::Colour(0xff064e3b) : juce::Colour(0xff172554),
                      isCc0 ? juce::Colour(0xff10b981) : juce::Colour(0xff3b82f6),
                      isCc0 ? juce::Colour(0xff6ee7b7) : juce::Colour(0xff93c5fd));

        // =====================================================================
        // LÍNEA 2: Subtítulo con Jerarquía Cromática (Duración • Autor • Specs • #Tags)
        // =====================================================================
        auto botRow = leftArea;
        juce::Font metaFont(13.6f, juce::Font::plain);
        juce::Font metaBold(13.6f, juce::Font::bold);

        auto drawSegment = [&](const juce::String& txt, juce::Colour col, bool bold = false)
        {
            if (botRow.getWidth() <= 8.0f || txt.isEmpty())
                return;
            const auto& f = bold ? metaBold : metaFont;
            g.setFont(f);
            g.setColour(col);
            float w = juce::jmin(botRow.getWidth(), f.getStringWidthFloat(txt) + 2.0f);
            auto segRect = botRow.removeFromLeft(w);
            g.drawFittedText(txt, segRect.toNearestInt(), juce::Justification::centredLeft, 1, 0.85f);
        };

        // Duración en Ámbar claro
        drawSegment(juce::String(item.duration, 1) + "s", juce::Colour(0xfffbbf24), true);
        drawSegment("   |   ", juce::Colour(0xff475569));

        // Autor en Plata Azul
        drawSegment("@" + item.username, juce::Colour(0xffe2e8f0), true);

        // Especificaciones de audio en Cian Suave
        juce::String formatSpec;
        if (item.channels > 0)
            formatSpec += (item.channels == 2 ? "Stereo" : "Mono");
        if (item.samplerate > 0)
            formatSpec += " " + juce::String(item.samplerate / 1000) + "kHz";
        if (formatSpec.isNotEmpty())
        {
            drawSegment("   |   ", juce::Colour(0xff475569));
            drawSegment(formatSpec, juce::Colour(0xff7dd3fc));
        }

        // Etiquetas (#tags) en Lavanda / Índigo Luminoso
        juce::String tagsStr;
        int maxTags = juce::jmin(3, item.tags.size());
        for (int i = 0; i < maxTags; ++i)
            tagsStr += (i == 0 ? "#" : "  #") + item.tags[i];
        if (tagsStr.isNotEmpty())
        {
            drawSegment("   |   ", juce::Colour(0xff475569));
            drawSegment(tagsStr, juce::Colour(0xffa5b4fc));
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12, 16);
        loadBtn.setBounds(area.removeFromRight(142));
        area.removeFromRight(6);
        previewBtn.setBounds(area.removeFromRight(106));
        area.removeFromRight(6);
        similarBtn.setBounds(area.removeFromRight(102));
    }

private:
    FreesoundItem item;
    bool playingState = false;
    int rowIdx = 0;
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

    apiKeyEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0b1325));
    apiKeyEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    apiKeyEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2e4670));
    apiKeyEditor.setTextToShowWhenEmpty(juce::String::fromUTF8("Ejemplo: a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0"), juce::Colour(0xff64748b));
    apiKeyEditor.onReturnKey = [this] { onConnectApiKeyClicked(); };
    addAndMakeVisible(apiKeyEditor);

    pasteKeyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff253b63));
    pasteKeyButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pasteKeyButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    pasteKeyButton.onClick = [this] { onPasteApiKeyClicked(); };
    addAndMakeVisible(pasteKeyButton);

    connectButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff059669));
    connectButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    connectButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    connectButton.onClick = [this] { onConnectApiKeyClicked(); };
    addAndMakeVisible(connectButton);

    getApiKeyHelpButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff172542));
    getApiKeyHelpButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff38bdf8));
    getApiKeyHelpButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    getApiKeyHelpButton.onClick = [this] { onOpenApiKeyWebClicked(); };
    addAndMakeVisible(getApiKeyHelpButton);

    introStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff59e0b));
    introStatusLabel.setJustificationType(juce::Justification::centred);
    introStatusLabel.setFont(juce::Font(12.0f, juce::Font::italic));
    addAndMakeVisible(introStatusLabel);

    // =========================================================================
    // Estado B: Buscador de Sonidos, Filtros y Paginación (Paleta Pro Dark Mode)
    // =========================================================================
    searchHeaderTitleLabel.setText(juce::String::fromUTF8("🎵 EXPLORADOR DE SAMPLES FREESOUND (API v2)"), juce::dontSendNotification);
    searchHeaderTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    searchHeaderTitleLabel.setFont(juce::Font(15.5f, juce::Font::bold));
    addChildComponent(searchHeaderTitleLabel);

    searchStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
    searchStatusBadge.setJustificationType(juce::Justification::centred);
    searchStatusBadge.setFont(juce::Font(11.2f, juce::Font::bold));
    addChildComponent(searchStatusBadge);

    exitSimilarButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6d28d9));
    exitSimilarButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff5f3ff));
    exitSimilarButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    exitSimilarButton.onClick = [this] {
        isViewingSimilar = false;
        exitSimilarButton.setVisible(false);
        triggerSearch(1);
    };
    addChildComponent(exitSimilarButton);

    changeKeyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e2f4d));
    changeKeyButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xfffcd34d));
    changeKeyButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    changeKeyButton.setTooltip(juce::String::fromUTF8("Cambiar o actualizar tu token personal de Freesound API v2"));
    changeKeyButton.onClick = [this] { onChangeApiKeyClicked(); };
    addChildComponent(changeKeyButton);

    audioRouteButton.setButtonText(juce::String::fromUTF8("🔊 Salida: Auto"));
    audioRouteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0369a1));
    audioRouteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe0f2fe));
    audioRouteButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    audioRouteButton.setTooltip(juce::String::fromUTF8("Haz clic para alternar: Modo Auto (reproduce en DAW o altavoces según actividad), Solo DAW o Solo Directo"));
    audioRouteButton.onClick = [this] {
        if (currentAudioMode == AudioOutputMode::Auto)
        {
            currentAudioMode = AudioOutputMode::DawMixer;
            audioRouteButton.setButtonText(juce::String::fromUTF8("🎛️ Salida: Solo DAW"));
        }
        else if (currentAudioMode == AudioOutputMode::DawMixer)
        {
            currentAudioMode = AudioOutputMode::DirectWindows;
            audioRouteButton.setButtonText(juce::String::fromUTF8("🔈 Salida: Solo Directo"));
        }
        else
        {
            currentAudioMode = AudioOutputMode::Auto;
            audioRouteButton.setButtonText(juce::String::fromUTF8("🔊 Salida: Auto"));
        }
    };
    addChildComponent(audioRouteButton);

    closeOverlayButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffbe123c));
    closeOverlayButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffe4e6));
    closeOverlayButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    closeOverlayButton.setTooltip(juce::String::fromUTF8("Cerrar el explorador Freesound y volver a ClipSampler (Atajo: Escape)"));
    closeOverlayButton.onClick = [this] {
        stopPreview();
        setVisible(false);
    };
    addChildComponent(closeOverlayButton);

    // Barra de búsqueda empotrada Nivel 2
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff080f1f));
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colour(0xfff8fafc));
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2c4470));
    searchBox.setTextToShowWhenEmpty(juce::String::fromUTF8("Escribe un término (ej. kick, 808, snare, vinyl loop, vocal, piano, riser)..."), juce::Colour(0xff64748b));
    searchBox.onReturnKey = [this] { triggerSearch(1); };
    addChildComponent(searchBox);

    searchButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0284c7));
    searchButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    searchButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    searchButton.setTooltip(juce::String::fromUTF8("Ejecutar búsqueda en Freesound API v2"));
    searchButton.onClick = [this] { triggerSearch(1); };
    addChildComponent(searchButton);

    // Filtros con etiquetas en Cian Luminoso y cursores interactivos
    durationFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    durationFilterLabel.setFont(juce::Font(11.4f, juce::Font::bold));
    addChildComponent(durationFilterLabel);

    durationFilterBox.addItem(juce::String::fromUTF8("Cualquier Duración"), 1);
    durationFilterBox.addItem(juce::String::fromUTF8("One-Shots (< 2s)"), 2);
    durationFilterBox.addItem(juce::String::fromUTF8("Loops Cortos (2s - 6s)"), 3);
    durationFilterBox.addItem(juce::String::fromUTF8("Frases (6s - 15s)"), 4);
    durationFilterBox.addItem(juce::String::fromUTF8("Muestras Largas (> 15s)"), 5);
    durationFilterBox.setSelectedId(1, juce::dontSendNotification);
    durationFilterBox.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    durationFilterBox.onChange = [this] { triggerSearch(1); };
    addChildComponent(durationFilterBox);

    sortFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    sortFilterLabel.setFont(juce::Font(11.4f, juce::Font::bold));
    addChildComponent(sortFilterLabel);

    sortFilterBox.addItem(juce::String::fromUTF8("Relevancia (Score)"), 1);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Descargados"), 2);
    sortFilterBox.addItem(juce::String::fromUTF8("Mejor Calificados"), 3);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Recientes"), 4);
    sortFilterBox.addItem(juce::String::fromUTF8("Más Cortos"), 5);
    sortFilterBox.setSelectedId(1, juce::dontSendNotification);
    sortFilterBox.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    sortFilterBox.onChange = [this] { triggerSearch(1); };
    addChildComponent(sortFilterBox);

    licenseFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    licenseFilterLabel.setFont(juce::Font(11.4f, juce::Font::bold));
    addChildComponent(licenseFilterLabel);

    licenseFilterBox.addItem(juce::String::fromUTF8("Todas las Licencias"), 1);
    licenseFilterBox.addItem(juce::String::fromUTF8("Creative Commons 0 (CC0)"), 2);
    licenseFilterBox.addItem(juce::String::fromUTF8("Attribution (CC-BY)"), 3);
    licenseFilterBox.setSelectedId(1, juce::dontSendNotification);
    licenseFilterBox.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    licenseFilterBox.onChange = [this] { triggerSearch(1); };
    addChildComponent(licenseFilterBox);

    typeFilterLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    typeFilterLabel.setFont(juce::Font(11.4f, juce::Font::bold));
    addChildComponent(typeFilterLabel);

    typeFilterBox.addItem(juce::String::fromUTF8("Cualquier Formato"), 1);
    typeFilterBox.addItem(juce::String::fromUTF8("Solo WAV"), 2);
    typeFilterBox.addItem(juce::String::fromUTF8("Solo FLAC"), 3);
    typeFilterBox.addItem(juce::String::fromUTF8("WAV o FLAC"), 4);
    typeFilterBox.addItem(juce::String::fromUTF8("Solo AIFF"), 5);
    typeFilterBox.setSelectedId(1, juce::dontSendNotification);
    typeFilterBox.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    typeFilterBox.onChange = [this] { triggerSearch(1); };
    addChildComponent(typeFilterBox);

    // ListBox de resultados (Nivel 2 - Lienzo Profundo con Scrollbar Cian)
    resultsListBox.setModel(this);
    resultsListBox.setRowHeight(88);
    resultsListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff080e1c));
    resultsListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff23385e));
    resultsListBox.setOutlineThickness(1);
    resultsListBox.getViewport()->setScrollBarsShown(true, false);
    resultsListBox.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff38bdf8).withAlpha(0.75f));
    resultsListBox.getVerticalScrollBar().setColour(juce::ScrollBar::trackColourId, juce::Colour(0xff0b1324));
    addChildComponent(resultsListBox);

    resultsFeedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    resultsFeedbackLabel.setJustificationType(juce::Justification::centred);
    resultsFeedbackLabel.setFont(juce::Font(13.5f, juce::Font::italic));
    resultsFeedbackLabel.setText(juce::String::fromUTF8("Ingresa un término arriba y pulsa 'Buscar Sonidos'."), juce::dontSendNotification);
    addChildComponent(resultsFeedbackLabel);

    // Barra de Paginación Inferior
    prevPageButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e3a5f));
    prevPageButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe0f2fe));
    prevPageButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
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

    nextPageButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e3a5f));
    nextPageButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe0f2fe));
    nextPageButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
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
    pageInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff1f5f9));
    pageInfoLabel.setFont(juce::Font(12.2f, juce::Font::bold));
    addChildComponent(pageInfoLabel);

    pageJumpLabel.setJustificationType(juce::Justification::centredRight);
    pageJumpLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    pageJumpLabel.setFont(juce::Font(11.6f, juce::Font::bold));
    addChildComponent(pageJumpLabel);

    pageJumpEditor.setInputRestrictions(6, "0123456789");
    pageJumpEditor.setJustification(juce::Justification::centred);
    pageJumpEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff080f1f));
    pageJumpEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xff38bdf8));
    pageJumpEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2c4470));
    pageJumpEditor.setFont(juce::Font(12.2f, juce::Font::bold));
    pageJumpEditor.setTextToShowWhenEmpty(juce::String::fromUTF8("N.º"), juce::Colour(0xff64748b));
    pageJumpEditor.setTooltip(juce::String::fromUTF8("Escribe el número de página (ej. 1000 o 1653) y presiona Enter para ir directamente"));
    pageJumpEditor.onReturnKey = [this] { onGoToPageTriggered(); };
    addChildComponent(pageJumpEditor);

    pageJumpGoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0284c7));
    pageJumpGoButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pageJumpGoButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    pageJumpGoButton.setTooltip(juce::String::fromUTF8("Saltar directamente a la página escrita [Atajo: Enter]"));
    pageJumpGoButton.onClick = [this] { onGoToPageTriggered(); };
    addChildComponent(pageJumpGoButton);

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
    if (reader == nullptr)
    {
        if (auto inStream = file.createInputStream())
            reader = processor.getFormatManager().createReaderFor(std::move(inStream));
    }
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
    pageJumpLabel.setVisible(showSearch);
    pageJumpEditor.setVisible(showSearch);
    pageJumpGoButton.setVisible(showSearch);

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

void FreesoundOverlayComponent::onGoToPageTriggered()
{
    if (!hasValidApiKey)
        return;

    juce::String txt = pageJumpEditor.getText().trim();
    if (txt.isEmpty())
        return;

    int targetPage = txt.getIntValue();
    if (targetPage < 1)
        targetPage = 1;
    if (totalPages > 1 && targetPage > totalPages)
        targetPage = totalPages;

    pageJumpEditor.setText(juce::String(targetPage), false);
    stopPreview();

    resultsFeedbackLabel.setText(juce::String::fromUTF8("Cargando página ") + juce::String(targetPage) + " en Freesound API v2...", juce::dontSendNotification);
    resultsFeedbackLabel.setVisible(true);

    if (isViewingSimilar)
        searchSimilarSounds(similarTargetItem, targetPage);
    else if (lastExecutedQuery.isNotEmpty())
        performSearchAsync(lastExecutedQuery, lastExecutedFilter, lastExecutedSort, targetPage);
    else
        triggerSearch(targetPage);
}

void FreesoundOverlayComponent::updatePaginationUi()
{
    prevPageButton.setEnabled(currentPage > 1);
    nextPageButton.setEnabled(currentPage < totalPages);
    pageJumpEditor.setText(juce::String(currentPage), false);

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
        if (testReader == nullptr)
        {
            if (auto inStream = tempFile.createInputStream())
                testReader.reset(processor.getFormatManager().createReaderFor(std::move(inStream)));
        }
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
    if (cleanName.endsWithIgnoreCase(".wav") || cleanName.endsWithIgnoreCase(".flac")
        || cleanName.endsWithIgnoreCase(".aiff") || cleanName.endsWithIgnoreCase(".aif")
        || cleanName.endsWithIgnoreCase(".ogg") || cleanName.endsWithIgnoreCase(".m4a"))
    {
        cleanName = cleanName.upToLastOccurrenceOf(".", false, false);
    }
    if (!cleanName.endsWithIgnoreCase(".mp3"))
        cleanName += ".mp3";

    auto targetFile = cacheDir.getChildFile("freesound_" + sound.id + "_" + cleanName);
    auto previewCacheFile = cacheDir.getChildFile("preview_" + sound.id + ".mp3");

    if (previewCacheFile.existsAsFile() && previewCacheFile.getSize() > 2048)
    {
        previewCacheFile.copyFileTo(targetFile);
        if (targetFile.existsAsFile() && targetFile.getSize() > 1024)
        {
            processor.setAutoChopEnabled(true);
            processor.loadAudioFileIntoSlot(targetFile, 1);
            processor.setAudioOriginLabel(1, "FREESOUND");
            processor.setStatusMessage(juce::String::fromUTF8("¡Sample de Freesound cargado y rebanado en C1–G2!: ") + sound.name);
            setVisible(false);
            return;
        }
    }

    resultsFeedbackLabel.setText(juce::String::fromUTF8("Descargando sample y cargando en pads de ClipSampler..."), juce::dontSendNotification);
    resultsFeedbackLabel.setVisible(true);

    std::thread([this, downloadUrl, targetFile, sound]() {
        juce::URL url(downloadUrl);
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                .withConnectionTimeoutMs(15000)
                                                .withNumRedirectsToFollow(5)
                                                .withExtraHeaders("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) ClipSampler\r\n"));
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
                    processor.setAutoChopEnabled(true);
                    processor.loadAudioFileIntoSlot(targetFile, 1);
                    processor.setAudioOriginLabel(1, "FREESOUND");
                    processor.setStatusMessage(juce::String::fromUTF8("¡Sample de Freesound cargado y rebanado en C1–G2!: ") + sound.name);
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
        rowComp->updateItem(sound, isCurrentSoundPlaying(sound.id), rowNumber);
    }

    return rowComp;
}

// ==============================================================================
// Renderizado Gráfico y Maquetación (diseno-gui-pc-pro)
// ==============================================================================
void FreesoundOverlayComponent::paint(juce::Graphics& g)
{
    // 1. Fondo translúcido profundo (Nivel 0 - Chasis Oscuro)
    g.fillAll(juce::Colour(0xe8040811));

    auto cardBounds = getLocalBounds().reduced(22, 16).toFloat();
    const float corner = 12.5f;

    // 2. Sombra exterior profunda del modal
    g.setColour(juce::Colour(0x88000000));
    g.fillRoundedRectangle(cardBounds.translated(0.0f, 5.0f), corner);

    // 3. Tarjeta elevada Nivel 1 (Obsidiana Zafiro Multicapa)
    juce::ColourGradient cardGrad(juce::Colour(0xfd101a30), cardBounds.getX(), cardBounds.getY(),
                                  juce::Colour(0xfd070c18), cardBounds.getX(), cardBounds.getBottom(), false);
    cardGrad.addColour(0.35, juce::Colour(0xfd0b1324));
    g.setGradientFill(cardGrad);
    g.fillRoundedRectangle(cardBounds, corner);

    if (hasValidApiKey)
    {
        // 4. Franja de Cabecera Superior Elevada (Header Bar)
        auto headerRect = juce::Rectangle<float>(cardBounds.getX() + 1.5f, cardBounds.getY() + 1.5f,
                                                 cardBounds.getWidth() - 3.0f, 52.0f);
        juce::ColourGradient hdrGrad(juce::Colour(0xff162646), headerRect.getX(), headerRect.getY(),
                                     juce::Colour(0xff0c1528), headerRect.getX(), headerRect.getBottom(), false);
        g.setGradientFill(hdrGrad);
        g.fillRoundedRectangle(headerRect, corner - 1.0f);

        // Línea divisoria luminosa tricolor (Ámbar -> Cian -> Violeta) bajo la cabecera
        juce::ColourGradient lineGrad(juce::Colour(0xfff59e0b), headerRect.getX() + 16.0f, headerRect.getBottom(),
                                      juce::Colour(0xffa855f7), headerRect.getRight() - 16.0f, headerRect.getBottom(), false);
        lineGrad.addColour(0.45, juce::Colour(0xff38bdf8));
        g.setGradientFill(lineGrad);
        g.fillRect(headerRect.getX() + 12.0f, headerRect.getBottom() - 1.5f, headerRect.getWidth() - 24.0f, 1.5f);

        // Píldora Esmeralda para el indicador "● Conectado"
        if (searchStatusBadge.isVisible() && !searchStatusBadge.getBounds().isEmpty())
        {
            auto badgeR = searchStatusBadge.getBounds().toFloat().reduced(1.0f, 2.5f);
            g.setColour(juce::Colour(0xff064e3b).withAlpha(0.78f));
            g.fillRoundedRectangle(badgeR, 6.0f);
            g.setColour(juce::Colour(0xff10b981).withAlpha(0.75f));
            g.drawRoundedRectangle(badgeR, 6.0f, 1.0f);
        }

        // 5. Panel Empotrado para el Deck de Búsqueda y Filtros (Nivel 2)
        auto deckRect = juce::Rectangle<float>(cardBounds.getX() + 10.0f, headerRect.getBottom() + 6.0f,
                                               cardBounds.getWidth() - 20.0f, 82.0f);
        g.setColour(juce::Colour(0xff091121));
        g.fillRoundedRectangle(deckRect, 8.5f);
        g.setColour(juce::Colour(0xff1e3256));
        g.drawRoundedRectangle(deckRect, 8.5f, 1.0f);

        // 6. Franja Inferior de Paginación (Footer Status Bar)
        if (!pageInfoLabel.getBounds().isEmpty())
        {
            auto footerRect = juce::Rectangle<float>(cardBounds.getX() + 10.0f, (float)pageInfoLabel.getY() - 4.0f,
                                                     cardBounds.getWidth() - 20.0f, (float)pageInfoLabel.getHeight() + 8.0f);
            g.setColour(juce::Colour(0xff0b1426));
            g.fillRoundedRectangle(footerRect, 7.0f);
            g.setColour(juce::Colour(0xff1e3256));
            g.drawRoundedRectangle(footerRect, 7.0f, 1.0f);
        }
    }

    // 7. Bisel especular interno superior y borde perimetral Cian brillante
    g.setColour(juce::Colour(0x24ffffff));
    g.drawRoundedRectangle(cardBounds.reduced(1.0f), corner, 1.0f);

    g.setColour(juce::Colour(0xff38bdf8).withAlpha(0.62f));
    g.drawRoundedRectangle(cardBounds, corner, 1.5f);
}

void FreesoundOverlayComponent::resized()
{
    auto area = getLocalBounds().reduced(36, 26);

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
        closeOverlayButton.setBounds(topRow.removeFromRight(154));
        topRow.removeFromRight(8);
        changeKeyButton.setBounds(topRow.removeFromRight(132));
        topRow.removeFromRight(8);
        audioRouteButton.setBounds(topRow.removeFromRight(146));
        topRow.removeFromRight(8);
        if (isViewingSimilar)
        {
            exitSimilarButton.setBounds(topRow.removeFromRight(168));
            topRow.removeFromRight(8);
        }
        searchStatusBadge.setBounds(topRow.removeFromRight(176));
        topRow.removeFromRight(6);
        searchHeaderTitleLabel.setBounds(topRow);

        area.removeFromTop(18);

        // Barra de búsqueda dentro del Deck Nivel 2
        auto searchRow = area.removeFromTop(34).reduced(6, 0);
        searchButton.setBounds(searchRow.removeFromRight(148));
        searchRow.removeFromRight(8);
        searchBox.setBounds(searchRow);

        area.removeFromTop(10);

        // Fila de 4 Filtros: Duración, Orden, Licencia, Formato
        auto filterRow = area.removeFromTop(28).reduced(6, 0);
        const int filterColW = (filterRow.getWidth() - 36) / 4;

        auto durArea = filterRow.removeFromLeft(filterColW);
        durationFilterLabel.setBounds(durArea.removeFromLeft(60));
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

        area.removeFromTop(14);

        // Barra inferior de paginación con casilla de salto directo de página (Enter)
        auto pageNavArea = area.removeFromBottom(30).reduced(6, 0);
        prevPageButton.setBounds(pageNavArea.removeFromLeft(108));
        nextPageButton.setBounds(pageNavArea.removeFromRight(108));
        pageNavArea.removeFromRight(10);
        pageJumpGoButton.setBounds(pageNavArea.removeFromRight(50).reduced(0, 1));
        pageNavArea.removeFromRight(4);
        pageJumpEditor.setBounds(pageNavArea.removeFromRight(72).reduced(0, 1));
        pageNavArea.removeFromRight(4);
        pageJumpLabel.setBounds(pageNavArea.removeFromRight(62));
        pageInfoLabel.setBounds(pageNavArea);

        area.removeFromBottom(10);

        // Lista de resultados
        resultsListBox.setBounds(area);
        resultsFeedbackLabel.setBounds(area);
    }
}

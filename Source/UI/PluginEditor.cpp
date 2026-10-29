#include "PluginEditor.h"

// ==============================================================================
// Constructor e Inicialización de Todos los Controles Visuales
// ==============================================================================
ClipSamplerAudioProcessorEditor::ClipSamplerAudioProcessorEditor(ClipSamplerAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      originalWaveformComponent(p, 1, juce::String::fromUTF8("VISOR 1: AUDIO ORIGINAL (REBANADOR MPC C1-G2)")),
      transformedWaveformComponent(p, 2, juce::String::fromUTF8("VISOR 2: AUDIO MODIFICADO (DERECHOS ±3 ST IA)")),
      licenseOverlay(p.getLicenseManager(), [this] { updateLicenseUiState(); }),
      freesoundOverlay(p)
{
    setLookAndFeel(&scalableLnf);

    // Permitimos redimensionar libremente la ventana desde un tamaño compacto (680x540) hasta 1800x1300
    setResizable(true, true);
    setResizeLimits(680, 540, 1800, 1300);
    setSize(980, 760);
    setWantsKeyboardFocus(true);

    // =========================================================================
    // Controles del Encabezado Superior (Licencia / Prueba de 3 Días, Freesound y Mantenimiento)
    // =========================================================================
    licenseBadgeButton.onClick = [this] { onLicenseBadgeClicked(); };
    addAndMakeVisible(licenseBadgeButton);

    freesoundButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffea580c)); // Naranja cálido distintivo Freesound
    freesoundButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    freesoundButton.onClick = [this] { onFreesoundButtonClicked(); };
    addAndMakeVisible(freesoundButton);

    updateExtractorButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b3d63));
    updateExtractorButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe2e8f0));
    updateExtractorButton.onClick = [this] { onUpdateExtractorClicked(); };
    addAndMakeVisible(updateExtractorButton);

    cleanCacheButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b3d63));
    cleanCacheButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe2e8f0));
    cleanCacheButton.onClick = [this] { onCleanCacheClicked(); };
    addAndMakeVisible(cleanCacheButton);

    // =========================================================================
    // SECCIÓN 1: PANEL SUPERIOR (PLATAFORMA, ENLACE, TIEMPO, 3s/10s/30s Y DERECHOS IA)
    // =========================================================================
    topSectionGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(topSectionGroup);

    // Fila 0: Cargar Audio Local + Activar Uso Libre (Web) + Selector de Plataforma
    loadLocalFileButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0f766e)); // Teal/Cyan elegante
    loadLocalFileButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loadLocalFileButton.onClick = [this] { onLoadLocalFileClicked(); };
    addAndMakeVisible(loadLocalFileButton);

    usoLibreButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff334155)); // Slate oscuro
    usoLibreButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
    usoLibreButton.onClick = [this] { onUsoLibreClicked(); };
    addAndMakeVisible(usoLibreButton);

    platformYoutubeBtn.onClick   = [this] { onSelectPlatform(SourcePlatform::YouTube, true); };
    platformTiktokBtn.onClick    = [this] { onSelectPlatform(SourcePlatform::TikTok, true); };
    platformInstagramBtn.onClick = [this] { onSelectPlatform(SourcePlatform::Instagram, true); };
    addAndMakeVisible(platformYoutubeBtn);
    addAndMakeVisible(platformTiktokBtn);
    addAndMakeVisible(platformInstagramBtn);

    // Fila 1: Campo de Enlace (URL) con autodetección de plataforma al pegar o escribir
    urlLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e8f0));
    urlLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(urlLabel);

    urlEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff080d1a));
    urlEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    urlEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff283a5e));
    if (audioProcessor.getLastUrl().isNotEmpty())
        urlEditor.setText(audioProcessor.getLastUrl(), false);
    urlEditor.onReturnKey = [this] { onDetectDurationClicked(); };
    urlEditor.onTextChange = [this] {
        juce::String txt = urlEditor.getText().trim();
        if (txt.startsWithIgnoreCase("http"))
        {
            SourcePlatform autoPlat = AudioExtractorWorker::detectPlatformFromUrl(txt);
            if (autoPlat != audioProcessor.getSelectedPlatform())
                onSelectPlatform(autoPlat, false);
        }
    };
    addAndMakeVisible(urlEditor);

    pasteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff30446e));
    pasteButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pasteButton.onClick = [this] { onPasteClicked(); };
    addAndMakeVisible(pasteButton);

    detectDurationButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1d4e6e));
    detectDurationButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff38bdf8));
    detectDurationButton.onClick = [this] { onDetectDurationClicked(); };
    addAndMakeVisible(detectDurationButton);

    // Fila 2: Tiempo de Inicio (MM:SS) + Deslizador interactivo + Indicador Inicio/Fin del Video
    startTimeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e8f0));
    startTimeLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(startTimeLabel);

    juce::String savedTime = audioProcessor.getLastStartTime();
    if (savedTime.isEmpty())
        savedTime = "00:00";

    const int initialSeconds = AudioExtractorWorker::parseTimeToSeconds(savedTime);

    startTimeEditor.setText(savedTime, false);
    startTimeEditor.setJustification(juce::Justification::centred);
    startTimeEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff080d1a));
    startTimeEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xff38bdf8));
    startTimeEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff283a5e));
    startTimeEditor.onTextChange = [this] {
        int secs = AudioExtractorWorker::parseTimeToSeconds(startTimeEditor.getText());
        if (secs > startTimeSlider.getMaximum())
            startTimeSlider.setRange(0.0, static_cast<double>(secs + 60), 1.0);
        startTimeSlider.setValue(static_cast<double>(secs), juce::dontSendNotification);
        audioProcessor.setLastStartTime(startTimeEditor.getText().trim());
        refreshVideoTimeRangeLabel();
    };
    addAndMakeVisible(startTimeEditor);

    int initialMaxDur = audioProcessor.getDetectedVideoDurationSeconds();
    if (initialMaxDur <= 0)
        initialMaxDur = 600;

    startTimeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    startTimeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    startTimeSlider.setRange(0.0, juce::jmax((double)initialMaxDur, (double)initialSeconds), 1.0);
    startTimeSlider.setSkewFactorFromMidPoint(120.0);
    startTimeSlider.setValue(static_cast<double>(initialSeconds), juce::dontSendNotification);
    startTimeSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff152036));
    startTimeSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff38bdf8));
    startTimeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffff2a55));
    startTimeSlider.onValueChange = [this] {
        int totalSecs = static_cast<int>(std::round(startTimeSlider.getValue()));
        juce::String formatted = AudioExtractorWorker::formatSecondsToMMSS(totalSecs);
        startTimeEditor.setText(formatted, false);
        audioProcessor.setLastStartTime(formatted);
        refreshVideoTimeRangeLabel();
    };
    addAndMakeVisible(startTimeSlider);

    videoRangeBadgeLabel.setJustificationType(juce::Justification::centred);
    videoRangeBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff192742));
    videoRangeBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    videoRangeBadgeLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff385382));
    videoRangeBadgeLabel.setMinimumHorizontalScale(0.65f);
    addAndMakeVisible(videoRangeBadgeLabel);
    refreshVideoTimeRangeLabel();

    // Fila 3: Conmutador de Modo (Segundos vs Compases) + Botones de extracción + "Nueva Extracción"
    extractModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e3a5f));
    extractModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7dd3fc));
    extractModeButton.onClick = [this] { onToggleExtractModeClicked(); };
    addAndMakeVisible(extractModeButton);

    extract3sButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffd81b60));
    extract3sButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    extract3sButton.onClick = [this] { onExtractDurationClicked(3); };
    addAndMakeVisible(extract3sButton);

    extract10sButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff1744));
    extract10sButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    extract10sButton.onClick = [this] { onExtractDurationClicked(10); };
    addAndMakeVisible(extract10sButton);

    extract30sButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8e24aa));
    extract30sButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    extract30sButton.onClick = [this] { onExtractDurationClicked(30); };
    addAndMakeVisible(extract30sButton);

    newExtractionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff263759));
    newExtractionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff94a3b8));
    newExtractionButton.onClick = [this] { onNewExtractionClicked(); };
    addAndMakeVisible(newExtractionButton);

    // Fila 4: Suite de Coloración y Derechos IA (Calor Analógico, Alterar +3 Tonos Más, Alterar -3 Tonos Menos, Verificar)
    tapeWarmthBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffd97706));
    tapeWarmthBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    tapeWarmthBtn.onClick = [this] { onTapeWarmthClicked(); };
    addAndMakeVisible(tapeWarmthBtn);

    derechosPlusBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff581c87));
    derechosPlusBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    derechosPlusBtn.onClick = [this] { onApplyDerechosClicked(3); };
    addAndMakeVisible(derechosPlusBtn);

    derechosMinusBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a148c));
    derechosMinusBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    derechosMinusBtn.onClick = [this] { onApplyDerechosClicked(-3); };
    addAndMakeVisible(derechosMinusBtn);

    detectCopyrightBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0d5c3a));
    detectCopyrightBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff69f0ae));
    detectCopyrightBtn.onClick = [this] { onCheckCopyrightOriginalClicked(); };
    addAndMakeVisible(detectCopyrightBtn);

    // Fila 5: Barra de Progreso, Estado e Indicador de Derechos
    currentProgressValue = static_cast<double>(audioProcessor.getExtractionProgress());
    progressBar.setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xff152036));
    progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(progressBar);

    lastDisplayedStatus = audioProcessor.getStatusMessage();
    statusLabel.setText(lastDisplayedStatus, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e8f0));
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(statusLabel);

    copyrightBadgeLabel.setText(audioProcessor.getCopyrightBadgeText(), juce::dontSendNotification);
    copyrightBadgeLabel.setJustificationType(juce::Justification::centredRight);
    copyrightBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffd54f));
    copyrightBadgeLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(copyrightBadgeLabel);

    // =========================================================================
    // SECCIÓN 2: VISOR DUAL, REBANADOR MPC (C1-G2), PERILLAS DE EFECTOS Y MIDI
    // =========================================================================
    bottomSectionGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(bottomSectionGroup);

    // Selector de audio objetivo para efectos (Audio 1 vs Audio 2)
    fxMacroTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    fxMacroTitleLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(fxMacroTitleLabel);

    fxTargetAudio1Btn.onClick = [this] { onSelectFxTargetSlot(1); };
    addAndMakeVisible(fxTargetAudio1Btn);

    fxTargetAudio2Btn.onClick = [this] { onSelectFxTargetSlot(2); };
    addAndMakeVisible(fxTargetAudio2Btn);

    // Perilla 1: Lo-Fi 12-Bit (0% a 100%) - Doble clic resetea a 0%
    lofiKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    lofiKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lofiKnob.setRange(0.0, 100.0, 1.0);
    lofiKnob.setDoubleClickReturnValue(true, 0.0);
    lofiKnob.setValue(audioProcessor.getLofiAmount() * 100.0f, juce::dontSendNotification);
    lofiKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffffca28));
    lofiKnob.onValueChange = [this] {
        int pct = juce::roundToInt(lofiKnob.getValue());
        audioProcessor.setLofiAmount((float)pct / 100.0f);
        lofiLabel.setText(juce::String::fromUTF8("Lo-Fi 12b: ") + juce::String(pct) + "%", juce::dontSendNotification);
        updateFxAndMpcUiState();
    };
    addAndMakeVisible(lofiKnob);

    lofiLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffe082));
    lofiLabel.setJustificationType(juce::Justification::centredLeft);
    lofiLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(lofiLabel);

    // Perilla 2: Tono / Pitch en Semitonos (-12 st a +12 st) - Doble clic resetea a 0 st
    pitchKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pitchKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    pitchKnob.setRange(-12.0, 12.0, 1.0);
    pitchKnob.setDoubleClickReturnValue(true, 0.0);
    pitchKnob.setValue(audioProcessor.getFxPitchSemitones(), juce::dontSendNotification);
    pitchKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff38bdf8));
    pitchKnob.onValueChange = [this] {
        int st = juce::roundToInt(pitchKnob.getValue());
        audioProcessor.setFxPitchSemitones((float)st);
        juce::String sign = (st > 0) ? "+" : "";
        pitchLabel.setText(juce::String::fromUTF8("Tono: ") + sign + juce::String(st) + " st", juce::dontSendNotification);
        updateFxAndMpcUiState();
    };
    addAndMakeVisible(pitchKnob);

    pitchLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    pitchLabel.setJustificationType(juce::Justification::centredLeft);
    pitchLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(pitchLabel);

    // Perilla 3: Filtro Paso-Bajo Cálido (15% a 100%) - Doble clic resetea a 100%
    toneKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    toneKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    toneKnob.setRange(15.0, 100.0, 1.0);
    toneKnob.setDoubleClickReturnValue(true, 100.0);
    toneKnob.setValue(audioProcessor.getFxToneFilter() * 100.0f, juce::dontSendNotification);
    toneKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00e676));
    toneKnob.onValueChange = [this] {
        int pct = juce::roundToInt(toneKnob.getValue());
        audioProcessor.setFxToneFilter((float)pct / 100.0f);
        toneLabel.setText(juce::String::fromUTF8("Filtro: ") + juce::String(pct) + "%", juce::dontSendNotification);
        updateFxAndMpcUiState();
    };
    addAndMakeVisible(toneKnob);

    toneLabel.setColour(juce::Label::textColourId, juce::Colour(0xff69f0ae));
    toneLabel.setJustificationType(juce::Justification::centredLeft);
    toneLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(toneLabel);

    // Perilla 4: Envolvente de Ataque (Attack) Anti-Chasquidos (0.5 ms a 500 ms) - Doble clic resetea a 2.0 ms
    attackKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    attackKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    attackKnob.setRange(0.5, 500.0, 0.5);
    attackKnob.setSkewFactorFromMidPoint(25.0);
    attackKnob.setDoubleClickReturnValue(true, 2.0);
    attackKnob.setValue(audioProcessor.getEnvelopeAttackMs(), juce::dontSendNotification);
    attackKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff9100));
    attackKnob.onValueChange = [this] {
        float ms = (float)attackKnob.getValue();
        audioProcessor.setEnvelopeAttackMs(ms);
        attackLabel.setText(juce::String::fromUTF8("Atk: ") + juce::String(ms, ms < 10.0f ? 1 : 0) + " ms", juce::dontSendNotification);
    };
    addAndMakeVisible(attackKnob);

    attackLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffb74d));
    attackLabel.setJustificationType(juce::Justification::centredLeft);
    attackLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(attackLabel);

    // Perilla 5: Envolvente de Caída (Release) Suave (5 ms a 2000 ms) - Doble clic resetea a 50.0 ms
    releaseKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    releaseKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    releaseKnob.setRange(5.0, 2000.0, 1.0);
    releaseKnob.setSkewFactorFromMidPoint(120.0);
    releaseKnob.setDoubleClickReturnValue(true, 50.0);
    releaseKnob.setValue(audioProcessor.getEnvelopeReleaseMs(), juce::dontSendNotification);
    releaseKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff4081));
    releaseKnob.onValueChange = [this] {
        float ms = (float)releaseKnob.getValue();
        audioProcessor.setEnvelopeReleaseMs(ms);
        releaseLabel.setText(juce::String::fromUTF8("Rel: ") + juce::String(juce::roundToInt(ms)) + " ms", juce::dontSendNotification);
    };
    addAndMakeVisible(releaseKnob);

    releaseLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff80ab));
    releaseLabel.setJustificationType(juce::Justification::centredLeft);
    releaseLabel.setMinimumHorizontalScale(0.68f);
    addAndMakeVisible(releaseLabel);

    // Botones de Sincronización BPM, Rebanador MPC, Aplicar Efectos e Indicador BPM/Tono
    autoSyncButton.onClick = [this] { onToggleAutoSyncClicked(); };
    addAndMakeVisible(autoSyncButton);

    autoChopButton.onClick = [this] { onToggleAutoChopClicked(); };
    addAndMakeVisible(autoChopButton);

    bakeFxButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff334b73));
    bakeFxButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffd54f));
    bakeFxButton.onClick = [this] { onBakeFxClicked(); };
    addAndMakeVisible(bakeFxButton);

    bpmKeyBadgeLabel.setJustificationType(juce::Justification::centred);
    bpmKeyBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff192742));
    bpmKeyBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    bpmKeyBadgeLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff385382));
    bpmKeyBadgeLabel.setMinimumHorizontalScale(0.65f);
    addAndMakeVisible(bpmKeyBadgeLabel);

    addAndMakeVisible(originalWaveformComponent);
    addAndMakeVisible(transformedWaveformComponent);

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00c853));
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    playButton.onClick = [this] { onPlayClicked(); };
    addAndMakeVisible(playButton);

    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff32466e));
    stopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    stopButton.onClick = [this] { onStopClicked(); };
    addAndMakeVisible(stopButton);

    exactCutButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0097a7));
    exactCutButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    exactCutButton.onClick = [this] { onExactCutClicked(); };
    exactCutButton.addMouseListener(this, false);
    addAndMakeVisible(exactCutButton);

    padReverseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff263759));
    padReverseButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
    padReverseButton.onClick = [this] { onPadReverseClicked(); };
    addAndMakeVisible(padReverseButton);

    padHalfTimeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff263759));
    padHalfTimeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
    padHalfTimeButton.onClick = [this] { onPadHalfTimeClicked(); };
    addAndMakeVisible(padHalfTimeButton);

    dragDawButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6200ea));
    dragDawButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    dragDawButton.onClick = [this] { onDragDawClicked(); };
    dragDawButton.addMouseListener(this, false);
    addAndMakeVisible(dragDawButton);

    dragMidiButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6d00));
    dragMidiButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    dragMidiButton.onClick = [this] { onDragMidiClicked(); };
    dragMidiButton.addMouseListener(this, false);
    addAndMakeVisible(dragMidiButton);

    exportWavButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a73e8));
    exportWavButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    exportWavButton.onClick = [this] { onExportWavClicked(); };
    addAndMakeVisible(exportWavButton);

    addChildComponent(licenseOverlay);
    addChildComponent(freesoundOverlay);
    updateLicenseUiState();
    updatePlatformUiState();
    updateFxAndMpcUiState();

    // =========================================================================
    // Ergonomía HIG (desktop-ergonomics-hig): Cursores interactivos y Tooltips con atajos
    // =========================================================================
    juce::Component* interactiveControls[] = {
        &licenseBadgeButton, &freesoundButton, &updateExtractorButton, &cleanCacheButton,
        &loadLocalFileButton, &usoLibreButton, &platformYoutubeBtn, &platformTiktokBtn, &platformInstagramBtn,
        &pasteButton, &detectDurationButton, &startTimeSlider, &extractModeButton,
        &extract3sButton, &extract10sButton, &extract30sButton, &newExtractionButton,
        &tapeWarmthBtn, &derechosPlusBtn, &derechosMinusBtn, &detectCopyrightBtn,
        &fxTargetAudio1Btn, &fxTargetAudio2Btn, &lofiKnob, &pitchKnob, &toneKnob, &attackKnob, &releaseKnob,
        &autoSyncButton, &autoChopButton, &bakeFxButton,
        &playButton, &stopButton, &exactCutButton, &padReverseButton, &padHalfTimeButton,
        &dragDawButton, &dragMidiButton, &exportWavButton
    };
    for (auto* comp : interactiveControls)
        comp->setMouseCursor(juce::MouseCursor::PointingHandCursor);

    loadLocalFileButton.setTooltip(juce::String::fromUTF8("Cargar muestra de audio local en el Visor [Atajo: Ctrl+O]"));
    freesoundButton.setTooltip(juce::String::fromUTF8("Abrir explorador de muestras Freesound API v2 [Atajo: Ctrl+F]"));
    playButton.setTooltip(juce::String::fromUTF8("Reproducir vista previa de la muestra activa [Atajo: Espacio]"));
    stopButton.setTooltip(juce::String::fromUTF8("Detener reproducción [Atajo: Espacio / Esc]"));
    exactCutButton.setTooltip(juce::String::fromUTF8("Seleccionar o arrastrar únicamente la rodaja MPC activa hacia tu DAW"));
    dragDawButton.setTooltip(juce::String::fromUTF8("Mantén presionado y arrastra el archivo .WAV directamente a una pista de tu DAW"));
    dragMidiButton.setTooltip(juce::String::fromUTF8("Convertir muestra a notas MIDI y arrastrar al Piano Roll [Atajo: Ctrl+M]"));
    exportWavButton.setTooltip(juce::String::fromUTF8("Guardar muestra activa como archivo .WAV de 24 bits [Atajo: Ctrl+E]"));
    lofiKnob.setTooltip(juce::String::fromUTF8("Reducción de resolución vintage de 12 bits (0% - 100%) [Doble clic: Reset a 0%]"));
    pitchKnob.setTooltip(juce::String::fromUTF8("Transposición cromática en semitonos (-12 st a +12 st) [Doble clic: Reset a 0 st]"));
    toneKnob.setTooltip(juce::String::fromUTF8("Filtro paso-bajo analógico cálido (15% - 100%) [Doble clic: Reset a 100%]"));
    attackKnob.setTooltip(juce::String::fromUTF8("Tiempo de ataque anti-click (0.5 ms - 500 ms) [Doble clic: Reset a 2 ms]"));
    releaseKnob.setTooltip(juce::String::fromUTF8("Tiempo de caída/relajación de cada nota (5 ms - 2000 ms) [Doble clic: Reset a 50 ms]"));

    // Temporizador a 60 FPS (juce-custom-lookandfeel) para máxima fluidez visual del cabezal y animaciones
    startTimerHz(60);
}

ClipSamplerAudioProcessorEditor::~ClipSamplerAudioProcessorEditor()
{
    stopTimer();
    exactCutButton.removeMouseListener(this);
    dragDawButton.removeMouseListener(this);
    dragMidiButton.removeMouseListener(this);
    setLookAndFeel(nullptr);
}

// ==============================================================================
// Gestión del Selector Multi-Plataforma (YouTube / TikTok / Instagram)
// ==============================================================================
void ClipSamplerAudioProcessorEditor::onSelectPlatform(SourcePlatform platform, bool updateStatusMsg)
{
    audioProcessor.setSelectedPlatform(platform);
    updatePlatformUiState();

    if (updateStatusMsg)
    {
        juce::String pName = AudioExtractorWorker::getPlatformName(platform);
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Plataforma seleccionada: ") + pName
                                        + juce::String::fromUTF8(". Pega un enlace de ") + pName
                                        + juce::String::fromUTF8(" y elige Extraer 3s, 10s o 30s."));
    }
    repaint();
}

void ClipSamplerAudioProcessorEditor::updatePlatformUiState()
{
    const SourcePlatform plat = audioProcessor.getSelectedPlatform();
    const bool webUnlocked = audioProcessor.isWebExtractionUnlocked();

    juce::Colour accentCol(0xff38bdf8);
    if (plat == SourcePlatform::YouTube)
    {
        accentCol = juce::Colour(0xffff2a55);
        urlEditor.setTextToShowWhenEmpty(webUnlocked
            ? "https://www.youtube.com/watch?v=...  o  https://youtu.be/..."
            : juce::String::fromUTF8("Pulsa [⚖️ Uso Libre (Web)] para activar extracción de enlaces..."),
            juce::Colour(0xff94a3b8));
        platformHintLabel.setText(juce::String::fromUTF8("Motor Activo: YouTube / Shorts (Alta Precisión)"), juce::dontSendNotification);
        platformHintLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff8a9f));
    }
    else if (plat == SourcePlatform::TikTok)
    {
        accentCol = juce::Colour(0xff00f2fe);
        urlEditor.setTextToShowWhenEmpty(webUnlocked
            ? "https://www.tiktok.com/@usuario/video/...  o  https://vm.tiktok.com/..."
            : juce::String::fromUTF8("Pulsa [⚖️ Uso Libre (Web)] para activar extracción de enlaces..."),
            juce::Colour(0xff94a3b8));
        platformHintLabel.setText(juce::String::fromUTF8("Motor Activo: TikTok Audio + Cabeceras Anti-Bloqueo"), juce::dontSendNotification);
        platformHintLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7dd3fc));
    }
    else
    {
        accentCol = juce::Colour(0xffe040fb);
        urlEditor.setTextToShowWhenEmpty(webUnlocked
            ? "https://www.instagram.com/reel/...  o  https://www.instagram.com/p/..."
            : juce::String::fromUTF8("Pulsa [⚖️ Uso Libre (Web)] para activar extracción de enlaces..."),
            juce::Colour(0xff94a3b8));
        platformHintLabel.setText(juce::String::fromUTF8("Motor Activo: Instagram Reels / Publicaciones"), juce::dontSendNotification);
        platformHintLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff0abfc));
    }

    scalableLnf.setThemeAccent(accentCol);
    urlEditor.setColour(juce::TextEditor::outlineColourId, accentCol.withAlpha(0.75f));
    progressBar.setColour(juce::ProgressBar::foregroundColourId, accentCol);

    // Colores de alto contraste para los 3 botones de plataforma
    const bool isYt = (plat == SourcePlatform::YouTube);
    platformYoutubeBtn.setColour(juce::TextButton::buttonColourId, isYt ? juce::Colour(0xffff1744) : juce::Colour(0xff263759));
    platformYoutubeBtn.setColour(juce::TextButton::textColourOffId, isYt ? juce::Colours::white : juce::Colour(0xffcbd5e1));

    const bool isTt = (plat == SourcePlatform::TikTok);
    platformTiktokBtn.setColour(juce::TextButton::buttonColourId, isTt ? juce::Colour(0xff00c4cc) : juce::Colour(0xff263759));
    platformTiktokBtn.setColour(juce::TextButton::textColourOffId, isTt ? juce::Colours::black : juce::Colour(0xffcbd5e1));

    const bool isIg = (plat == SourcePlatform::Instagram);
    platformInstagramBtn.setColour(juce::TextButton::buttonColourId, isIg ? juce::Colour(0xffaa00ff) : juce::Colour(0xff263759));
    platformInstagramBtn.setColour(juce::TextButton::textColourOffId, isIg ? juce::Colours::white : juce::Colour(0xffcbd5e1));
}

void ClipSamplerAudioProcessorEditor::refreshVideoTimeRangeLabel()
{
    const int startSec = AudioExtractorWorker::parseTimeToSeconds(startTimeEditor.getText());
    const int totalVideoSec = audioProcessor.getDetectedVideoDurationSeconds();

    juce::String startStr = AudioExtractorWorker::formatSecondsToMMSS(startSec);
    if (totalVideoSec > 0)
    {
        juce::String endStr = AudioExtractorWorker::formatSecondsToMMSS(totalVideoSec);
        videoRangeBadgeLabel.setText(juce::String::fromUTF8("Inicio: ") + startStr
                                     + juce::String::fromUTF8("  |  Fin Video: ") + endStr,
                                     juce::dontSendNotification);
    }
    else
    {
        videoRangeBadgeLabel.setText(juce::String::fromUTF8("Inicio: ") + startStr
                                     + juce::String::fromUTF8("  |  Fin Video: [Auto / Detectar]"),
                                     juce::dontSendNotification);
    }
}

void ClipSamplerAudioProcessorEditor::updateFxAndMpcUiState()
{
    const int targetSlot = audioProcessor.getFxTargetSlot();
    fxTargetAudio1Btn.setColour(juce::TextButton::buttonColourId, (targetSlot == 1) ? juce::Colour(0xff00b0ff) : juce::Colour(0xff263759));
    fxTargetAudio1Btn.setColour(juce::TextButton::textColourOffId, (targetSlot == 1) ? juce::Colours::black : juce::Colour(0xffcbd5e1));

    fxTargetAudio2Btn.setColour(juce::TextButton::buttonColourId, (targetSlot == 2) ? juce::Colour(0xff00e676) : juce::Colour(0xff263759));
    fxTargetAudio2Btn.setColour(juce::TextButton::textColourOffId, (targetSlot == 2) ? juce::Colours::black : juce::Colour(0xffcbd5e1));

    const bool isFxBaked = audioProcessor.isFxActiveForSlot(targetSlot);
    const bool hasPendingTweaks = (audioProcessor.getLofiAmount() > 0.005f
                                   || std::abs(audioProcessor.getFxPitchSemitones()) > 0.1f
                                   || audioProcessor.getFxToneFilter() < 0.98f);

    if (isFxBaked && !hasPendingTweaks)
    {
        bakeFxButton.setButtonText(juce::String::fromUTF8("✓ Efectos Activados"));
        bakeFxButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00e676));
        bakeFxButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff0a192f));
    }
    else
    {
        bakeFxButton.setButtonText(juce::String::fromUTF8("Aplicar FX"));
        bakeFxButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff334b73));
        bakeFxButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffd54f));
    }

    const bool syncOn = audioProcessor.isAutoSyncEnabled();
    autoSyncButton.setButtonText(syncOn ? juce::String::fromUTF8("Sinc. BPM: SÍ") : juce::String::fromUTF8("Sinc. BPM: NO"));
    autoSyncButton.setColour(juce::TextButton::buttonColourId, syncOn ? juce::Colour(0xff00c853) : juce::Colour(0xff283a5e));
    autoSyncButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    const bool chopOn = audioProcessor.isAutoChopEnabled();
    autoChopButton.setButtonText(chopOn ? juce::String::fromUTF8("Rebanador MPC: SÍ") : juce::String::fromUTF8("Rebanador MPC: NO"));
    autoChopButton.setColour(juce::TextButton::buttonColourId, chopOn ? juce::Colour(0xffffab00) : juce::Colour(0xff283a5e));
    autoChopButton.setColour(juce::TextButton::textColourOffId, chopOn ? juce::Colours::black : juce::Colours::white);

    const int curSlot = audioProcessor.getActiveSlot();
    const int selectedSliceIdx = audioProcessor.getSelectedSliceIndex(curSlot);
    if (selectedSliceIdx >= 0)
    {
        const juce::String sliceName = audioProcessor.getSelectedSliceName(curSlot);
        exactCutButton.setButtonText(juce::String::fromUTF8("✂ Corte Exacto (") + sliceName + ")");
        exactCutButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00e5ff));
        exactCutButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);

        dragDawButton.setButtonText(juce::String::fromUTF8("Arrastrar ") + sliceName + " (.WAV)");
    }
    else
    {
        exactCutButton.setButtonText(juce::String::fromUTF8("✂ Corte Exacto"));
        exactCutButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00838f));
        exactCutButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

        dragDawButton.setButtonText(juce::String::fromUTF8("Arrastrar .WAV"));
    }

    // Actualizar botones de extracción según modo Segundos vs Compases
    if (isBarsExtractMode)
    {
        double bpm = audioProcessor.getHostBpm();
        if (bpm <= 20.0)
            bpm = audioProcessor.getDetectedBpm(1);
        if (bpm <= 20.0)
            bpm = 120.0;

        double t1 = 1.0 * 4.0 * (60.0 / bpm);
        double t2 = 2.0 * 4.0 * (60.0 / bpm);
        double t4 = 4.0 * 4.0 * (60.0 / bpm);

        extract3sButton.setButtonText(juce::String::fromUTF8("1 Bar (") + juce::String(t1, 1) + "s)");
        extract10sButton.setButtonText(juce::String::fromUTF8("2 Bars (") + juce::String(t2, 1) + "s)");
        extract30sButton.setButtonText(juce::String::fromUTF8("4 Bars (") + juce::String(t4, 1) + "s)");
    }
    else
    {
        extract3sButton.setButtonText(juce::String::fromUTF8("Extraer 3 Segundos"));
        extract10sButton.setButtonText(juce::String::fromUTF8("Extraer 10 Segundos"));
        extract30sButton.setButtonText(juce::String::fromUTF8("Extraer 30 Segundos"));
    }

    // Actualizar botones de Reverse y Half-Time (según si hay un Corte Exacto con doble clic o Todo el Audio con 1 clic)
    if (selectedSliceIdx >= 0)
    {
        const bool isRev = audioProcessor.isSliceReversed(curSlot, selectedSliceIdx);
        padReverseButton.setButtonText(isRev ? juce::String::fromUTF8("◀ Rev: SÍ") : juce::String::fromUTF8("◀ Rev"));
        padReverseButton.setColour(juce::TextButton::buttonColourId, isRev ? juce::Colour(0xffd97706) : juce::Colour(0xff263759));
        padReverseButton.setColour(juce::TextButton::textColourOffId, isRev ? juce::Colours::white : juce::Colour(0xffcbd5e1));

        const bool isHalf = audioProcessor.isSliceHalfTime(curSlot, selectedSliceIdx);
        padHalfTimeButton.setButtonText(isHalf ? juce::String::fromUTF8("⏳ 1/2: SÍ") : juce::String::fromUTF8("⏳ 1/2"));
        padHalfTimeButton.setColour(juce::TextButton::buttonColourId, isHalf ? juce::Colour(0xff7c3aed) : juce::Colour(0xff263759));
        padHalfTimeButton.setColour(juce::TextButton::textColourOffId, isHalf ? juce::Colours::white : juce::Colour(0xffcbd5e1));
    }
    else
    {
        const bool isFullRev = audioProcessor.isFullAudioReversed(curSlot);
        padReverseButton.setButtonText(isFullRev ? juce::String::fromUTF8("◀ Rev: SÍ") : juce::String::fromUTF8("◀ Rev"));
        padReverseButton.setColour(juce::TextButton::buttonColourId, isFullRev ? juce::Colour(0xffd97706) : juce::Colour(0xff263759));
        padReverseButton.setColour(juce::TextButton::textColourOffId, isFullRev ? juce::Colours::white : juce::Colour(0xffcbd5e1));

        const bool isFullHalf = audioProcessor.isFullAudioHalfTime(curSlot);
        padHalfTimeButton.setButtonText(isFullHalf ? juce::String::fromUTF8("⏳ 1/2: SÍ") : juce::String::fromUTF8("⏳ 1/2"));
        padHalfTimeButton.setColour(juce::TextButton::buttonColourId, isFullHalf ? juce::Colour(0xff7c3aed) : juce::Colour(0xff263759));
        padHalfTimeButton.setColour(juce::TextButton::textColourOffId, isFullHalf ? juce::Colours::white : juce::Colour(0xffcbd5e1));
    }

    const double detBpm = audioProcessor.getDetectedBpm(curSlot);
    const juce::String detKey = audioProcessor.getDetectedKey(curSlot);
    const double hostBpm = audioProcessor.getHostBpm();

    if (detBpm > 10.0)
    {
        bpmKeyBadgeLabel.setText(juce::String(detBpm, 1)
                                 + juce::String::fromUTF8(" BPM | Tono: ") + detKey
                                 + " | DAW: " + juce::String(hostBpm, 0),
                                 juce::dontSendNotification);
    }
    else
    {
        bpmKeyBadgeLabel.setText(juce::String::fromUTF8("-- BPM | Tono: -- | DAW: ") + juce::String(hostBpm, 0),
                                 juce::dontSendNotification);
    }
}

// ==============================================================================
// Temporizador de Sincronización con el Procesador DSP (30 Hz)
// ==============================================================================
void ClipSamplerAudioProcessorEditor::timerCallback()
{
    // 1. Actualización fluida a 60 FPS del cabezal de reproducción cuando está sonando
    if (audioProcessor.isAudioPlaying())
    {
        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }

    // Sincronizamos el resto de estados de controles a 30 Hz (cada 2 ticks de 60 Hz) para máxima eficiencia CPU
    if ((++uiSyncTickCounter & 1) != 0)
        return;

    if (++licenseCheckTickCounter >= 150)
    {
        licenseCheckTickCounter = 0;
        audioProcessor.getLicenseManager().verifyAndSyncState();
        updateLicenseUiState();
    }

    const bool unlocked = audioProcessor.getLicenseManager().isUnlocked();
    const bool extracting = audioProcessor.isExtractionRunning();
    const bool updating = audioProcessor.isUpdateRunning();
    const bool fetchingMeta = audioProcessor.isMetadataFetchRunning();
    const bool bothAudiosReady = audioProcessor.hasBothAudiosReady();

    const bool webUnlocked = audioProcessor.isWebExtractionUnlocked();

    loadLocalFileButton.setEnabled(unlocked);
    usoLibreButton.setEnabled(unlocked);

    if (webUnlocked)
    {
        usoLibreButton.setButtonText(juce::String::fromUTF8("✅ Uso Libre (Web Activo)"));
        usoLibreButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff065f46));
        usoLibreButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff6ee7b7));
    }
    else
    {
        usoLibreButton.setButtonText(juce::String::fromUTF8("⚖️ Uso Libre (Web)"));
        usoLibreButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff334155));
        usoLibreButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
    }

    platformYoutubeBtn.setEnabled(unlocked && webUnlocked);
    platformTiktokBtn.setEnabled(unlocked && webUnlocked);
    platformInstagramBtn.setEnabled(unlocked && webUnlocked);
    urlEditor.setEnabled(unlocked && webUnlocked);
    pasteButton.setEnabled(unlocked && webUnlocked);
    detectDurationButton.setEnabled(unlocked && webUnlocked && !fetchingMeta);
    startTimeSlider.setEnabled(unlocked && webUnlocked);
    startTimeEditor.setEnabled(unlocked && webUnlocked);
    extractModeButton.setEnabled(unlocked && webUnlocked);
    extract3sButton.setEnabled(unlocked && webUnlocked && !extracting);
    extract10sButton.setEnabled(unlocked && webUnlocked && !extracting);
    extract30sButton.setEnabled(unlocked && webUnlocked && !extracting);
    updateExtractorButton.setEnabled(unlocked && !updating);

    const bool hasAudio1 = audioProcessor.hasSlotAudio(1);
    const bool hasAudio2 = audioProcessor.hasSlotAudio(2);
    tapeWarmthBtn.setEnabled(unlocked && !extracting && hasAudio1);
    derechosPlusBtn.setEnabled(unlocked && !extracting && hasAudio1);
    derechosMinusBtn.setEnabled(unlocked && !extracting && hasAudio1);
    detectCopyrightBtn.setEnabled(unlocked && !extracting && hasAudio1);

    // El botón "Nueva Extracción" se habilita inmediatamente al tener el primer audio extraído
    const bool canStartNew = (hasAudio1 || hasAudio2);
    newExtractionButton.setEnabled(unlocked && !extracting && canStartNew);
    if (canStartNew)
    {
        newExtractionButton.setButtonText(juce::String::fromUTF8("🔄 Nueva Extracción"));
        newExtractionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00b0ff));
        newExtractionButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    }
    else
    {
        newExtractionButton.setButtonText(juce::String::fromUTF8("🔄 Nueva Extracción"));
        newExtractionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff263759));
        newExtractionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff94a3b8));
    }

    updateFxAndMpcUiState();

    // Actualiza automáticamente el rango del deslizador cuando se detecta la duración del video
    const int detectedDur = audioProcessor.getDetectedVideoDurationSeconds();
    if (detectedDur > 0 && detectedDur != lastKnownVideoDuration)
    {
        lastKnownVideoDuration = detectedDur;
        startTimeSlider.setRange(0.0, static_cast<double>(juce::jmax(10, detectedDur)), 1.0);
        startTimeSlider.setSkewFactorFromMidPoint(juce::jlimit(15.0, 180.0, detectedDur * 0.35));
        refreshVideoTimeRangeLabel();
    }

    const double latestProgress = static_cast<double>(audioProcessor.getExtractionProgress());
    if (std::abs(currentProgressValue - latestProgress) > 0.001)
    {
        currentProgressValue = latestProgress;
        progressBar.repaint();
    }

    const juce::String latestStatus = audioProcessor.getStatusMessage();
    if (latestStatus != lastDisplayedStatus)
    {
        lastDisplayedStatus = latestStatus;
        statusLabel.setText(lastDisplayedStatus, juce::dontSendNotification);
        copyrightBadgeLabel.setText(audioProcessor.getCopyrightBadgeText(), juce::dontSendNotification);

        int verifyState = audioProcessor.getCopyrightVerifyState();
        if (verifyState == 2)
            copyrightBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff69f0ae));
        else if (verifyState == 1)
            copyrightBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff5252));
        else
            copyrightBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffd54f));

        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }

    if (audioProcessor.isAudioPlaying())
    {
        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }
}

// ==============================================================================
// Acciones de Botones de Extracción, Derechos IA, Efectos y Exportación
// ==============================================================================
bool ClipSamplerAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".aiff" || ext == ".aif" || ext == ".flac" || ext == ".ogg")
            return true;
    }
    return false;
}

void ClipSamplerAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& f : files)
    {
        juce::File file(f);
        auto ext = file.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".aiff" || ext == ".aif" || ext == ".flac" || ext == ".ogg")
        {
            loadLocalAudioFile(file);
            break;
        }
    }
}

void ClipSamplerAudioProcessorEditor::loadLocalAudioFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    if (audioProcessor.loadAudioFileIntoSlot(file, 1))
    {
        audioProcessor.setAudioOriginLabel(1, file.getFileName().startsWithIgnoreCase("freesound_") ? "FREESOUND" : "LOCAL");
        audioProcessor.setStatusMessage(juce::String::fromUTF8("¡Audio local cargado con éxito en Visor 1!: ") + file.getFileName());
        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
        updateFxAndMpcUiState();
        timerCallback();
    }
    else
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Error al leer el archivo de audio. Formatos soportados: WAV, MP3, AIFF, FLAC, OGG."));
    }
}

void ClipSamplerAudioProcessorEditor::onLoadLocalFileClicked()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        juce::String::fromUTF8("Selecciona un archivo de audio (WAV, MP3, AIFF, FLAC, OGG)..."),
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.mp3;*.aiff;*.aif;*.flac;*.ogg");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto result = fc.getResult();
        if (result.existsAsFile())
            loadLocalAudioFile(result);
    });
}

void ClipSamplerAudioProcessorEditor::onUsoLibreClicked()
{
    if (audioProcessor.isWebExtractionUnlocked())
    {
        audioProcessor.setWebExtractionUnlocked(false);
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Modo Sampler Local activado. Extracción Web bloqueada."));
        timerCallback();
        return;
    }

    auto options = juce::MessageBoxOptions()
        .withIconType(juce::MessageBoxIconType::WarningIcon)
        .withTitle(juce::String::fromUTF8("⚖️ ACUERDO DE USUARIO, USO LIBRE Y DESCARGO DE RESPONSABILIDAD LEGAL"))
        .withMessage(juce::String::fromUTF8(
            "CLIPSAMPLER v2.6.1 es un Instrumento Musical Virtual (VST3/AU) y Sampler de Transformación de Audio Local.\n\n"
            "La función de extracción y lectura de URLs externas (YouTube, TikTok, Instagram) es una "
            "herramienta técnica opcional de USO LIBRE destinada exclusivamente a material de Dominio Público, "
            "licencias Creative Commons (CC0 / CC-BY), contenido propio o material con expresa autorización de su titular.\n\n"
            "AL ACTIVAR ESTA FUNCIÓN DECLARAS Y ACEPTAS EXPRESAMENTE:\n"
            "1. Que el precio de la licencia del software corresponde únicamente al motor DSP e interfaz gráfica; "
            "AudioInnovations NO vende, aloja ni comercializa fonogramas ni contenidos de terceros.\n"
            "2. Que la procedencia, legalidad y derechos de autor (Copyright) de cualquier audio cargado o extraído "
            "recaen bajo TU TOTAL Y EXCLUSIVA RESPONSABILIDAD LEGAL.\n"
            "3. Que ClipSampler, AudioInnovations y sus desarrolladores quedan 100% exentos de cualquier responsabilidad "
            "civil, mercantil o penal derivada del uso comercial o no comercial que el usuario realice con el software.\n\n"
            "¿Deseas aceptar el Acuerdo de Usuario y activar la función de Uso Libre (Web)?"))
        .withButton(juce::String::fromUTF8("Aceptar Acuerdo y Activar"))
        .withButton(juce::String::fromUTF8("Cancelar (Modo Local Seguro)"))
        .withAssociatedComponent(this);

    juce::AlertWindow::showAsync(options, [this](int result) {
        if (result == 1) // Aceptar
        {
            audioProcessor.setWebExtractionUnlocked(true);
            audioProcessor.setStatusMessage(juce::String::fromUTF8("✅ Modo Uso Libre (Web) activado bajo tu total responsabilidad."));
        }
        else
        {
            audioProcessor.setWebExtractionUnlocked(false);
            audioProcessor.setStatusMessage(juce::String::fromUTF8("Permaneciendo en Modo Sampler de Transformación Local Seguro."));
        }
        timerCallback();
    });
}

void ClipSamplerAudioProcessorEditor::onPasteClicked()
{
    juce::String clip = juce::SystemClipboard::getTextFromClipboard().trim();
    if (clip.isNotEmpty())
    {
        SourcePlatform detectedPlat = AudioExtractorWorker::detectPlatformFromUrl(clip);
        onSelectPlatform(detectedPlat, false);

        urlEditor.setText(clip, false);
        audioProcessor.detectVideoDuration(clip);
    }
}

void ClipSamplerAudioProcessorEditor::onDetectDurationClicked()
{
    juce::String url = urlEditor.getText().trim();
    if (url.isEmpty())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero pega un enlace de YouTube, TikTok o Instagram para detectar su duración."));
        return;
    }
    audioProcessor.detectVideoDuration(url);
}

void ClipSamplerAudioProcessorEditor::onToggleExtractModeClicked()
{
    isBarsExtractMode = !isBarsExtractMode;
    if (isBarsExtractMode)
    {
        extractModeButton.setButtonText(juce::String::fromUTF8("🎵 Compases"));
        extractModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff5b21b6));
        extractModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4b5fd));
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Modo Compases (Bars) ACTIVADO: Duraciones sincronizadas con el tempo musical (BPM)."));
    }
    else
    {
        extractModeButton.setButtonText(juce::String::fromUTF8("⏱️ Segundos"));
        extractModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e3a5f));
        extractModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7dd3fc));
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Modo Segundos ACTIVADO: Extracciones fijas de 3s, 10s y 30s."));
    }
    updateFxAndMpcUiState();
}

void ClipSamplerAudioProcessorEditor::onExtractDurationClicked(int opt)
{
    juce::String url = urlEditor.getText().trim();
    if (url.isEmpty())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Ingresa un enlace válido de YouTube, TikTok o Instagram."));
        statusLabel.setText(audioProcessor.getStatusMessage(), juce::dontSendNotification);
        return;
    }

    int durationSec = opt;
    if (isBarsExtractMode)
    {
        double bpm = audioProcessor.getHostBpm();
        if (bpm <= 20.0)
            bpm = audioProcessor.getDetectedBpm(1);
        if (bpm <= 20.0)
            bpm = 120.0;

        int bars = (opt == 3) ? 1 : ((opt == 10) ? 2 : 4);
        double durDbl = (double)bars * 4.0 * (60.0 / bpm);
        durationSec = juce::jlimit(1, 60, juce::roundToInt(durDbl));
    }

    audioProcessor.startSampleExtraction(url, startTimeEditor.getText(), durationSec);
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onNewExtractionClicked()
{
    if (!audioProcessor.hasSlotAudio(1) && !audioProcessor.hasSlotAudio(2))
        return;

    audioProcessor.resetForNewExtraction();

    urlEditor.clear();
    startTimeEditor.setText("00:00", false);
    startTimeSlider.setRange(0.0, 600.0, 1.0);
    startTimeSlider.setValue(0.0, juce::dontSendNotification);
    lastKnownVideoDuration = -1;
    refreshVideoTimeRangeLabel();

    lofiKnob.setValue(0.0, juce::dontSendNotification);
    lofiLabel.setText(juce::String::fromUTF8("Lo-Fi: 0%"), juce::dontSendNotification);
    pitchKnob.setValue(0.0, juce::dontSendNotification);
    pitchLabel.setText(juce::String::fromUTF8("Tono: 0 st"), juce::dontSendNotification);
    toneKnob.setValue(100.0, juce::dontSendNotification);
    toneLabel.setText(juce::String::fromUTF8("Filtro: 100%"), juce::dontSendNotification);
    attackKnob.setValue(2.0, juce::dontSendNotification);
    attackLabel.setText(juce::String::fromUTF8("Atk: 2 ms"), juce::dontSendNotification);
    releaseKnob.setValue(50.0, juce::dontSendNotification);
    releaseLabel.setText(juce::String::fromUTF8("Rel: 50 ms"), juce::dontSendNotification);

    currentProgressValue = 0.0;
    progressBar.repaint();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onTapeWarmthClicked()
{
    juce::String report;
    if (audioProcessor.applySubtleColor(ClipSamplerAudioProcessor::SubtleColorType::TapeWarmth, report))
    {
        updateFxAndMpcUiState();
        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onCheckCopyrightOriginalClicked()
{
    juce::String report;
    const int curSlot = audioProcessor.getActiveSlot();
    if (curSlot == 2 && audioProcessor.hasSlotAudio(2))
        audioProcessor.verifyTransformedAudioCopyrightFree(report);
    else
        audioProcessor.analyzeOriginalCopyright(report);
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onApplyDerechosClicked(int semitones)
{
    juce::String report;
    audioProcessor.applyDerechosTransformation(semitones, report);
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onVerifyDerechosFreeClicked()
{
    juce::String report;
    audioProcessor.verifyTransformedAudioCopyrightFree(report);
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onSelectFxTargetSlot(int slotIndex)
{
    audioProcessor.setFxTargetSlot(slotIndex);
    updateFxAndMpcUiState();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
    audioProcessor.setStatusMessage(juce::String::fromUTF8("Destino de Efectos y MIDI cambiado a: Audio ")
                                    + juce::String(slotIndex)
                                    + (slotIndex == 2 ? juce::String::fromUTF8(" (Modificado Derechos IA)") : juce::String::fromUTF8(" (Original)")));
}

void ClipSamplerAudioProcessorEditor::onToggleAutoChopClicked()
{
    bool nextState = !audioProcessor.isAutoChopEnabled();
    audioProcessor.setAutoChopEnabled(nextState);
    updateFxAndMpcUiState();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
    audioProcessor.setStatusMessage(nextState
        ? juce::String::fromUTF8("Rebanador MPC ACTIVADO: Las rodajas rítmicas se han asignado de C1 a G2 en tu teclado MIDI y en el visor.")
        : juce::String::fromUTF8("Modo Cromático ACTIVADO: El sample completo suena cromáticamente en el teclado MIDI."));
}

void ClipSamplerAudioProcessorEditor::onToggleAutoSyncClicked()
{
    bool nextState = !audioProcessor.isAutoSyncEnabled();
    audioProcessor.setAutoSyncEnabled(nextState);
    updateFxAndMpcUiState();
    int curSlot = audioProcessor.getActiveSlot();
    audioProcessor.setStatusMessage(nextState
        ? (juce::String::fromUTF8("Sincronización BPM ACTIVADA: Ajustando sample (")
           + juce::String(audioProcessor.getDetectedBpm(curSlot), 1) + " BPM) al tempo del DAW ("
           + juce::String(audioProcessor.getHostBpm(), 1) + " BPM).")
        : juce::String::fromUTF8("Sincronización BPM DESACTIVADA: Reproducción al tempo natural del sample."));
}

void ClipSamplerAudioProcessorEditor::onBakeFxClicked()
{
    const int targetSlot = audioProcessor.getFxTargetSlot();
    const bool isFxBaked = audioProcessor.isFxActiveForSlot(targetSlot);
    const bool hasPendingTweaks = (audioProcessor.getLofiAmount() > 0.005f
                                   || std::abs(audioProcessor.getFxPitchSemitones()) > 0.1f
                                   || audioProcessor.getFxToneFilter() < 0.98f);

    if (isFxBaked && !hasPendingTweaks)
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("✓ Los Efectos ya se encuentran ACTIVADOS e impresos en el Audio ")
                                        + juce::String(targetSlot) + ".");
        return;
    }

    juce::String report;
    if (audioProcessor.applyFxMacroToTargetSlot(report))
    {
        lofiKnob.setValue(0.0, juce::dontSendNotification);
        lofiLabel.setText(juce::String::fromUTF8("Lo-Fi 12b: 0%"), juce::dontSendNotification);
        pitchKnob.setValue(0.0, juce::dontSendNotification);
        pitchLabel.setText(juce::String::fromUTF8("Tono: 0 st"), juce::dontSendNotification);
        toneKnob.setValue(100.0, juce::dontSendNotification);
        toneLabel.setText(juce::String::fromUTF8("Filtro: 100%"), juce::dontSendNotification);

        updateFxAndMpcUiState();
        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onPlayClicked()
{
    audioProcessor.startPlayback();
}

void ClipSamplerAudioProcessorEditor::onStopClicked()
{
    audioProcessor.stopPlayback();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
}

void ClipSamplerAudioProcessorEditor::onExactCutClicked()
{
    const int curSlot = audioProcessor.getActiveSlot();
    if (!audioProcessor.hasSlotAudio(curSlot))
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero debes extraer una muestra para seleccionar o arrastrar un Corte Exacto."));
        return;
    }

    // Si aún no hay un trozo seleccionado en este visor, seleccionamos automáticamente la primera nota (C1)
    // o la última nota tocada, para que el usuario pueda escucharla o arrastrarla de inmediato.
    int selSlice = audioProcessor.getSelectedSliceIndex(curSlot);
    ChopSlice slice;

    if (selSlice < 0)
    {
        int targetSlice = audioProcessor.getActivePlayingSlice();
        if (targetSlice < 0)
            targetSlice = 0; // C1 por defecto

        if (audioProcessor.selectSliceByIndex(curSlot, targetSlice, slice))
        {
            audioProcessor.triggerChopSlice(targetSlice);
            updateFxAndMpcUiState();
            originalWaveformComponent.repaint();
            transformedWaveformComponent.repaint();
        }
    }
    else
    {
        // Si ya hay un corte exacto seleccionado (ej. C1), lo reproducimos y recordamos que puede arrastrar el botón
        audioProcessor.triggerChopSlice(selSlice);
        const juce::String sName = audioProcessor.getSelectedSliceName(curSlot);
        audioProcessor.setStatusMessage(
            juce::String::fromUTF8("✂ CORTE EXACTO ACTIVO [") + sName
            + juce::String::fromUTF8("]: Arrastra este botón '✂ Corte Exacto' o el visor hacia tu DAW para pasar SOLO el trozo ")
            + sName + juce::String::fromUTF8(" (o haz 1 clic en la onda para seleccionar todo el audio)."));
        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }
}

void ClipSamplerAudioProcessorEditor::onPadReverseClicked()
{
    const int curSlot = audioProcessor.getActiveSlot();
    if (!audioProcessor.hasSlotAudio(curSlot))
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero carga o extrae un audio para invertirlo (Reverse)."));
        return;
    }

    const int selSlice = audioProcessor.getSelectedSliceIndex(curSlot);
    if (selSlice >= 0)
    {
        // Si el usuario seleccionó un corte exacto con doble clic (ej. C1, D1), invierte SOLO ese corte
        audioProcessor.toggleSliceReverse(curSlot, selSlice);
        audioProcessor.triggerChopSlice(selSlice);
    }
    else
    {
        // Si el usuario hizo 1 solo clic en el audio (Todo el Audio seleccionado), invierte TODO el audio completo
        audioProcessor.toggleFullAudioReverse(curSlot);
        audioProcessor.startPlayback();
    }
    updateFxAndMpcUiState();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
}

void ClipSamplerAudioProcessorEditor::onPadHalfTimeClicked()
{
    const int curSlot = audioProcessor.getActiveSlot();
    if (!audioProcessor.hasSlotAudio(curSlot))
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero carga o extrae un audio para aplicar Media Velocidad (Half-Time)."));
        return;
    }

    const int selSlice = audioProcessor.getSelectedSliceIndex(curSlot);
    if (selSlice >= 0)
    {
        // Si el usuario seleccionó un corte exacto con doble clic, aplica Half-Time SOLO a ese corte
        audioProcessor.toggleSliceHalfTime(curSlot, selSlice);
        audioProcessor.triggerChopSlice(selSlice);
    }
    else
    {
        // Si el usuario hizo 1 solo clic en el audio (Todo el Audio seleccionado), aplica Half-Time a TODO el audio
        audioProcessor.toggleFullAudioHalfTime(curSlot);
        audioProcessor.startPlayback();
    }
    updateFxAndMpcUiState();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
}

void ClipSamplerAudioProcessorEditor::onDragDawClicked()
{
    const int slot = audioProcessor.getActiveSlot();
    if (!audioProcessor.hasSlotAudio(slot))
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero debes extraer una muestra para poder arrastrar el .WAV al DAW."));
        return;
    }

    const int selSlice = audioProcessor.getSelectedSliceIndex(slot);
    if (selSlice >= 0)
    {
        const juce::String sName = audioProcessor.getSelectedSliceName(slot);
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Mantén presionado 'Arrastrar ") + sName
                                        + juce::String::fromUTF8(" (.WAV)' o '✂ Corte Exacto' y suéltalo en tu DAW para pasar SOLO la nota ")
                                        + sName + ".");
    }
    else
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Mantén presionado 'Arrastrar .WAV' y suéltalo en una pista de audio de tu DAW. [Activo: Audio ")
                                        + juce::String(slot) + juce::String::fromUTF8(" Completo]"));
    }
}

void ClipSamplerAudioProcessorEditor::onDragMidiClicked()
{
    const int curSlot = audioProcessor.getActiveSlot();
    if (!audioProcessor.hasSlotAudio(curSlot))
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero debes extraer una muestra de audio para convertirla y arrastrar sus notas MIDI al Piano Roll."));
        return;
    }

    auto midiFile = audioProcessor.generateMidiFileForSlot(curSlot);
    if (midiFile.existsAsFile())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("¡Audio convertido a MIDI! Mantén presionado '🎹 Convertir a MIDI' y arrástralo al Piano Roll de cualquier sintetizador."));
    }
}

void ClipSamplerAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (event.eventComponent == &exactCutButton && event.getDistanceFromDragStart() > 4)
    {
        const int curSlot = audioProcessor.getActiveSlot();
        if (!audioProcessor.hasSlotAudio(curSlot))
            return;

        if (audioProcessor.getLofiAmount() > 0.01f
            || std::abs(audioProcessor.getFxPitchSemitones()) > 0.1f
            || audioProcessor.getFxToneFilter() < 0.98f)
        {
            juce::String rep;
            audioProcessor.applyFxMacroToTargetSlot(rep);
        }

        // Si el usuario arrastra directamente desde el botón "✂ Corte Exacto" sin haber hecho doble clic antes,
        // seleccionamos automáticamente el corte C1 (o el último corte tocado) y generamos su .WAV exacto.
        if (audioProcessor.getSelectedSliceIndex(curSlot) < 0)
        {
            int targetSlice = audioProcessor.getActivePlayingSlice();
            if (targetSlice < 0)
                targetSlice = 0; // C1 por defecto
            ChopSlice slice;
            audioProcessor.selectSliceByIndex(curSlot, targetSlice, slice);
            updateFxAndMpcUiState();
            originalWaveformComponent.repaint();
            transformedWaveformComponent.repaint();
        }

        auto sliceFile = audioProcessor.getOrCreateSelectedSliceWavFile(curSlot);
        if (sliceFile.existsAsFile())
        {
            juce::StringArray files;
            files.add(sliceFile.getFullPathName());
            performExternalDragDropOfFiles(files, false, &exactCutButton);
        }
    }
    else if (event.eventComponent == &dragDawButton && event.getDistanceFromDragStart() > 4)
    {
        if (audioProcessor.getLofiAmount() > 0.01f
            || std::abs(audioProcessor.getFxPitchSemitones()) > 0.1f
            || audioProcessor.getFxToneFilter() < 0.98f)
        {
            juce::String rep;
            audioProcessor.applyFxMacroToTargetSlot(rep);
        }

        const int curSlot = audioProcessor.getActiveSlot();
        juce::File fileToDrag = audioProcessor.getOrCreateSelectedSliceWavFile(curSlot);
        if (!fileToDrag.existsAsFile())
            fileToDrag = audioProcessor.getCurrentAudioFile();

        if (fileToDrag.existsAsFile())
        {
            juce::StringArray files;
            files.add(fileToDrag.getFullPathName());
            performExternalDragDropOfFiles(files, false, &dragDawButton);
        }
    }
    else if (event.eventComponent == &dragMidiButton && event.getDistanceFromDragStart() > 4)
    {
        const int curSlot = audioProcessor.getActiveSlot();
        if (!audioProcessor.hasSlotAudio(curSlot))
            return;

        auto midiFile = audioProcessor.generateMidiFileForSlot(curSlot);
        if (midiFile.existsAsFile())
        {
            juce::StringArray files;
            files.add(midiFile.getFullPathName());
            performExternalDragDropOfFiles(files, false, &dragMidiButton);
        }
    }
}

bool ClipSamplerAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    // Si el usuario está escribiendo un enlace o tiempo en una caja de texto, solo interceptamos Escape
    if (urlEditor.hasKeyboardFocus(true) || startTimeEditor.hasKeyboardFocus(true))
    {
        if (key == juce::KeyPress::escapeKey)
        {
            grabKeyboardFocus();
            return true;
        }
        return false;
    }

    // Escape: cierra modales abiertos o detiene la reproducción
    if (key == juce::KeyPress::escapeKey)
    {
        if (freesoundOverlay.isVisible())
        {
            freesoundOverlay.setVisible(false);
            return true;
        }
        if (licenseOverlay.isVisible() && audioProcessor.getLicenseManager().isUnlocked())
        {
            licenseOverlay.setVisible(false);
            return true;
        }
        if (audioProcessor.isAudioPlaying())
        {
            onStopClicked();
            return true;
        }
        return false;
    }

    // Barra Espaciadora: Reproducir / Detener vista previa
    if (key == juce::KeyPress::spaceKey)
    {
        if (audioProcessor.isAudioPlaying())
            onStopClicked();
        else
            onPlayClicked();

        return true;
    }

    // Atajos de teclado con Ctrl (Windows) o Cmd (macOS) - desktop-ergonomics-hig
    if (key.getModifiers().isCommandDown())
    {
        const auto ch = key.getTextCharacter();
        if (ch == 'f' || ch == 'F' || key.getKeyCode() == 'F')
        {
            onFreesoundButtonClicked();
            return true;
        }
        if (ch == 'o' || ch == 'O' || key.getKeyCode() == 'O')
        {
            onLoadLocalFileClicked();
            return true;
        }
        if (ch == 'm' || ch == 'M' || key.getKeyCode() == 'M')
        {
            onDragMidiClicked();
            return true;
        }
        if (ch == 'e' || ch == 'E' || key.getKeyCode() == 'E')
        {
            onExportWavClicked();
            return true;
        }
    }

    // Teclas numéricas 1 a 8: Disparo en vivo de las primeras 8 rodajas MPC (C1 a G1) desde el teclado del PC
    if (!key.getModifiers().isAnyModifierKeyDown())
    {
        const auto ch = key.getTextCharacter();
        if (ch >= '1' && ch <= '8')
        {
            const int sliceIdx = (int)(ch - '1');
            const int curSlot = audioProcessor.getActiveSlot();
            if (audioProcessor.hasSlotAudio(curSlot))
            {
                audioProcessor.triggerChopSlice(sliceIdx);
                originalWaveformComponent.repaint();
                transformedWaveformComponent.repaint();
                return true;
            }
        }
    }

    return false;
}

void ClipSamplerAudioProcessorEditor::onUpdateExtractorClicked()
{
    audioProcessor.startExtractorUpdate();
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onCleanCacheClicked()
{
    audioProcessor.cleanAudioCache();
    timerCallback();
}

void ClipSamplerAudioProcessorEditor::onLicenseBadgeClicked()
{
    licenseOverlay.refreshState();
    licenseOverlay.setVisible(true);
    licenseOverlay.toFront(true);
}

void ClipSamplerAudioProcessorEditor::onFreesoundButtonClicked()
{
    freesoundOverlay.refreshView();
    freesoundOverlay.setVisible(true);
    freesoundOverlay.toFront(true);
}

void ClipSamplerAudioProcessorEditor::updateLicenseUiState()
{
    auto& lm = audioProcessor.getLicenseManager();
    licenseBadgeButton.setButtonText(lm.getHeaderStatusBadgeText());

    if (lm.isLicensed())
    {
        licenseBadgeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0f4d34));
        licenseBadgeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff69f0ae));
    }
    else if (lm.isTrialActive())
    {
        licenseBadgeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e4268));
        licenseBadgeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7dd3fc));
    }
    else
    {
        licenseBadgeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff5c1628));
        licenseBadgeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff8a80));
        licenseOverlay.refreshState();
        licenseOverlay.setVisible(true);
        licenseOverlay.toFront(true);
    }
}

void ClipSamplerAudioProcessorEditor::onExportWavClicked()
{
    const int curSlot = audioProcessor.getActiveSlot();
    juce::File currentFile = audioProcessor.getOrCreateSelectedSliceWavFile(curSlot);
    if (!currentFile.existsAsFile())
        currentFile = audioProcessor.getCurrentAudioFile();

    if (!currentFile.existsAsFile())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero extrae una muestra antes de exportar."));
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        juce::String::fromUTF8("Exportar muestra activa en formato WAV (24-bit PCM)"),
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile(currentFile.getFileName()),
        "*.wav");

    auto flags = juce::FileBrowserComponent::saveMode
               | juce::FileBrowserComponent::canSelectFiles
               | juce::FileBrowserComponent::warnAboutOverwriting;

    juce::Component::SafePointer<ClipSamplerAudioProcessorEditor> safeThis(this);
    fileChooser->launchAsync(flags, [safeThis, currentFile](const juce::FileChooser& fc) {
        if (safeThis == nullptr)
            return;

        auto result = fc.getResult();
        if (result != juce::File())
        {
            if (!result.hasFileExtension(".wav"))
                result = result.withFileExtension(".wav");

            if (currentFile.copyFileTo(result))
                safeThis->audioProcessor.setStatusMessage(juce::String::fromUTF8("Muestra exportada a: ") + result.getFileName());
            else
                safeThis->audioProcessor.setStatusMessage(juce::String::fromUTF8("No se pudo guardar el archivo en la ubicación seleccionada."));
        }
    });
}

// ==============================================================================
// Renderizado del Fondo Elegante (Azul Zafiro Real / Titanio Satinado) y Encabezado Responsivo
// ==============================================================================
void ClipSamplerAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float w = (float)getWidth();
    const float h = (float)getHeight();

    // 1. Chasis Nivel 0 (Dark Mode por capas - desktop-ergonomics-hig): Obsidiana Carbono Profundo
    juce::ColourGradient bgGrad(juce::Colour(0xff090e1c), 0.0f, 0.0f,
                                juce::Colour(0xff060a14), w, h, false);
    bgGrad.addColour(0.50, juce::Colour(0xff0f182d));
    g.setGradientFill(bgGrad);
    g.fillAll();

    // 2. Iluminación ambiental radial suave según la plataforma activa
    const SourcePlatform plat = audioProcessor.getSelectedPlatform();
    juce::Colour platAccent = (plat == SourcePlatform::YouTube) ? juce::Colour(0xffff2a55)
                            : (plat == SourcePlatform::TikTok)  ? juce::Colour(0xff00f2fe)
                                                                : juce::Colour(0xffe040fb);

    {
        juce::ColourGradient topGlow(platAccent.withAlpha(0.14f), w * 0.25f, 0.0f,
                                     juce::Colours::transparentBlack, w * 0.25f, h * 0.48f, true);
        g.setGradientFill(topGlow);
        g.fillRect(0.0f, 0.0f, w, h * 0.55f);

        juce::ColourGradient botGlow(juce::Colour(0x1638bdf8), w * 0.72f, h * 0.75f,
                                     juce::Colours::transparentBlack, w * 0.72f, h * 0.25f, true);
        g.setGradientFill(botGlow);
        g.fillRect(0.0f, h * 0.35f, w, h * 0.65f);
    }

    // 3. Textura arquitectónica sutil de estudio (líneas acústicas finas)
    g.setColour(juce::Colour(0x06ffffff));
    for (float yLine = 60.0f; yLine < h; yLine += 28.0f)
        g.drawHorizontalLine((int)yLine, 0.0f, w);

    // 4. Barra de Encabezado Superior en Titanio Anodizado con bisel especular
    const float wScale = juce::jlimit(0.72f, 1.65f, w / 980.0f);
    const float hScale = juce::jlimit(0.72f, 1.65f, h / 760.0f);
    const int headerH = juce::roundToInt(54.0f * hScale);

    auto headerBounds = getLocalBounds().removeFromTop(headerH);
    juce::ColourGradient hdrGrad(juce::Colour(0xff15223b), 0.0f, 0.0f,
                                 juce::Colour(0xff0b1222), 0.0f, (float)headerH, false);
    g.setGradientFill(hdrGrad);
    g.fillRect(headerBounds);

    // Reflejo especular superior de 1px en la barra de encabezado
    g.setColour(juce::Colour(0x22ffffff));
    g.drawHorizontalLine(0, 0.0f, w);

    // Calculamos el espacio disponible EXACTO a la izquierda del botón de Licencia
    const int rightButtonsStartX = licenseBadgeButton.getX() > 40
        ? licenseBadgeButton.getX()
        : juce::roundToInt(w * 0.50f);
    const int availHeaderTextW = juce::jmax(150, rightButtonsStartX - 36);

    const int titleY = juce::roundToInt(4.0f * hScale);
    const int titleH = juce::roundToInt(25.0f * hScale);
    const int subY   = titleY + titleH;
    const int subH   = juce::jmax(14, headerH - subY - 4);

    // Indicador LED luminoso junto al nombre del plugin
    const float ledRadius = 4.2f * hScale;
    const float ledX = 15.0f;
    const float ledY = (float)titleY + (float)titleH * 0.5f - ledRadius;
    juce::Colour ledCol = audioProcessor.isAudioPlaying() ? juce::Colour(0xff10b981) : juce::Colour(0xff38bdf8);
    g.setColour(ledCol.withAlpha(0.32f));
    g.fillEllipse(ledX - 2.5f, ledY - 2.5f, (ledRadius + 2.5f) * 2.0f, (ledRadius + 2.5f) * 2.0f);
    g.setColour(ledCol);
    g.fillEllipse(ledX, ledY, ledRadius * 2.0f, ledRadius * 2.0f);

    // Título principal adaptable con sombra sutil
    const int titleTextX = juce::roundToInt(ledX + ledRadius * 2.0f + 8.0f);
    g.setFont(juce::Font(juce::jlimit(14.5f, 24.0f, 21.0f * wScale), juce::Font::bold));
    g.setColour(juce::Colour(0x88000000));
    g.drawFittedText(juce::String::fromUTF8("CLIPSAMPLER v2.6.1"),
                     titleTextX, titleY + 1, availHeaderTextW, titleH,
                     juce::Justification::centredLeft, 1, 0.68f);
    g.setColour(juce::Colour(0xfff0f9ff));
    g.drawFittedText(juce::String::fromUTF8("CLIPSAMPLER v2.6.1"),
                     titleTextX, titleY, availHeaderTextW, titleH,
                     juce::Justification::centredLeft, 1, 0.68f);

    // Subtítulo 100% en español con indicador de flujo de trabajo Pro
    const juce::String subText = juce::String::fromUTF8("YOUTUBE  |  TIKTOK  |  INSTAGRAM   —   REBANADOR MPC   |   EFECTOS Y MIDI");
    float subFontSize = juce::jlimit(8.5f, 12.5f, 11.0f * wScale);
    juce::Font subFont(subFontSize, juce::Font::bold);
    float rawSubW = subFont.getStringWidthFloat(subText);
    if (rawSubW > (float)availHeaderTextW && rawSubW > 0.0f)
    {
        subFontSize = juce::jmax(8.0f, subFontSize * ((float)availHeaderTextW / rawSubW) * 0.96f);
        subFont = juce::Font(subFontSize, juce::Font::bold);
    }

    g.setFont(subFont);
    g.setColour(platAccent.brighter(0.32f));
    g.drawFittedText(subText,
                     16, subY, availHeaderTextW + 10, subH,
                     juce::Justification::centredLeft, 1, 0.65f);

    // Línea luminosa divisoria debajo del encabezado con resplandor
    juce::ColourGradient lineGrad(platAccent, 0.0f, (float)headerH,
                                  juce::Colour(0xff38bdf8), w, (float)headerH, false);
    g.setGradientFill(lineGrad);
    g.fillRect(0.0f, (float)headerH - 2.0f, w, 2.0f);
}

// ==============================================================================
// Distribución 100% Adaptable (Responsive) por Porcentajes de Ancho y Alto
// ==============================================================================
void ClipSamplerAudioProcessorEditor::resized()
{
    licenseOverlay.setBounds(getLocalBounds());
    freesoundOverlay.setBounds(getLocalBounds());

    const int totalW = getWidth();
    const int totalH = getHeight();

    const float wScale = juce::jlimit(0.70f, 1.75f, (float)totalW / 980.0f);
    const float hScale = juce::jlimit(0.70f, 1.75f, (float)totalH / 760.0f);
    const float uiScale = (wScale + hScale) * 0.5f;
    scalableLnf.setUiScale(uiScale);

    // Escalamos todas las fuentes de etiquetas de acuerdo con la proporción actual de la ventana
    platformLabel.setFont(juce::Font(juce::jlimit(10.0f, 15.0f, 12.5f * uiScale), juce::Font::bold));
    platformHintLabel.setFont(juce::Font(juce::jlimit(9.5f, 14.0f, 11.5f * uiScale), juce::Font::bold));
    urlLabel.setFont(juce::Font(juce::jlimit(10.0f, 15.0f, 12.5f * uiScale), juce::Font::bold));
    startTimeLabel.setFont(juce::Font(juce::jlimit(10.0f, 15.0f, 12.5f * uiScale), juce::Font::bold));
    urlEditor.setFont(juce::Font(juce::jlimit(11.0f, 16.0f, 13.5f * uiScale)));
    startTimeEditor.setFont(juce::Font(juce::jlimit(11.0f, 16.0f, 13.5f * uiScale), juce::Font::bold));
    videoRangeBadgeLabel.setFont(juce::Font(juce::jlimit(9.5f, 14.5f, 11.8f * uiScale), juce::Font::bold));
    statusLabel.setFont(juce::Font(juce::jlimit(9.5f, 14.5f, 11.8f * uiScale)));
    copyrightBadgeLabel.setFont(juce::Font(juce::jlimit(9.5f, 14.5f, 11.8f * uiScale), juce::Font::bold));
    fxMacroTitleLabel.setFont(juce::Font(juce::jlimit(10.0f, 15.0f, 12.2f * uiScale), juce::Font::bold));
    lofiLabel.setFont(juce::Font(juce::jlimit(9.5f, 14.0f, 11.2f * uiScale), juce::Font::bold));
    pitchLabel.setFont(juce::Font(juce::jlimit(9.5f, 14.0f, 11.2f * uiScale), juce::Font::bold));
    toneLabel.setFont(juce::Font(juce::jlimit(9.5f, 14.0f, 11.2f * uiScale), juce::Font::bold));
    attackLabel.setFont(juce::Font(juce::jlimit(9.0f, 13.5f, 10.8f * uiScale), juce::Font::bold));
    releaseLabel.setFont(juce::Font(juce::jlimit(9.0f, 13.5f, 10.8f * uiScale), juce::Font::bold));
    bpmKeyBadgeLabel.setFont(juce::Font(juce::jlimit(9.5f, 14.0f, 11.2f * uiScale), juce::Font::bold));

    // -------------------------------------------------------------------------
    // 0. Encabezado Responsivo (los 4 botones derechos ocupan máximo el 48% del ancho)
    // -------------------------------------------------------------------------
    const int headerH = juce::roundToInt(54.0f * hScale);
    auto headerArea = getLocalBounds().removeFromTop(headerH).reduced(12, juce::roundToInt(9.0f * hScale));

    const int cleanBtnW     = juce::jlimit(68, 112, juce::roundToInt(totalW * 0.090f));
    const int updateBtnW    = juce::jlimit(80, 128, juce::roundToInt(totalW * 0.105f));
    const int freesoundBtnW = juce::jlimit(88, 138, juce::roundToInt(totalW * 0.115f));
    const int licenseBtnW   = juce::jlimit(145, 235, juce::roundToInt(totalW * 0.190f));

    cleanCacheButton.setBounds(headerArea.removeFromRight(cleanBtnW));
    headerArea.removeFromRight(5);
    updateExtractorButton.setBounds(headerArea.removeFromRight(updateBtnW));
    headerArea.removeFromRight(5);
    freesoundButton.setBounds(headerArea.removeFromRight(freesoundBtnW));
    headerArea.removeFromRight(5);
    licenseBadgeButton.setBounds(headerArea.removeFromRight(licenseBtnW));

    auto totalArea = getLocalBounds();
    totalArea.removeFromTop(headerH + 5);
    totalArea.reduce(12, 5);

    // -------------------------------------------------------------------------
    // 1. PANEL SUPERIOR (38% de la altura útil)
    // -------------------------------------------------------------------------
    const int topPanelHeight = juce::jlimit(210, 360, juce::roundToInt(totalArea.getHeight() * 0.39f));
    auto topArea = totalArea.removeFromTop(topPanelHeight);
    topSectionGroup.setBounds(topArea);

    auto topInner = topArea.reduced(12, juce::roundToInt(20.0f * hScale));
    const int innerW = topInner.getWidth();
    const int gapY = juce::jmax(3, juce::roundToInt(topInner.getHeight() * 0.025f));
    const int rowH = juce::jmax(22, (topInner.getHeight() - (gapY * 5) - juce::roundToInt(32.0f * hScale)) / 5);
    const int labelColW = juce::jlimit(78, 120, juce::roundToInt(innerW * 0.115f));

    // Fila 0: Cargar Audio Local + Activar Uso Libre (Web) + Plataformas Web
    auto platRow = topInner.removeFromTop(rowH);
    const int totalPlatRowW = platRow.getWidth() - (4 * 6); // 4 separadores de 6px
    const int localBtnW = juce::jlimit(145, 250, juce::roundToInt(totalPlatRowW * 0.26f));
    const int usoLibreW = juce::jlimit(140, 240, juce::roundToInt(totalPlatRowW * 0.24f));
    const int remainW   = totalPlatRowW - localBtnW - usoLibreW;
    const int eachPlatW = remainW / 3;

    loadLocalFileButton.setBounds(platRow.removeFromLeft(localBtnW));
    platRow.removeFromLeft(6);
    usoLibreButton.setBounds(platRow.removeFromLeft(usoLibreW));
    platRow.removeFromLeft(6);
    platformYoutubeBtn.setBounds(platRow.removeFromLeft(eachPlatW));
    platRow.removeFromLeft(6);
    platformTiktokBtn.setBounds(platRow.removeFromLeft(eachPlatW));
    platRow.removeFromLeft(6);
    platformInstagramBtn.setBounds(platRow);

    topInner.removeFromTop(gapY);

    // Fila 1: Enlace URL + Pegar + Detectar Duración
    auto urlRow = topInner.removeFromTop(rowH);
    urlLabel.setBounds(urlRow.removeFromLeft(labelColW));
    const int detectBtnW = juce::jlimit(105, 168, juce::roundToInt(urlRow.getWidth() * 0.21f));
    const int pasteBtnW  = juce::jlimit(58, 92, juce::roundToInt(urlRow.getWidth() * 0.11f));
    detectDurationButton.setBounds(urlRow.removeFromRight(detectBtnW));
    urlRow.removeFromRight(6);
    pasteButton.setBounds(urlRow.removeFromRight(pasteBtnW));
    urlRow.removeFromRight(6);
    urlEditor.setBounds(urlRow);

    topInner.removeFromTop(gapY);

    // Fila 2: Inicio (MM:SS) + Barra deslizadora + Rango Inicio/Fin del Video
    auto timeRow = topInner.removeFromTop(rowH);
    startTimeLabel.setBounds(timeRow.removeFromLeft(labelColW));
    startTimeEditor.setBounds(timeRow.removeFromLeft(juce::jlimit(56, 86, juce::roundToInt(timeRow.getWidth() * 0.10f))));
    timeRow.removeFromLeft(6);
    videoRangeBadgeLabel.setBounds(timeRow.removeFromRight(juce::jlimit(155, 265, juce::roundToInt(timeRow.getWidth() * 0.34f))));
    timeRow.removeFromRight(6);
    startTimeSlider.setBounds(timeRow);

    topInner.removeFromTop(gapY);

    // Fila 3: Conmutador de Modo (Segundos vs Compases) + 3 Botones de extracción + "Nueva Extracción"
    auto extractRow = topInner.removeFromTop(rowH);
    const int modeBtnW = juce::jlimit(85, 125, juce::roundToInt(extractRow.getWidth() * 0.14f));
    extractModeButton.setBounds(extractRow.removeFromLeft(modeBtnW));
    extractRow.removeFromLeft(6);

    const int newExtractW = juce::jlimit(135, 210, juce::roundToInt(extractRow.getWidth() * 0.27f));
    newExtractionButton.setBounds(extractRow.removeFromRight(newExtractW));
    extractRow.removeFromRight(6);

    const int extractBtnW = (extractRow.getWidth() - 12) / 3;
    extract3sButton.setBounds(extractRow.removeFromLeft(extractBtnW));
    extractRow.removeFromLeft(6);
    extract10sButton.setBounds(extractRow.removeFromLeft(extractBtnW));
    extractRow.removeFromLeft(6);
    extract30sButton.setBounds(extractRow);

    topInner.removeFromTop(gapY);

    // Fila 4: Suite de Coloración y Derechos IA (Calor Analógico, Alterar +3 Tonos Más, Alterar -3 Tonos Menos, Verificar)
    auto rightsRow = topInner.removeFromTop(rowH);
    const int totalRightsW = rightsRow.getWidth() - 18;
    const int smallBtnW = juce::roundToInt(totalRightsW * 0.22f);
    const int largeBtnW = (totalRightsW - (smallBtnW * 2)) / 2;

    tapeWarmthBtn.setBounds(rightsRow.removeFromLeft(largeBtnW));
    rightsRow.removeFromLeft(6);
    derechosPlusBtn.setBounds(rightsRow.removeFromLeft(smallBtnW));
    rightsRow.removeFromLeft(6);
    derechosMinusBtn.setBounds(rightsRow.removeFromLeft(smallBtnW));
    rightsRow.removeFromLeft(6);
    detectCopyrightBtn.setBounds(rightsRow);

    topInner.removeFromTop(gapY);

    // Fila 5: Barra de Progreso + Mensaje de Estado y Veredicto de Derechos
    const int progH = juce::jlimit(9, 14, juce::roundToInt(topInner.getHeight() * 0.36f));
    progressBar.setBounds(topInner.removeFromTop(progH));
    topInner.removeFromTop(2);
    auto statusRow = topInner;
    copyrightBadgeLabel.setBounds(statusRow.removeFromRight(juce::roundToInt(statusRow.getWidth() * 0.38f)));
    statusRow.removeFromRight(6);
    statusLabel.setBounds(statusRow);

    totalArea.removeFromTop(5);

    // -------------------------------------------------------------------------
    // 2. PANEL INFERIOR (61% de la altura útil: Efectos, Doble Visor y Exportación)
    // -------------------------------------------------------------------------
    auto bottomArea = totalArea;
    bottomSectionGroup.setBounds(bottomArea);

    auto bottomInner = bottomArea.reduced(12, juce::roundToInt(20.0f * hScale));
    const int botW = bottomInner.getWidth();

    // Barra superior de la Sección 2: Efectos + Perillas + Sinc. BPM + Aplicar FX + Rebanador MPC
    const int fxRowH = juce::jlimit(30, 44, juce::roundToInt(38.0f * hScale));
    auto fxRow = bottomInner.removeFromTop(fxRowH);

    fxMacroTitleLabel.setBounds(fxRow.removeFromLeft(juce::jlimit(48, 72, juce::roundToInt(botW * 0.068f))));
    const int targetBtnW = juce::jlimit(52, 78, juce::roundToInt(botW * 0.075f));
    fxTargetAudio1Btn.setBounds(fxRow.removeFromLeft(targetBtnW).reduced(0, 3));
    fxRow.removeFromLeft(4);
    fxTargetAudio2Btn.setBounds(fxRow.removeFromLeft(targetBtnW).reduced(0, 3));
    fxRow.removeFromLeft(6);

    // Botones del lado derecho de la fila de efectos
    const int chopBtnW = juce::jlimit(95, 140, juce::roundToInt(botW * 0.140f));
    const int bakeBtnW = juce::jlimit(118, 168, juce::roundToInt(botW * 0.165f));
    const int syncBtnW = juce::jlimit(74, 108, juce::roundToInt(botW * 0.102f));

    autoChopButton.setBounds(fxRow.removeFromRight(chopBtnW).reduced(0, 3));
    fxRow.removeFromRight(4);
    bakeFxButton.setBounds(fxRow.removeFromRight(bakeBtnW).reduced(0, 3));
    fxRow.removeFromRight(4);
    autoSyncButton.setBounds(fxRow.removeFromRight(syncBtnW).reduced(0, 3));
    fxRow.removeFromRight(6);

    // Espacio central dividido en 5 bloques iguales para las 5 perillas (Lo-Fi, Tono, Filtro, Ataque, Caída)
    const int singleKnobBlockW = fxRow.getWidth() / 5;
    const int knobDia = juce::jmin(fxRowH - 2, juce::jmax(22, singleKnobBlockW / 3));

    auto k1Area = fxRow.removeFromLeft(singleKnobBlockW);
    lofiKnob.setBounds(k1Area.removeFromLeft(knobDia));
    lofiLabel.setBounds(k1Area);

    auto k2Area = fxRow.removeFromLeft(singleKnobBlockW);
    pitchKnob.setBounds(k2Area.removeFromLeft(knobDia));
    pitchLabel.setBounds(k2Area);

    auto k3Area = fxRow.removeFromLeft(singleKnobBlockW);
    toneKnob.setBounds(k3Area.removeFromLeft(knobDia));
    toneLabel.setBounds(k3Area);

    auto k4Area = fxRow.removeFromLeft(singleKnobBlockW);
    attackKnob.setBounds(k4Area.removeFromLeft(knobDia));
    attackLabel.setBounds(k4Area);

    auto k5Area = fxRow;
    releaseKnob.setBounds(k5Area.removeFromLeft(knobDia));
    releaseLabel.setBounds(k5Area);

    bottomInner.removeFromTop(5);

    // Barra inferior de transporte, Corte Exacto, Reverse, Half-Time y exportación
    const int actionRowH = juce::jlimit(26, 36, juce::roundToInt(32.0f * hScale));
    auto actionRow = bottomInner.removeFromBottom(actionRowH);

    playButton.setBounds(actionRow.removeFromLeft(juce::jlimit(68, 98, juce::roundToInt(botW * 0.095f))));
    actionRow.removeFromLeft(4);
    stopButton.setBounds(actionRow.removeFromLeft(juce::jlimit(52, 76, juce::roundToInt(botW * 0.072f))));
    actionRow.removeFromLeft(4);
    bpmKeyBadgeLabel.setBounds(actionRow.removeFromLeft(juce::jlimit(110, 180, juce::roundToInt(botW * 0.175f))));
    actionRow.removeFromLeft(4);

    exportWavButton.setBounds(actionRow.removeFromRight(juce::jlimit(78, 110, juce::roundToInt(botW * 0.108f))));
    actionRow.removeFromRight(4);

    const int padFxW = juce::jlimit(46, 68, juce::roundToInt(botW * 0.065f));
    padReverseButton.setBounds(actionRow.removeFromLeft(padFxW));
    actionRow.removeFromLeft(3);
    padHalfTimeButton.setBounds(actionRow.removeFromLeft(padFxW));
    actionRow.removeFromLeft(4);

    const int middleBtnW = (actionRow.getWidth() - 8) / 3;
    exactCutButton.setBounds(actionRow.removeFromLeft(middleBtnW));
    actionRow.removeFromLeft(4);
    dragDawButton.setBounds(actionRow.removeFromLeft(middleBtnW));
    actionRow.removeFromLeft(4);
    dragMidiButton.setBounds(actionRow);

    bottomInner.removeFromBottom(5);

    // Los 2 Visores de Onda se reparten en partes iguales todo el espacio central restante
    const int waveGap = 6;
    const int singleWaveHeight = juce::jmax(45, (bottomInner.getHeight() - waveGap) / 2);
    originalWaveformComponent.setBounds(bottomInner.removeFromTop(singleWaveHeight));
    bottomInner.removeFromTop(waveGap);
    transformedWaveformComponent.setBounds(bottomInner);
}

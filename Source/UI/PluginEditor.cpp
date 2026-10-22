#include "PluginEditor.h"

//==============================================================================
ClipSampler10AudioProcessorEditor::ClipSampler10AudioProcessorEditor(ClipSampler10AudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      originalWaveformComponent(p, 1, juce::String::fromUTF8("VISOR 1: AUDIO ORIGINAL EXTRAÍDO")),
      transformedWaveformComponent(p, 2, juce::String::fromUTF8("VISOR 2: AUDIO MODIFICADO (BOTÓN DERECHOS / ±3 SEMITONOS IA)")),
      licenseOverlay(p.getLicenseManager(), [this] { updateLicenseUiState(); })
{
    setLookAndFeel(&scalableLnf);

    // Enable free user window resizing from the bottom-right corner
    setResizable(true, true);
    setResizeLimits(820, 650, 1650, 1200);
    setSize(940, 720);

    // =========================================================================
    // Header Maintenance & License Controls
    // =========================================================================
    licenseBadgeButton.onClick = [this] { onLicenseBadgeClicked(); };
    addAndMakeVisible(licenseBadgeButton);

    updateExtractorButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222733));
    updateExtractorButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb0bac9));
    updateExtractorButton.onClick = [this] { onUpdateExtractorClicked(); };
    addAndMakeVisible(updateExtractorButton);

    cleanCacheButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222733));
    cleanCacheButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb0bac9));
    cleanCacheButton.onClick = [this] { onCleanCacheClicked(); };
    addAndMakeVisible(cleanCacheButton);

    // =========================================================================
    // SECTION 1: TOP PANEL (LINK, VIDEO DURATION & MULTI-DURATION CAPTURE)
    // =========================================================================
    topSectionGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(topSectionGroup);

    urlLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0bac9));
    urlLabel.setFont(juce::Font(13.5f, juce::Font::bold));
    addAndMakeVisible(urlLabel);

    urlEditor.setTextToShowWhenEmpty("https://www.youtube.com/watch?v=...", juce::Colour(0xff606875));
    urlEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1e222d));
    urlEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    urlEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2d3444));
    if (audioProcessor.getLastUrl().isNotEmpty())
        urlEditor.setText(audioProcessor.getLastUrl(), false);
    urlEditor.onReturnKey = [this] { onDetectDurationClicked(); };
    addAndMakeVisible(urlEditor);

    pasteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3444));
    pasteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe0e6ed));
    pasteButton.onClick = [this] { onPasteClicked(); };
    addAndMakeVisible(pasteButton);

    detectDurationButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1b3a4b));
    detectDurationButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e5ff));
    detectDurationButton.onClick = [this] { onDetectDurationClicked(); };
    addAndMakeVisible(detectDurationButton);

    // Start Time input + interactive time bar slider + video start/end badge
    startTimeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0bac9));
    startTimeLabel.setFont(juce::Font(13.5f, juce::Font::bold));
    addAndMakeVisible(startTimeLabel);

    juce::String savedTime = audioProcessor.getLastStartTime();
    if (savedTime.isEmpty())
        savedTime = "00:00";

    const int initialSeconds = AudioExtractorWorker::parseTimeToSeconds(savedTime);

    startTimeEditor.setText(savedTime, false);
    startTimeEditor.setJustification(juce::Justification::centred);
    startTimeEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1e222d));
    startTimeEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xff00e5ff));
    startTimeEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2d3444));
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
    startTimeSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff161922));
    startTimeSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff00e5ff));
    startTimeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffff0055));
    startTimeSlider.onValueChange = [this] {
        int totalSecs = static_cast<int>(std::round(startTimeSlider.getValue()));
        juce::String formatted = AudioExtractorWorker::formatSecondsToMMSS(totalSecs);
        startTimeEditor.setText(formatted, false);
        audioProcessor.setLastStartTime(formatted);
        refreshVideoTimeRangeLabel();
    };
    addAndMakeVisible(startTimeSlider);

    videoRangeBadgeLabel.setJustificationType(juce::Justification::centred);
    videoRangeBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff182232));
    videoRangeBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    videoRangeBadgeLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff2d4464));
    addAndMakeVisible(videoRangeBadgeLabel);
    refreshVideoTimeRangeLabel();

    // 3 Multi-Duration Extraction Buttons (3s, 10s, 30s)
    extract3sButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffd81b60));
    extract3sButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    extract3sButton.onClick = [this] { onExtractDurationClicked(3); };
    addAndMakeVisible(extract3sButton);

    extract10sButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff0055));
    extract10sButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    extract10sButton.onClick = [this] { onExtractDurationClicked(10); };
    addAndMakeVisible(extract10sButton);

    extract30sButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffad1457));
    extract30sButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    extract30sButton.onClick = [this] { onExtractDurationClicked(30); };
    addAndMakeVisible(extract30sButton);

    // 3 Copyright Detection & Derechos AI Buttons
    detectCopyrightBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b3040));
    detectCopyrightBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffb74d));
    detectCopyrightBtn.onClick = [this] { onCheckCopyrightOriginalClicked(); };
    addAndMakeVisible(detectCopyrightBtn);

    derechosAiBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6200ea));
    derechosAiBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    derechosAiBtn.onClick = [this] { onApplyDerechosClicked(); };
    addAndMakeVisible(derechosAiBtn);

    verifyFreeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0f3d29));
    verifyFreeBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e676));
    verifyFreeBtn.onClick = [this] { onVerifyDerechosFreeClicked(); };
    addAndMakeVisible(verifyFreeBtn);

    // Progress bar, status label & copyright status pill
    currentProgressValue = static_cast<double>(audioProcessor.getExtractionProgress());
    progressBar.setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xff161922));
    progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(progressBar);

    lastDisplayedStatus = audioProcessor.getStatusMessage();
    statusLabel.setText(lastDisplayedStatus, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0bac9));
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    copyrightBadgeLabel.setText(audioProcessor.getCopyrightBadgeText(), juce::dontSendNotification);
    copyrightBadgeLabel.setJustificationType(juce::Justification::centredRight);
    copyrightBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffb74d));
    addAndMakeVisible(copyrightBadgeLabel);

    // =========================================================================
    // SECTION 2: BOTTOM PANEL (DUAL WAVEFORM VIEWERS & EXPORT)
    // =========================================================================
    bottomSectionGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(bottomSectionGroup);

    addAndMakeVisible(originalWaveformComponent);
    addAndMakeVisible(transformedWaveformComponent);

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00c853));
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    playButton.onClick = [this] { onPlayClicked(); };
    addAndMakeVisible(playButton);

    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3444));
    stopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    stopButton.onClick = [this] { onStopClicked(); };
    addAndMakeVisible(stopButton);

    dragDawButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6200ea));
    dragDawButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    dragDawButton.onClick = [this] { onDragDawClicked(); };
    dragDawButton.addMouseListener(this, false);
    addAndMakeVisible(dragDawButton);

    exportWavButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a73e8));
    exportWavButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    exportWavButton.onClick = [this] { onExportWavClicked(); };
    addAndMakeVisible(exportWavButton);

    addChildComponent(licenseOverlay);
    updateLicenseUiState();

    startTimerHz(30);
}

ClipSampler10AudioProcessorEditor::~ClipSampler10AudioProcessorEditor()
{
    stopTimer();
    dragDawButton.removeMouseListener(this);
    setLookAndFeel(nullptr);
}

void ClipSampler10AudioProcessorEditor::refreshVideoTimeRangeLabel()
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
                                     + juce::String::fromUTF8("  |  Fin Video: [Pulsa Detectar]"),
                                     juce::dontSendNotification);
    }
}

void ClipSampler10AudioProcessorEditor::timerCallback()
{
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

    extract3sButton.setEnabled(unlocked && !extracting);
    extract10sButton.setEnabled(unlocked && !extracting);
    extract30sButton.setEnabled(unlocked && !extracting);
    detectDurationButton.setEnabled(unlocked && !fetchingMeta);
    updateExtractorButton.setEnabled(unlocked && !updating);

    // Sync detected video duration with slider range if newly detected
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
            copyrightBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e676));
        else if (verifyState == 1)
            copyrightBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff5252));
        else
            copyrightBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffb74d));

        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }

    if (audioProcessor.isAudioPlaying())
    {
        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }
}

void ClipSampler10AudioProcessorEditor::onPasteClicked()
{
    juce::String clip = juce::SystemClipboard::getTextFromClipboard().trim();
    if (clip.isNotEmpty())
    {
        urlEditor.setText(clip, false);
        audioProcessor.detectVideoDuration(clip);
    }
}

void ClipSampler10AudioProcessorEditor::onDetectDurationClicked()
{
    juce::String url = urlEditor.getText().trim();
    if (url.isEmpty())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero pega una URL de YouTube para detectar su duración."));
        return;
    }
    audioProcessor.detectVideoDuration(url);
}

void ClipSampler10AudioProcessorEditor::onExtractDurationClicked(int durationSec)
{
    juce::String url = urlEditor.getText().trim();
    if (url.isEmpty())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Ingresa una URL válida de YouTube."));
        statusLabel.setText(audioProcessor.getStatusMessage(), juce::dontSendNotification);
        return;
    }

    audioProcessor.startSampleExtraction(url, startTimeEditor.getText(), durationSec);
    timerCallback();
}

void ClipSampler10AudioProcessorEditor::onCheckCopyrightOriginalClicked()
{
    juce::String report;
    audioProcessor.analyzeOriginalCopyright(report);
    timerCallback();
}

void ClipSampler10AudioProcessorEditor::onApplyDerechosClicked()
{
    // Alternates between +3 semitones and -3 semitones (+ AI phase/stereo decorrelation)
    const int semitonesToApply = nextShiftPositive ? 3 : -3;
    nextShiftPositive = !nextShiftPositive;

    juce::String nextBtnLabel = nextShiftPositive
        ? juce::String::fromUTF8("🛡️ 2. Derechos IA (Alterar +3 Semitonos)")
        : juce::String::fromUTF8("🛡️ 2. Derechos IA (Alterar -3 Semitonos)");
    derechosAiBtn.setButtonText(nextBtnLabel);

    juce::String report;
    audioProcessor.applyDerechosTransformation(semitonesToApply, report);
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
    timerCallback();
}

void ClipSampler10AudioProcessorEditor::onVerifyDerechosFreeClicked()
{
    juce::String report;
    audioProcessor.verifyTransformedAudioCopyrightFree(report);
    timerCallback();
}

void ClipSampler10AudioProcessorEditor::onPlayClicked()
{
    audioProcessor.startPlayback();
}

void ClipSampler10AudioProcessorEditor::onStopClicked()
{
    audioProcessor.stopPlayback();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
}

void ClipSampler10AudioProcessorEditor::onDragDawClicked()
{
    auto loadedFile = audioProcessor.getCurrentAudioFile();
    if (!loadedFile.existsAsFile())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero debes extraer una muestra para poder arrastrarla al DAW."));
        return;
    }

    int slot = audioProcessor.getActiveSlot();
    audioProcessor.setStatusMessage(juce::String::fromUTF8("Tip: Mantén presionado y arrastra este botón (o cualquiera de los 2 visores) hacia tu DAW. [Activo: Visor ")
                                    + juce::String(slot) + "]");
}

void ClipSampler10AudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (event.eventComponent == &dragDawButton && event.getDistanceFromDragStart() > 4)
    {
        auto loadedFile = audioProcessor.getCurrentAudioFile();
        if (loadedFile.existsAsFile())
        {
            juce::StringArray files;
            files.add(loadedFile.getFullPathName());
            performExternalDragDropOfFiles(files, false, &dragDawButton);
        }
    }
}

void ClipSampler10AudioProcessorEditor::onUpdateExtractorClicked()
{
    audioProcessor.startExtractorUpdate();
    timerCallback();
}

void ClipSampler10AudioProcessorEditor::onCleanCacheClicked()
{
    audioProcessor.cleanAudioCache();
    timerCallback();
}

void ClipSampler10AudioProcessorEditor::onLicenseBadgeClicked()
{
    licenseOverlay.refreshState();
    licenseOverlay.setVisible(true);
    licenseOverlay.toFront(true);
}

void ClipSampler10AudioProcessorEditor::updateLicenseUiState()
{
    auto& lm = audioProcessor.getLicenseManager();
    licenseBadgeButton.setButtonText(lm.getHeaderStatusBadgeText());

    if (lm.isLicensed())
    {
        licenseBadgeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0f3d29));
        licenseBadgeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e676));
    }
    else if (lm.isTrialActive())
    {
        licenseBadgeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1b3648));
        licenseBadgeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e5ff));
    }
    else
    {
        licenseBadgeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a121f));
        licenseBadgeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff5252));
        licenseOverlay.refreshState();
        licenseOverlay.setVisible(true);
        licenseOverlay.toFront(true);
    }
}

void ClipSampler10AudioProcessorEditor::onExportWavClicked()
{
    auto currentFile = audioProcessor.getCurrentAudioFile();
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

    juce::Component::SafePointer<ClipSampler10AudioProcessorEditor> safeThis(this);
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

void ClipSampler10AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12141a));

    const float scale = juce::jlimit(0.9f, 1.6f, (float)getWidth() / 940.0f);
    const int headerH = juce::roundToInt(56.0f * scale);

    auto headerBounds = getLocalBounds().removeFromTop(headerH);
    g.setColour(juce::Colour(0xff1a1d26));
    g.fillRect(headerBounds);

    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::Font(22.0f * scale, juce::Font::bold));
    g.drawText("CLIPSAMPLER v2.2", 18, juce::roundToInt(6.0f * scale), juce::roundToInt(260.0f * scale), juce::roundToInt(26.0f * scale), juce::Justification::left);

    g.setColour(juce::Colour(0xff8c9ba5));
    g.setFont(juce::Font(11.5f * scale, juce::Font::plain));
    g.drawText("AI SAMPLER // 3s - 10s - 30s // EVASOR DE DERECHOS (±3 ST)", 18, juce::roundToInt(32.0f * scale), juce::roundToInt(350.0f * scale), juce::roundToInt(18.0f * scale), juce::Justification::left);

    g.setColour(juce::Colour(0xff2d3444));
    g.drawHorizontalLine(headerH, 0.0f, (float)getWidth());
}

void ClipSampler10AudioProcessorEditor::resized()
{
    licenseOverlay.setBounds(getLocalBounds());

    const float wScale = juce::jlimit(0.88f, 1.75f, (float)getWidth() / 940.0f);
    const float hScale = juce::jlimit(0.88f, 1.75f, (float)getHeight() / 720.0f);
    const float uiScale = (wScale + hScale) * 0.5f;
    scalableLnf.setUiScale(uiScale);

    // Update label fonts proportionally when window stretches
    urlLabel.setFont(juce::Font(13.5f * uiScale, juce::Font::bold));
    startTimeLabel.setFont(juce::Font(13.5f * uiScale, juce::Font::bold));
    urlEditor.setFont(juce::Font(14.0f * uiScale));
    startTimeEditor.setFont(juce::Font(14.5f * uiScale, juce::Font::bold));
    videoRangeBadgeLabel.setFont(juce::Font(12.5f * uiScale, juce::Font::bold));
    statusLabel.setFont(juce::Font(12.5f * uiScale));
    copyrightBadgeLabel.setFont(juce::Font(12.5f * uiScale, juce::Font::bold));

    // 0. Header Layout (large, clear Trial 3-Days button that scales with window width)
    const int headerH = juce::roundToInt(56.0f * wScale);
    auto headerArea = getLocalBounds().removeFromTop(headerH).reduced(14, juce::roundToInt(9.0f * hScale));

    cleanCacheButton.setBounds(headerArea.removeFromRight(juce::roundToInt(120.0f * wScale)));
    headerArea.removeFromRight(8);
    updateExtractorButton.setBounds(headerArea.removeFromRight(juce::roundToInt(145.0f * wScale)));
    headerArea.removeFromRight(8);
    licenseBadgeButton.setBounds(headerArea.removeFromRight(juce::roundToInt(275.0f * wScale)));

    auto totalArea = getLocalBounds();
    totalArea.removeFromTop(headerH + 6);
    totalArea.reduce(14, 8);

    const int topPanelHeight = juce::roundToInt(262.0f * hScale);
    const int rowH = juce::roundToInt(34.0f * hScale);
    const int gapY = juce::roundToInt(9.0f * hScale);

    // =========================================================================
    // 1. TOP PANEL LAYOUT
    // =========================================================================
    auto topArea = totalArea.removeFromTop(topPanelHeight);
    topSectionGroup.setBounds(topArea);

    auto topInner = topArea.reduced(16, juce::roundToInt(22.0f * hScale));

    // Row 1: URL input + Paste + Detect Video Duration
    auto urlRow = topInner.removeFromTop(rowH);
    urlLabel.setBounds(urlRow.removeFromLeft(juce::roundToInt(125.0f * wScale)));
    detectDurationButton.setBounds(urlRow.removeFromRight(juce::roundToInt(165.0f * wScale)));
    urlRow.removeFromRight(6);
    pasteButton.setBounds(urlRow.removeFromRight(juce::roundToInt(88.0f * wScale)));
    urlRow.removeFromRight(8);
    urlEditor.setBounds(urlRow);

    topInner.removeFromTop(gapY);

    // Row 2: Start time + Interactive slider + Video Start/End Duration Badge
    auto timeRow = topInner.removeFromTop(rowH);
    startTimeLabel.setBounds(timeRow.removeFromLeft(juce::roundToInt(125.0f * wScale)));
    startTimeEditor.setBounds(timeRow.removeFromLeft(juce::roundToInt(82.0f * wScale)));
    timeRow.removeFromLeft(10);
    videoRangeBadgeLabel.setBounds(timeRow.removeFromRight(juce::roundToInt(258.0f * wScale)));
    timeRow.removeFromRight(10);
    startTimeSlider.setBounds(timeRow);

    topInner.removeFromTop(gapY);

    // Row 3: 3 Multi-Duration Extract Buttons (3s, 10s, 30s)
    auto extractRow = topInner.removeFromTop(juce::roundToInt(36.0f * hScale));
    const int thirdW = (extractRow.getWidth() - 16) / 3;
    extract3sButton.setBounds(extractRow.removeFromLeft(thirdW));
    extractRow.removeFromLeft(8);
    extract10sButton.setBounds(extractRow.removeFromLeft(thirdW));
    extractRow.removeFromLeft(8);
    extract30sButton.setBounds(extractRow);

    topInner.removeFromTop(gapY);

    // Row 4: 3 Copyright & Derechos AI Buttons
    auto rightsRow = topInner.removeFromTop(juce::roundToInt(36.0f * hScale));
    const int rightsW = (rightsRow.getWidth() - 16) / 3;
    detectCopyrightBtn.setBounds(rightsRow.removeFromLeft(rightsW));
    rightsRow.removeFromLeft(8);
    derechosAiBtn.setBounds(rightsRow.removeFromLeft(rightsW));
    rightsRow.removeFromLeft(8);
    verifyFreeBtn.setBounds(rightsRow);

    topInner.removeFromTop(gapY);

    // Row 5: Progress Bar & Status + Copyright Verdict Badge
    auto progressRow = topInner.removeFromTop(juce::roundToInt(16.0f * hScale));
    progressBar.setBounds(progressRow);

    topInner.removeFromTop(4);
    auto statusRow = topInner.removeFromTop(juce::roundToInt(22.0f * hScale));
    copyrightBadgeLabel.setBounds(statusRow.removeFromRight(juce::roundToInt(320.0f * wScale)));
    statusLabel.setBounds(statusRow);

    totalArea.removeFromTop(8);

    // =========================================================================
    // 2. BOTTOM PANEL LAYOUT (SPLIT DUAL WAVEFORM VIEWERS)
    // =========================================================================
    auto bottomArea = totalArea;
    bottomSectionGroup.setBounds(bottomArea);

    auto bottomInner = bottomArea.reduced(16, juce::roundToInt(22.0f * hScale));

    // Bottom action row: Play, Stop, Drag to DAW, Export .WAV
    auto actionRow = bottomInner.removeFromBottom(juce::roundToInt(36.0f * hScale));
    playButton.setBounds(actionRow.removeFromLeft(juce::roundToInt(160.0f * wScale)));
    actionRow.removeFromLeft(8);
    stopButton.setBounds(actionRow.removeFromLeft(juce::roundToInt(95.0f * wScale)));
    actionRow.removeFromLeft(12);

    exportWavButton.setBounds(actionRow.removeFromRight(juce::roundToInt(140.0f * wScale)));
    actionRow.removeFromRight(8);
    dragDawButton.setBounds(actionRow);

    bottomInner.removeFromBottom(8);

    // Split remaining central area into 2 equal Waveform Viewers (Top: Original, Bottom: Modificado Derechos)
    const int waveGap = 8;
    const int singleWaveHeight = (bottomInner.getHeight() - waveGap) / 2;
    originalWaveformComponent.setBounds(bottomInner.removeFromTop(singleWaveHeight));
    bottomInner.removeFromTop(waveGap);
    transformedWaveformComponent.setBounds(bottomInner);
}

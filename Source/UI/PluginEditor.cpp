#include "PluginEditor.h"

//==============================================================================
ClipSampler10AudioProcessorEditor::ClipSampler10AudioProcessorEditor(ClipSampler10AudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      originalWaveformComponent(p, 1, juce::String::fromUTF8("VISOR 1: AUDIO ORIGINAL (AUTO-CHOP C1-G2)")),
      transformedWaveformComponent(p, 2, juce::String::fromUTF8("VISOR 2: AUDIO MODIFICADO (DERECHOS ±3 ST IA)")),
      licenseOverlay(p.getLicenseManager(), [this] { updateLicenseUiState(); })
{
    setLookAndFeel(&scalableLnf);

    // Enable free user window resizing from the bottom-right corner
    setResizable(true, true);
    setResizeLimits(860, 680, 1750, 1250);
    setSize(980, 760);

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
    // SECTION 1: TOP PANEL (LINK, VIDEO DURATION, 3s/10s/30s & NUEVA EXTRACCIÓN)
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

    // 3 Multi-Duration Extraction Buttons (3s, 10s, 30s) + Conditional "Nueva Extracción" Button
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

    newExtractionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252b3b));
    newExtractionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7d8799));
    newExtractionButton.onClick = [this] { onNewExtractionClicked(); };
    addAndMakeVisible(newExtractionButton);

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
    // SECTION 2: VISOR AUTO-CHOP (MODO MPC), FX MACRO KNOBS, BPM/KEY & MIDI
    // =========================================================================
    bottomSectionGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(bottomSectionGroup);

    // FX Target Selection (Audio 1 vs Audio 2)
    fxMacroTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    fxMacroTitleLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    addAndMakeVisible(fxMacroTitleLabel);

    fxTargetAudio1Btn.onClick = [this] { onSelectFxTargetSlot(1); };
    addAndMakeVisible(fxTargetAudio1Btn);

    fxTargetAudio2Btn.onClick = [this] { onSelectFxTargetSlot(2); };
    addAndMakeVisible(fxTargetAudio2Btn);

    // Knob 1: Lo-Fi 12-Bit (0% to 100%)
    lofiKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    lofiKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lofiKnob.setRange(0.0, 100.0, 1.0);
    lofiKnob.setValue(audioProcessor.getLofiAmount() * 100.0f, juce::dontSendNotification);
    lofiKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffffab00));
    lofiKnob.onValueChange = [this] {
        int pct = juce::roundToInt(lofiKnob.getValue());
        audioProcessor.setLofiAmount((float)pct / 100.0f);
        lofiLabel.setText(juce::String::fromUTF8("📻 Lo-Fi 12-Bit: ") + juce::String(pct) + "%", juce::dontSendNotification);
    };
    addAndMakeVisible(lofiKnob);

    lofiLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffd54f));
    lofiLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(lofiLabel);

    // Knob 2: Pitch Semitones (-12 st to +12 st)
    pitchKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pitchKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    pitchKnob.setRange(-12.0, 12.0, 1.0);
    pitchKnob.setValue(audioProcessor.getFxPitchSemitones(), juce::dontSendNotification);
    pitchKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00e5ff));
    pitchKnob.onValueChange = [this] {
        int st = juce::roundToInt(pitchKnob.getValue());
        audioProcessor.setFxPitchSemitones((float)st);
        juce::String sign = (st > 0) ? "+" : "";
        pitchLabel.setText(juce::String::fromUTF8("🌀 Pitch: ") + sign + juce::String(st) + " st", juce::dontSendNotification);
    };
    addAndMakeVisible(pitchKnob);

    pitchLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    pitchLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pitchLabel);

    // Knob 3: Tone / Low-Pass Warmth Filter (15% to 100%)
    toneKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    toneKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    toneKnob.setRange(15.0, 100.0, 1.0);
    toneKnob.setValue(audioProcessor.getFxToneFilter() * 100.0f, juce::dontSendNotification);
    toneKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00e676));
    toneKnob.onValueChange = [this] {
        int pct = juce::roundToInt(toneKnob.getValue());
        audioProcessor.setFxToneFilter((float)pct / 100.0f);
        toneLabel.setText(juce::String::fromUTF8("🎚️ Filtro: ") + juce::String(pct) + "%", juce::dontSendNotification);
    };
    addAndMakeVisible(toneKnob);

    toneLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e676));
    toneLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(toneLabel);

    // Auto-Sync, Auto-Chop MPC Mode, Bake FX & BPM/Key Detector Badge
    autoSyncButton.onClick = [this] { onToggleAutoSyncClicked(); };
    addAndMakeVisible(autoSyncButton);

    autoChopButton.onClick = [this] { onToggleAutoChopClicked(); };
    addAndMakeVisible(autoChopButton);

    bakeFxButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff37474f));
    bakeFxButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffab00));
    bakeFxButton.onClick = [this] { onBakeFxClicked(); };
    addAndMakeVisible(bakeFxButton);

    bpmKeyBadgeLabel.setJustificationType(juce::Justification::centred);
    bpmKeyBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff182232));
    bpmKeyBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    bpmKeyBadgeLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff2d4464));
    addAndMakeVisible(bpmKeyBadgeLabel);

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
    updateLicenseUiState();
    updateFxAndMpcUiState();

    startTimerHz(30);
}

ClipSampler10AudioProcessorEditor::~ClipSampler10AudioProcessorEditor()
{
    stopTimer();
    dragDawButton.removeMouseListener(this);
    dragMidiButton.removeMouseListener(this);
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

void ClipSampler10AudioProcessorEditor::updateFxAndMpcUiState()
{
    const int targetSlot = audioProcessor.getFxTargetSlot();
    fxTargetAudio1Btn.setColour(juce::TextButton::buttonColourId, (targetSlot == 1) ? juce::Colour(0xff00b0ff) : juce::Colour(0xff222733));
    fxTargetAudio1Btn.setColour(juce::TextButton::textColourOffId, (targetSlot == 1) ? juce::Colours::black : juce::Colour(0xffb0bac9));

    fxTargetAudio2Btn.setColour(juce::TextButton::buttonColourId, (targetSlot == 2) ? juce::Colour(0xff00e676) : juce::Colour(0xff222733));
    fxTargetAudio2Btn.setColour(juce::TextButton::textColourOffId, (targetSlot == 2) ? juce::Colours::black : juce::Colour(0xffb0bac9));

    const bool syncOn = audioProcessor.isAutoSyncEnabled();
    autoSyncButton.setButtonText(syncOn ? juce::String::fromUTF8("🔗 Sync: ON") : juce::String::fromUTF8("🔗 Sync: OFF"));
    autoSyncButton.setColour(juce::TextButton::buttonColourId, syncOn ? juce::Colour(0xff00c853) : juce::Colour(0xff252b3b));
    autoSyncButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    const bool chopOn = audioProcessor.isAutoChopEnabled();
    autoChopButton.setButtonText(chopOn ? juce::String::fromUTF8("🥁 Auto-Chop MPC: ON") : juce::String::fromUTF8("🥁 Auto-Chop MPC: OFF"));
    autoChopButton.setColour(juce::TextButton::buttonColourId, chopOn ? juce::Colour(0xffffab00) : juce::Colour(0xff252b3b));
    autoChopButton.setColour(juce::TextButton::textColourOffId, chopOn ? juce::Colours::black : juce::Colours::white);

    const int curSlot = audioProcessor.getActiveSlot();
    const double detBpm = audioProcessor.getDetectedBpm(curSlot);
    const juce::String detKey = audioProcessor.getDetectedKey(curSlot);
    const double hostBpm = audioProcessor.getHostBpm();

    if (detBpm > 10.0)
    {
        bpmKeyBadgeLabel.setText(juce::String::fromUTF8("🥁 ") + juce::String(detBpm, 1)
                                 + " BPM | 🎹 Key: " + detKey
                                 + " | DAW: " + juce::String(hostBpm, 0),
                                 juce::dontSendNotification);
    }
    else
    {
        bpmKeyBadgeLabel.setText(juce::String::fromUTF8("🥁 -- BPM | 🎹 Key: -- | DAW: ") + juce::String(hostBpm, 0),
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
    const bool bothAudiosReady = audioProcessor.hasBothAudiosReady();

    extract3sButton.setEnabled(unlocked && !extracting);
    extract10sButton.setEnabled(unlocked && !extracting);
    extract30sButton.setEnabled(unlocked && !extracting);
    detectDurationButton.setEnabled(unlocked && !fetchingMeta);
    updateExtractorButton.setEnabled(unlocked && !updating);

    // Condition requested by user: Enable "Nueva Extracción" button ONLY when both Audio 1 & Audio 2 are extracted/ready!
    newExtractionButton.setEnabled(unlocked && !extracting && bothAudiosReady);
    if (bothAudiosReady)
    {
        newExtractionButton.setButtonText(juce::String::fromUTF8("🔄 Nueva Extracción (Limpiar Todo)"));
        newExtractionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00b0ff));
        newExtractionButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    }
    else
    {
        newExtractionButton.setButtonText(juce::String::fromUTF8("🔒 Nueva Extracción (2 Audios)"));
        newExtractionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252b3b));
        newExtractionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7d8799));
    }

    updateFxAndMpcUiState();

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

void ClipSampler10AudioProcessorEditor::onNewExtractionClicked()
{
    if (!audioProcessor.hasBothAudiosReady())
        return;

    audioProcessor.resetForNewExtraction();

    urlEditor.clear();
    startTimeEditor.setText("00:00", false);
    startTimeSlider.setRange(0.0, 600.0, 1.0);
    startTimeSlider.setValue(0.0, juce::dontSendNotification);
    lastKnownVideoDuration = -1;
    refreshVideoTimeRangeLabel();

    lofiKnob.setValue(0.0, juce::dontSendNotification);
    lofiLabel.setText(juce::String::fromUTF8("📻 Lo-Fi 12-Bit: 0%"), juce::dontSendNotification);
    pitchKnob.setValue(0.0, juce::dontSendNotification);
    pitchLabel.setText(juce::String::fromUTF8("🌀 Pitch: 0 st"), juce::dontSendNotification);
    toneKnob.setValue(100.0, juce::dontSendNotification);
    toneLabel.setText(juce::String::fromUTF8("🎚️ Filtro: 100%"), juce::dontSendNotification);

    currentProgressValue = 0.0;
    progressBar.repaint();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
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

void ClipSampler10AudioProcessorEditor::onSelectFxTargetSlot(int slotIndex)
{
    audioProcessor.setFxTargetSlot(slotIndex);
    updateFxAndMpcUiState();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
    audioProcessor.setStatusMessage(juce::String::fromUTF8("🎛️ Destino FX Macro y MIDI cambiado a: Audio ")
                                    + juce::String(slotIndex)
                                    + (slotIndex == 2 ? juce::String::fromUTF8(" (Modificado Derechos IA)") : juce::String::fromUTF8(" (Original)")));
}

void ClipSampler10AudioProcessorEditor::onToggleAutoChopClicked()
{
    bool nextState = !audioProcessor.isAutoChopEnabled();
    audioProcessor.setAutoChopEnabled(nextState);
    updateFxAndMpcUiState();
    originalWaveformComponent.repaint();
    transformedWaveformComponent.repaint();
    audioProcessor.setStatusMessage(nextState
        ? juce::String::fromUTF8("🥁 Modo Auto-Chop MPC ACTIVADO: Los transientes se han asignado de C1 a G2 en tu teclado MIDI y en el visor.")
        : juce::String::fromUTF8("🎹 Modo Cromático Estándar ACTIVADO: El sample completo suena cromáticamente en el teclado MIDI."));
}

void ClipSampler10AudioProcessorEditor::onToggleAutoSyncClicked()
{
    bool nextState = !audioProcessor.isAutoSyncEnabled();
    audioProcessor.setAutoSyncEnabled(nextState);
    updateFxAndMpcUiState();
    int curSlot = audioProcessor.getActiveSlot();
    audioProcessor.setStatusMessage(nextState
        ? (juce::String::fromUTF8("🔗 Auto-Sync ACTIVADO: Sincronizando sample (")
           + juce::String(audioProcessor.getDetectedBpm(curSlot), 1) + " BPM) al tempo del DAW ("
           + juce::String(audioProcessor.getHostBpm(), 1) + " BPM).")
        : juce::String::fromUTF8("🔗 Auto-Sync DESACTIVADO: Reproducción al tempo natural del sample."));
}

void ClipSampler10AudioProcessorEditor::onBakeFxClicked()
{
    juce::String report;
    if (audioProcessor.applyFxMacroToTargetSlot(report))
    {
        lofiKnob.setValue(0.0, juce::dontSendNotification);
        lofiLabel.setText(juce::String::fromUTF8("📻 Lo-Fi 12-Bit: 0%"), juce::dontSendNotification);
        pitchKnob.setValue(0.0, juce::dontSendNotification);
        pitchLabel.setText(juce::String::fromUTF8("🌀 Pitch: 0 st"), juce::dontSendNotification);
        toneKnob.setValue(100.0, juce::dontSendNotification);
        toneLabel.setText(juce::String::fromUTF8("🎚️ Filtro: 100%"), juce::dontSendNotification);
        originalWaveformComponent.repaint();
        transformedWaveformComponent.repaint();
    }
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
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero debes extraer una muestra para poder arrastrar el .WAV al DAW."));
        return;
    }

    int slot = audioProcessor.getActiveSlot();
    audioProcessor.setStatusMessage(juce::String::fromUTF8("🎵 Tip: Mantén presionado '🎵 Arrastrar .WAV' y suéltalo en una pista de audio de tu DAW. [Activo: Audio ")
                                    + juce::String(slot) + "]");
}

void ClipSampler10AudioProcessorEditor::onDragMidiClicked()
{
    auto loadedFile = audioProcessor.getCurrentAudioFile();
    if (!loadedFile.existsAsFile())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero debes extraer una muestra para convertir y arrastrar su .MIDI al Piano Roll."));
        return;
    }

    auto midiFile = audioProcessor.generateMidiFileForSlot(audioProcessor.getActiveSlot());
    if (midiFile.existsAsFile())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("🎹 ¡Archivo MIDI generado! Mantén presionado '🎹 Arrastrar .MIDI' y suéltalo en el Piano Roll de cualquier sintetizador."));
    }
}

void ClipSampler10AudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (event.eventComponent == &dragDawButton && event.getDistanceFromDragStart() > 4)
    {
        // If user moved any FX knob, automatically bake FX to the target slot so the dragged .WAV includes their FX!
        if (audioProcessor.getLofiAmount() > 0.01f
            || std::abs(audioProcessor.getFxPitchSemitones()) > 0.1f
            || audioProcessor.getFxToneFilter() < 0.98f)
        {
            juce::String rep;
            audioProcessor.applyFxMacroToTargetSlot(rep);
        }

        auto loadedFile = audioProcessor.getCurrentAudioFile();
        if (loadedFile.existsAsFile())
        {
            juce::StringArray files;
            files.add(loadedFile.getFullPathName());
            performExternalDragDropOfFiles(files, false, &dragDawButton);
        }
    }
    else if (event.eventComponent == &dragMidiButton && event.getDistanceFromDragStart() > 4)
    {
        auto midiFile = audioProcessor.getCurrentMidiFile();
        if (midiFile.existsAsFile())
        {
            juce::StringArray files;
            files.add(midiFile.getFullPathName());
            performExternalDragDropOfFiles(files, false, &dragMidiButton);
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

    const float scale = juce::jlimit(0.9f, 1.6f, (float)getWidth() / 980.0f);
    const int headerH = juce::roundToInt(54.0f * scale);

    auto headerBounds = getLocalBounds().removeFromTop(headerH);
    g.setColour(juce::Colour(0xff1a1d26));
    g.fillRect(headerBounds);

    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::Font(21.5f * scale, juce::Font::bold));
    g.drawText("CLIPSAMPLER v2.3", 18, juce::roundToInt(5.0f * scale), juce::roundToInt(270.0f * scale), juce::roundToInt(25.0f * scale), juce::Justification::left);

    g.setColour(juce::Colour(0xff8c9ba5));
    g.setFont(juce::Font(11.0f * scale, juce::Font::plain));
    g.drawText("MPC AUTO-CHOP (C1-G2) // FX MACRO // BPM & KEY // AUDIO A MIDI // DERECHOS IA", 18, juce::roundToInt(30.0f * scale), juce::roundToInt(420.0f * scale), juce::roundToInt(18.0f * scale), juce::Justification::left);

    g.setColour(juce::Colour(0xff2d3444));
    g.drawHorizontalLine(headerH, 0.0f, (float)getWidth());
}

void ClipSampler10AudioProcessorEditor::resized()
{
    licenseOverlay.setBounds(getLocalBounds());

    const float wScale = juce::jlimit(0.88f, 1.75f, (float)getWidth() / 980.0f);
    const float hScale = juce::jlimit(0.88f, 1.75f, (float)getHeight() / 760.0f);
    const float uiScale = (wScale + hScale) * 0.5f;
    scalableLnf.setUiScale(uiScale);

    // Update label fonts proportionally when window stretches
    urlLabel.setFont(juce::Font(13.0f * uiScale, juce::Font::bold));
    startTimeLabel.setFont(juce::Font(13.0f * uiScale, juce::Font::bold));
    urlEditor.setFont(juce::Font(13.5f * uiScale));
    startTimeEditor.setFont(juce::Font(14.0f * uiScale, juce::Font::bold));
    videoRangeBadgeLabel.setFont(juce::Font(12.0f * uiScale, juce::Font::bold));
    statusLabel.setFont(juce::Font(12.0f * uiScale));
    copyrightBadgeLabel.setFont(juce::Font(12.0f * uiScale, juce::Font::bold));
    fxMacroTitleLabel.setFont(juce::Font(12.5f * uiScale, juce::Font::bold));
    lofiLabel.setFont(juce::Font(11.5f * uiScale, juce::Font::bold));
    pitchLabel.setFont(juce::Font(11.5f * uiScale, juce::Font::bold));
    toneLabel.setFont(juce::Font(11.5f * uiScale, juce::Font::bold));
    bpmKeyBadgeLabel.setFont(juce::Font(11.5f * uiScale, juce::Font::bold));

    // 0. Header Layout
    const int headerH = juce::roundToInt(54.0f * wScale);
    auto headerArea = getLocalBounds().removeFromTop(headerH).reduced(14, juce::roundToInt(8.0f * hScale));

    cleanCacheButton.setBounds(headerArea.removeFromRight(juce::roundToInt(118.0f * wScale)));
    headerArea.removeFromRight(8);
    updateExtractorButton.setBounds(headerArea.removeFromRight(juce::roundToInt(142.0f * wScale)));
    headerArea.removeFromRight(8);
    licenseBadgeButton.setBounds(headerArea.removeFromRight(juce::roundToInt(275.0f * wScale)));

    auto totalArea = getLocalBounds();
    totalArea.removeFromTop(headerH + 5);
    totalArea.reduce(14, 6);

    const int topPanelHeight = juce::roundToInt(248.0f * hScale);
    const int rowH = juce::roundToInt(32.0f * hScale);
    const int gapY = juce::roundToInt(7.0f * hScale);

    // =========================================================================
    // 1. TOP PANEL LAYOUT
    // =========================================================================
    auto topArea = totalArea.removeFromTop(topPanelHeight);
    topSectionGroup.setBounds(topArea);

    auto topInner = topArea.reduced(14, juce::roundToInt(20.0f * hScale));

    // Row 1: URL input + Paste + Detect Video Duration
    auto urlRow = topInner.removeFromTop(rowH);
    urlLabel.setBounds(urlRow.removeFromLeft(juce::roundToInt(120.0f * wScale)));
    detectDurationButton.setBounds(urlRow.removeFromRight(juce::roundToInt(162.0f * wScale)));
    urlRow.removeFromRight(6);
    pasteButton.setBounds(urlRow.removeFromRight(juce::roundToInt(85.0f * wScale)));
    urlRow.removeFromRight(8);
    urlEditor.setBounds(urlRow);

    topInner.removeFromTop(gapY);

    // Row 2: Start time + Interactive slider + Video Start/End Duration Badge
    auto timeRow = topInner.removeFromTop(rowH);
    startTimeLabel.setBounds(timeRow.removeFromLeft(juce::roundToInt(120.0f * wScale)));
    startTimeEditor.setBounds(timeRow.removeFromLeft(juce::roundToInt(80.0f * wScale)));
    timeRow.removeFromLeft(8);
    videoRangeBadgeLabel.setBounds(timeRow.removeFromRight(juce::roundToInt(255.0f * wScale)));
    timeRow.removeFromRight(8);
    startTimeSlider.setBounds(timeRow);

    topInner.removeFromTop(gapY);

    // Row 3: 3 Multi-Duration Extract Buttons (3s, 10s, 30s) + Conditional "Nueva Extracción" Button
    auto extractRow = topInner.removeFromTop(juce::roundToInt(34.0f * hScale));
    const int fourthW = (extractRow.getWidth() - 24) / 4;
    extract3sButton.setBounds(extractRow.removeFromLeft(fourthW));
    extractRow.removeFromLeft(8);
    extract10sButton.setBounds(extractRow.removeFromLeft(fourthW));
    extractRow.removeFromLeft(8);
    extract30sButton.setBounds(extractRow.removeFromLeft(fourthW));
    extractRow.removeFromLeft(8);
    newExtractionButton.setBounds(extractRow);

    topInner.removeFromTop(gapY);

    // Row 4: 3 Copyright & Derechos AI Buttons
    auto rightsRow = topInner.removeFromTop(juce::roundToInt(34.0f * hScale));
    const int rightsW = (rightsRow.getWidth() - 16) / 3;
    detectCopyrightBtn.setBounds(rightsRow.removeFromLeft(rightsW));
    rightsRow.removeFromLeft(8);
    derechosAiBtn.setBounds(rightsRow.removeFromLeft(rightsW));
    rightsRow.removeFromLeft(8);
    verifyFreeBtn.setBounds(rightsRow);

    topInner.removeFromTop(gapY);

    // Row 5: Progress Bar & Status + Copyright Verdict Badge
    auto progressRow = topInner.removeFromTop(juce::roundToInt(14.0f * hScale));
    progressBar.setBounds(progressRow);

    topInner.removeFromTop(3);
    auto statusRow = topInner.removeFromTop(juce::roundToInt(21.0f * hScale));
    copyrightBadgeLabel.setBounds(statusRow.removeFromRight(juce::roundToInt(315.0f * wScale)));
    statusLabel.setBounds(statusRow);

    totalArea.removeFromTop(6);

    // =========================================================================
    // 2. BOTTOM PANEL LAYOUT (FX MACRO + AUTO-CHOP MPC + DUAL WAVEFORMS + MIDI)
    // =========================================================================
    auto bottomArea = totalArea;
    bottomSectionGroup.setBounds(bottomArea);

    auto bottomInner = bottomArea.reduced(14, juce::roundToInt(20.0f * hScale));

    // Top row inside Section 2: FX Macro Knobs + Target Audio 1/2 + Sync + Auto-Chop MPC + BPM/Key Badge
    auto fxRow = bottomInner.removeFromTop(juce::roundToInt(40.0f * hScale));
    fxMacroTitleLabel.setBounds(fxRow.removeFromLeft(juce::roundToInt(88.0f * wScale)));

    fxTargetAudio1Btn.setBounds(fxRow.removeFromLeft(juce::roundToInt(78.0f * wScale)).reduced(0, 4));
    fxRow.removeFromLeft(4);
    fxTargetAudio2Btn.setBounds(fxRow.removeFromLeft(juce::roundToInt(78.0f * wScale)).reduced(0, 4));
    fxRow.removeFromLeft(8);

    const int knobDia = juce::roundToInt(38.0f * hScale);

    // Knob 1: Lo-Fi 12-Bit
    lofiKnob.setBounds(fxRow.removeFromLeft(knobDia));
    lofiLabel.setBounds(fxRow.removeFromLeft(juce::roundToInt(118.0f * wScale)));
    fxRow.removeFromLeft(4);

    // Knob 2: Pitch Semitones
    pitchKnob.setBounds(fxRow.removeFromLeft(knobDia));
    pitchLabel.setBounds(fxRow.removeFromLeft(juce::roundToInt(92.0f * wScale)));
    fxRow.removeFromLeft(4);

    // Knob 3: Tone Filter
    toneKnob.setBounds(fxRow.removeFromLeft(knobDia));
    toneLabel.setBounds(fxRow.removeFromLeft(juce::roundToInt(92.0f * wScale)));
    fxRow.removeFromLeft(6);

    // Sync, Bake FX & Auto-Chop MPC buttons on right side of FX bar
    autoChopButton.setBounds(fxRow.removeFromRight(juce::roundToInt(142.0f * wScale)).reduced(0, 4));
    fxRow.removeFromRight(5);
    bakeFxButton.setBounds(fxRow.removeFromRight(juce::roundToInt(92.0f * wScale)).reduced(0, 4));
    fxRow.removeFromRight(5);
    autoSyncButton.setBounds(fxRow.reduced(0, 4));

    bottomInner.removeFromTop(6);

    // Bottom action row: Play, Stop, BPM/Key Badge, Arrastrar .WAV, Arrastrar .MIDI, Exportar .WAV
    auto actionRow = bottomInner.removeFromBottom(juce::roundToInt(34.0f * hScale));
    playButton.setBounds(actionRow.removeFromLeft(juce::roundToInt(115.0f * wScale)));
    actionRow.removeFromLeft(6);
    stopButton.setBounds(actionRow.removeFromLeft(juce::roundToInt(82.0f * wScale)));
    actionRow.removeFromLeft(8);
    bpmKeyBadgeLabel.setBounds(actionRow.removeFromLeft(juce::roundToInt(235.0f * wScale)));
    actionRow.removeFromLeft(8);

    exportWavButton.setBounds(actionRow.removeFromRight(juce::roundToInt(122.0f * wScale)));
    actionRow.removeFromRight(6);

    const int dragBtnW = (actionRow.getWidth() - 6) / 2;
    dragDawButton.setBounds(actionRow.removeFromLeft(dragBtnW));
    actionRow.removeFromLeft(6);
    dragMidiButton.setBounds(actionRow);

    bottomInner.removeFromBottom(6);

    // Split remaining central area into 2 equal MPC Auto-Chop Waveform Viewers
    const int waveGap = 7;
    const int singleWaveHeight = (bottomInner.getHeight() - waveGap) / 2;
    originalWaveformComponent.setBounds(bottomInner.removeFromTop(singleWaveHeight));
    bottomInner.removeFromTop(waveGap);
    transformedWaveformComponent.setBounds(bottomInner);
}

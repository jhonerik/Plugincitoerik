#include "PluginEditor.h"

//==============================================================================
ClipSampler10AudioProcessorEditor::ClipSampler10AudioProcessorEditor(ClipSampler10AudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      waveformComponent(p)
{
    // Configure window size
    setSize(720, 560);

    // =========================================================================
    // Header Maintenance Controls
    // =========================================================================
    updateExtractorButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222733));
    updateExtractorButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb0bac9));
    updateExtractorButton.onClick = [this] { onUpdateExtractorClicked(); };
    addAndMakeVisible(updateExtractorButton);

    cleanCacheButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222733));
    cleanCacheButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb0bac9));
    cleanCacheButton.onClick = [this] { onCleanCacheClicked(); };
    addAndMakeVisible(cleanCacheButton);

    // =========================================================================
    // SECTION 1: TOP PANEL (LINK & CAPTURE)
    // =========================================================================
    topSectionGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(topSectionGroup);

    // YouTube URL input (restores last saved session state if available)
    urlLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0bac9));
    urlLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    addAndMakeVisible(urlLabel);

    urlEditor.setTextToShowWhenEmpty("https://www.youtube.com/watch?v=...", juce::Colour(0xff606875));
    urlEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1e222d));
    urlEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    urlEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2d3444));
    if (audioProcessor.getLastUrl().isNotEmpty())
        urlEditor.setText(audioProcessor.getLastUrl());
    addAndMakeVisible(urlEditor);

    pasteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3444));
    pasteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe0e6ed));
    pasteButton.onClick = [this] { onPasteClicked(); };
    addAndMakeVisible(pasteButton);

    // Start Time input (restores last saved session state if available)
    startTimeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0bac9));
    startTimeLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    addAndMakeVisible(startTimeLabel);

    juce::String savedTime = audioProcessor.getLastStartTime();
    startTimeEditor.setText(savedTime.isNotEmpty() ? savedTime : "00:00");
    startTimeEditor.setJustification(juce::Justification::centred);
    startTimeEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1e222d));
    startTimeEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xff00e5ff));
    startTimeEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2d3444));
    addAndMakeVisible(startTimeEditor);

    // Strict 10-Second Duration Badge (Locked in Code)
    durationBadgeLabel.setText(juce::String::fromUTF8("DURACIÓN MÁXIMA: 10s EXACTOS [BLOQUEADO]"), juce::dontSendNotification);
    durationBadgeLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    durationBadgeLabel.setJustificationType(juce::Justification::centred);
    durationBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0x33ff9800));
    durationBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffb74d));
    durationBadgeLabel.setColour(juce::Label::outlineColourId, juce::Colour(0x66ff9800));
    addAndMakeVisible(durationBadgeLabel);

    // Primary Action Button: Extract 10s Sample
    extractButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff0055));
    extractButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    extractButton.onClick = [this] { onExtractClicked(); };
    addAndMakeVisible(extractButton);

    // Progress bar and status label synchronized with Processor
    currentProgressValue = static_cast<double>(audioProcessor.getExtractionProgress());
    progressBar.setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xff161922));
    progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(progressBar);

    lastDisplayedStatus = audioProcessor.getStatusMessage();
    statusLabel.setText(lastDisplayedStatus, juce::dontSendNotification);
    statusLabel.setFont(juce::Font(12.0f, juce::Font::plain));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8c9ba5));
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    // =========================================================================
    // SECTION 2: BOTTOM PANEL (WAVEFORM & EXPORT)
    // =========================================================================
    bottomSectionGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(bottomSectionGroup);

    addAndMakeVisible(waveformComponent);

    // Playback Preview Buttons (routed via processBlock)
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00c853));
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    playButton.onClick = [this] { onPlayClicked(); };
    addAndMakeVisible(playButton);

    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3444));
    stopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    stopButton.onClick = [this] { onStopClicked(); };
    addAndMakeVisible(stopButton);

    // Direct DAW Track Drag & Drop Button (supports click feedback & mouse drag)
    dragDawButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6200ea));
    dragDawButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    dragDawButton.onClick = [this] { onDragDawClicked(); };
    dragDawButton.addMouseListener(this, false);
    addAndMakeVisible(dragDawButton);

    // Export .WAV FileChooser Button
    exportWavButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a73e8));
    exportWavButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    exportWavButton.onClick = [this] { onExportWavClicked(); };
    addAndMakeVisible(exportWavButton);

    // Start UI animation & state sync timer (30 FPS)
    startTimerHz(30);
}

ClipSampler10AudioProcessorEditor::~ClipSampler10AudioProcessorEditor()
{
    stopTimer();
    dragDawButton.removeMouseListener(this);
}

void ClipSampler10AudioProcessorEditor::timerCallback()
{
    // Synchronize extraction and update state from Processor
    const bool extracting = audioProcessor.isExtractionRunning();
    const bool updating = audioProcessor.isUpdateRunning();

    if (extractButton.isEnabled() == extracting)
        extractButton.setEnabled(!extracting);

    if (updateExtractorButton.isEnabled() == updating)
        updateExtractorButton.setEnabled(!updating);

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
    }

    if (audioProcessor.isAudioPlaying())
    {
        waveformComponent.repaint();
    }
}

void ClipSampler10AudioProcessorEditor::onPasteClicked()
{
    juce::String clip = juce::SystemClipboard::getTextFromClipboard().trim();
    if (clip.isNotEmpty())
    {
        urlEditor.setText(clip);
    }
}

void ClipSampler10AudioProcessorEditor::onExtractClicked()
{
    juce::String url = urlEditor.getText().trim();
    if (url.isEmpty())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Ingresa una URL válida de YouTube."));
        statusLabel.setText(audioProcessor.getStatusMessage(), juce::dontSendNotification);
        return;
    }

    audioProcessor.startSampleExtraction(url, startTimeEditor.getText());
    timerCallback();
}

void ClipSampler10AudioProcessorEditor::onPlayClicked()
{
    audioProcessor.startPlayback();
}

void ClipSampler10AudioProcessorEditor::onStopClicked()
{
    audioProcessor.stopPlayback();
    waveformComponent.repaint();
}

void ClipSampler10AudioProcessorEditor::onDragDawClicked()
{
    auto loadedFile = audioProcessor.getCurrentAudioFile();
    if (!loadedFile.existsAsFile())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero debes extraer una muestra para poder arrastrarla al DAW."));
        return;
    }

    audioProcessor.setStatusMessage(juce::String::fromUTF8("Tip: Mantén presionado y arrastra este botón o la onda hacia una pista del DAW."));
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

void ClipSampler10AudioProcessorEditor::onExportWavClicked()
{
    auto currentFile = audioProcessor.getCurrentAudioFile();
    if (!currentFile.existsAsFile())
    {
        audioProcessor.setStatusMessage(juce::String::fromUTF8("Primero extrae una muestra antes de exportar."));
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        juce::String::fromUTF8("Exportar muestra de 10s en formato WAV (24-bit PCM)"),
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
    // Main Dark Brushed Obsidian Background
    g.fillAll(juce::Colour(0xff12141a));

    // Plugin Header
    auto headerBounds = getLocalBounds().removeFromTop(48);
    g.setColour(juce::Colour(0xff1a1d26));
    g.fillRect(headerBounds);

    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("CLIPSAMPLER 10", 20, 6, 220, 24, juce::Justification::left);

    g.setColour(juce::Colour(0xff738096));
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.drawText("VIRTUAL INSTRUMENT // VSTi & AAX SAMPLER // 10s HARD CUT", 20, 28, 380, 16, juce::Justification::left);

    g.setColour(juce::Colour(0xff2d3444));
    g.drawHorizontalLine(48, 0.0f, (float)getWidth());
}

void ClipSampler10AudioProcessorEditor::resized()
{
    // 0. Position Header Maintenance Buttons
    auto headerArea = getLocalBounds().removeFromTop(48).reduced(12, 10);
    cleanCacheButton.setBounds(headerArea.removeFromRight(105));
    headerArea.removeFromRight(8);
    updateExtractorButton.setBounds(headerArea.removeFromRight(145));

    auto totalArea = getLocalBounds();
    totalArea.removeFromTop(54); // Space for top header
    totalArea.reduce(14, 8);

    int totalHeight = totalArea.getHeight();
    int topPanelHeight = 220;
    int bottomPanelHeight = totalHeight - topPanelHeight - 10;

    // =========================================================================
    // 1. TOP PANEL LAYOUT
    // =========================================================================
    auto topArea = totalArea.removeFromTop(topPanelHeight);
    topSectionGroup.setBounds(topArea);

    auto topInner = topArea.reduced(16, 24);

    // Row 1: URL input + Paste button
    auto urlRow = topInner.removeFromTop(32);
    urlLabel.setBounds(urlRow.removeFromLeft(125));
    pasteButton.setBounds(urlRow.removeFromRight(75));
    urlRow.removeFromRight(8);
    urlEditor.setBounds(urlRow);

    topInner.removeFromTop(12);

    // Row 2: Start time + Duration lock badge
    auto timeRow = topInner.removeFromTop(32);
    startTimeLabel.setBounds(timeRow.removeFromLeft(125));
    startTimeEditor.setBounds(timeRow.removeFromLeft(80));
    timeRow.removeFromLeft(14);
    durationBadgeLabel.setBounds(timeRow);

    topInner.removeFromTop(12);

    // Row 3: Extract Button
    auto extractRow = topInner.removeFromTop(36);
    extractButton.setBounds(extractRow);

    topInner.removeFromTop(10);

    // Row 4: Progress Bar & Status
    auto progressRow = topInner.removeFromTop(18);
    progressBar.setBounds(progressRow);

    topInner.removeFromTop(4);
    statusLabel.setBounds(topInner.removeFromTop(20));

    // Space between sections
    totalArea.removeFromTop(10);

    // =========================================================================
    // 2. BOTTOM PANEL LAYOUT
    // =========================================================================
    auto bottomArea = totalArea.removeFromTop(bottomPanelHeight);
    bottomSectionGroup.setBounds(bottomArea);

    auto bottomInner = bottomArea.reduced(16, 24);

    // Bottom action row: Play, Stop, Drag to DAW, Export .WAV
    auto actionRow = bottomInner.removeFromBottom(36);
    playButton.setBounds(actionRow.removeFromLeft(140));
    actionRow.removeFromLeft(8);
    stopButton.setBounds(actionRow.removeFromLeft(90));
    actionRow.removeFromLeft(16);

    exportWavButton.setBounds(actionRow.removeFromRight(130));
    actionRow.removeFromRight(8);
    dragDawButton.setBounds(actionRow);

    bottomInner.removeFromBottom(10);

    // Waveform component occupies the remaining central area
    waveformComponent.setBounds(bottomInner);
}

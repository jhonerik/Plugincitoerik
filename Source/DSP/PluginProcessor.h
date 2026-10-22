#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "../Services/AudioExtractorWorker.h"
#include "../Security/LicenseManager.h"

/**
 * ClipSampler10AudioProcessor (ClipSampler v2.2)
 * 
 * VSTi / AAX Virtual Instrument DSP Engine & Dual-Slot AI Copyright Evasion Coordinator.
 * Features:
 * - Multi-duration YouTube extraction: 3s, 10s, or 30s.
 * - Automatic YouTube video duration & Content ID metadata detection.
 * - Dual Waveform Slots:
 *     Slot 1: Original Extracted Audio (3s / 10s / 30s)
 *     Slot 2: "Derechos IA" Transformed Audio (±3 Semitones + Spectral/Phase Fingerprint Evasion)
 * - Built-in Copyright Risk Detector (Audio 1) & Post-Transform Fingerprint Verifier (Audio 2).
 */
class ClipSampler10AudioProcessor : public juce::AudioProcessor
{
public:
    ClipSampler10AudioProcessor();
    ~ClipSampler10AudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    // DAW State Persistence (APVTS + Session Metadata)
    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // ClipSampler v2.2 Custom Control, Multi-Duration & Derechos AI API
    //==============================================================================
    /** Starts a background YouTube extraction for the specified duration (3s, 10s, or 30s). */
    void startSampleExtraction(const juce::String& url, const juce::String& startTime, int durationSeconds = 10);

    /** Queries the YouTube URL asynchronously to detect total video duration and copyright metadata. */
    void detectVideoDuration(const juce::String& url);

    /** Analyzes Slot 1 (Original Audio) for copyright / Content ID fingerprint risk. */
    bool analyzeOriginalCopyright(juce::String& outReport);

    /**
     * Generates Slot 2 ("Derechos IA") from Slot 1 by shifting ±3 semitones and applying
     * all-pass phase dispersion + stereo micro-modulation to evade acoustic fingerprinting.
     */
    bool applyDerechosTransformation(int semitoneShift, juce::String& outReport);

    /** Verifies whether Slot 2 (Transformed Audio) is sufficiently decorrelated from Slot 1's copyright fingerprint. */
    bool verifyTransformedAudioCopyrightFree(juce::String& outReport);

    /** Switches active playback/MIDI/drag slot (1 = Original, 2 = Transformado Derechos). */
    void setActiveSlot(int slotIndex);
    int getActiveSlot() const { return activeSlot.load(std::memory_order_relaxed); }

    /** Triggers an asynchronous yt-dlp -U update managed safely by the Processor. */
    void startExtractorUpdate();

    /** Cleans cached .wav files while preserving both Slot 1 and Slot 2 active samples. */
    int cleanAudioCache();

    /** Loads a .wav file into Slot 1 (Original) or Slot 2 (Transformed) and makes it active. */
    bool loadAudioFileIntoSlot(const juce::File& file, int slotIndex);

    /** Legacy helper: loads into Slot 1. */
    bool loadAudioFile(const juce::File& file) { return loadAudioFileIntoSlot(file, 1); }

    /** Triggers playback of the active slot sample from the start. */
    void startPlayback();

    /** Immediately stops sample playback. */
    void stopPlayback();

    /** Checks if audio is currently rendering in processBlock. */
    bool isAudioPlaying() const { return isPlaying.load(std::memory_order_relaxed); }

    /** Returns normalized playback progress (0.0 to 1.0). */
    float getPlaybackProgress() const;

    /** Returns the currently active audio file (or specific slot 1 / 2). */
    juce::File getCurrentAudioFile() const;
    juce::File getSlotAudioFile(int slotIndex) const;

    /** Returns the sample rate of the currently loaded audio file. */
    double getLoadedFileSampleRate() const { return loadedFileSampleRate.load(std::memory_order_relaxed); }

    /** Extraction, Metadata & Update status accessors for the UI */
    bool isExtractionRunning() const { return extractionInProgress.load(std::memory_order_relaxed); }
    bool isUpdateRunning() const { return updateInProgress.load(std::memory_order_relaxed); }
    bool isMetadataFetchRunning() const { return metadataInProgress.load(std::memory_order_relaxed); }
    float getExtractionProgress() const { return extractionProgress.load(std::memory_order_relaxed); }
    int getDetectedVideoDurationSeconds() const { return detectedVideoDurationSec.load(std::memory_order_relaxed); }
    int getLastSelectedDurationSeconds() const { return lastSelectedDurationSec.load(std::memory_order_relaxed); }
    int getLastAppliedSemitones() const { return lastAppliedSemitones.load(std::memory_order_relaxed); }
    double getLastDivergencePercent() const { return lastDivergencePercent.load(std::memory_order_relaxed); }

    juce::String getStatusMessage() const;
    void setStatusMessage(const juce::String& msg);
    juce::String getCopyrightBadgeText() const;
    int getCopyrightVerifyState() const { return copyrightVerifyState.load(std::memory_order_relaxed); }

    /** Dual Waveform thumbnails access for the GUI (Slot 1 = Original, Slot 2 = Derechos Transformed). */
    juce::AudioThumbnail& getThumbnail(int slotIndex = 1) { return (slotIndex == 2) ? transformedThumbnail : originalThumbnail; }

    juce::AudioFormatManager& getFormatManager() { return formatManager; }

    /** Security & 3-Day Trial License Manager access */
    LicenseManager& getLicenseManager() { return licenseManager; }
    const LicenseManager& getLicenseManager() const { return licenseManager; }

    // Session state accessors
    juce::String getLastUrl() const { return lastUrl; }
    void setLastUrl(const juce::String& url) { lastUrl = url; }

    juce::String getLastStartTime() const { return lastStartTime; }
    void setLastStartTime(const juce::String& time) { lastStartTime = time; }

private:
    bool activateSlotBuffer(const juce::File& file, int slotIndex);

    juce::AudioProcessorValueTreeState apvts;

    juce::Synthesiser sampler;
    static constexpr int NUM_VOICES = 8;

    AudioExtractorWorker extractorWorker;
    LicenseManager licenseManager;

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 10 };
    juce::AudioThumbnail originalThumbnail;
    juce::AudioThumbnail transformedThumbnail;

    mutable juce::CriticalSection bufferLock;
    juce::AudioBuffer<float> sampleBuffer;
    juce::File originalLoadedFile;
    juce::File transformedLoadedFile;
    std::atomic<int> activeSlot { 1 };
    std::atomic<double> loadedFileSampleRate { 44100.0 };

    // Stored session parameters
    juce::String lastUrl;
    juce::String lastStartTime = "00:00";
    juce::String detectedVideoTitle;
    juce::String detectedLicenseInfo;
    std::atomic<bool> detectedHasCopyrightMeta { true };
    std::atomic<int> detectedVideoDurationSec { 0 };
    std::atomic<int> lastSelectedDurationSec { 10 };
    std::atomic<int> lastAppliedSemitones { 3 };
    std::atomic<double> lastDivergencePercent { 0.0 };
    // 0 = Not checked, 1 = Original has Copyright Risk, 2 = Transformed Verified Copyright-Free
    std::atomic<int> copyrightVerifyState { 0 };

    // Thread-safe status state for Editor synchronization
    mutable juce::CriticalSection statusLock;
    juce::String currentStatusMessage;
    juce::String currentCopyrightBadge;
    std::atomic<bool> extractionInProgress { false };
    std::atomic<bool> updateInProgress { false };
    std::atomic<bool> metadataInProgress { false };
    std::atomic<float> extractionProgress { 0.0f };

    // Atomic playback status
    std::atomic<bool> isPlaying { false };
    std::atomic<int> playheadPosition { 0 };
    std::atomic<int> totalSampleCount { 0 };

    std::shared_ptr<std::atomic<bool>> aliveFlag = std::make_shared<std::atomic<bool>>(true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSampler10AudioProcessor)
};

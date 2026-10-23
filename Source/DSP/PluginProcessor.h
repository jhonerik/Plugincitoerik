#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <vector>
#include "../Services/AudioExtractorWorker.h"
#include "../Security/LicenseManager.h"

/**
 * Represents a single rhythmic transient slice in MPC "Auto-Chop" mode,
 * mapped chromatically from C1 (MIDI 36) to G2 (MIDI 55).
 */
struct ChopSlice
{
    int midiNote = 36;
    juce::String noteName;
    int startSample = 0;
    int endSample = 0;
    float startNorm = 0.0f;
    float endNorm = 1.0f;
};

/**
 * ClipSampler10AudioProcessor (ClipSampler v2.3)
 * 
 * VSTi / AAX Virtual Instrument DSP Engine, MPC Auto-Chop Slicer,
 * BPM & Musical Key Detector, FX Macro Processor, Audio-to-MIDI Converter,
 * and Dual-Slot AI Copyright Evasion Coordinator.
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
    // ClipSampler v2.4 Multi-Platform (YouTube / TikTok / Instagram), Multi-Duration & Derechos AI API
    //==============================================================================
    void setSelectedPlatform(SourcePlatform platform) { selectedPlatform.store(static_cast<int>(platform), std::memory_order_relaxed); }
    SourcePlatform getSelectedPlatform() const { return static_cast<SourcePlatform>(selectedPlatform.load(std::memory_order_relaxed)); }

    /** Starts a background multi-platform extraction for the specified duration (3s, 10s, or 30s). */
    void startSampleExtraction(const juce::String& url, const juce::String& startTime, int durationSeconds = 10);

    /** Queries the URL asynchronously (YouTube, TikTok, or Instagram) to detect total video duration and metadata. */
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

    /** Returns true if the given slot (1 or 2) currently has a valid audio file loaded. */
    bool hasSlotAudio(int slotIndex) const;

    /**
     * Condition requested for v2.3:
     * Returns true ONLY when BOTH Audio 1 (Original) and Audio 2 (Transformed/Extraído 2) are loaded,
     * enabling the "🔄 Nueva Extracción" button to wipe and reset the entire plugin state.
     */
    bool hasBothAudiosReady() const;

    /**
     * Clears both waveform slots, slices, BPM/Key metadata, FX state, and URL/time fields
     * so the plugin is completely clean for a brand-new extraction.
     */
    void resetForNewExtraction();

    /** Switches active playback/MIDI/drag slot (1 = Original, 2 = Transformado Derechos). */
    void setActiveSlot(int slotIndex);
    int getActiveSlot() const { return activeSlot.load(std::memory_order_relaxed); }

    //==============================================================================
    // v2.3: MPC Auto-Chop Mode (Transient Detection C1 -> G2)
    //==============================================================================
    void setAutoChopEnabled(bool enabled);
    bool isAutoChopEnabled() const { return autoChopEnabled.load(std::memory_order_relaxed); }

    /** Re-runs transient detection on the specified slot and populates C1..G2 slices. */
    void analyzeAutoChopAndMusicData(int slotIndex);

    /** Returns the list of detected MPC transient slices for the requested slot (1 or 2). */
    std::vector<ChopSlice> getChopSlices(int slotIndex) const;

    /** Triggers playback of a specific MPC slice index (0 = C1, 1 = C#1, etc.) on the active slot. */
    void triggerChopSlice(int sliceIndex);

    /** Returns the currently playing slice index, or -1 if playing full sample / stopped. */
    int getActivePlayingSlice() const { return activePlayingSlice.load(std::memory_order_relaxed); }

    //==============================================================================
    // v2.3: FX Macro (Lo-Fi 12-Bit, Pitch Semitones, Filter, Auto-Sync) per Slot
    //==============================================================================
    void setFxTargetSlot(int slotIndex);
    int getFxTargetSlot() const { return fxTargetSlot.load(std::memory_order_relaxed); }

    void setLofiAmount(float amount0to1);
    float getLofiAmount() const { return fxLofiAmount.load(std::memory_order_relaxed); }

    void setFxPitchSemitones(float semitones);
    float getFxPitchSemitones() const { return fxPitchSemitones.load(std::memory_order_relaxed); }

    void setFxToneFilter(float tone0to1);
    float getFxToneFilter() const { return fxToneFilter.load(std::memory_order_relaxed); }

    void setAutoSyncEnabled(bool enabled);
    bool isAutoSyncEnabled() const { return autoSyncEnabled.load(std::memory_order_relaxed); }

    /** Bakes the current FX Macro (Lo-Fi 12-Bit, Pitch, Tone, Auto-Sync) into the target slot (1 or 2). */
    bool applyFxMacroToTargetSlot(juce::String& outReport);

    //==============================================================================
    // v2.3: Automatic BPM + Musical Key (Tonalidad) Detection
    //==============================================================================
    double getDetectedBpm(int slotIndex) const;
    juce::String getDetectedKey(int slotIndex) const;
    double getHostBpm() const { return currentHostBpm.load(std::memory_order_relaxed); }

    //==============================================================================
    // v2.3: Audio-to-MIDI Converter (Drag & Drop .MIDI for Piano Roll)
    //==============================================================================
    /** Analyses pitched notes & rhythmic onsets in the given slot and writes a .mid file. */
    juce::File generateMidiFileForSlot(int slotIndex);
    juce::File getCurrentMidiFile();

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

    static juce::String midiNoteToName(int midiNote);

private:
    bool activateSlotBuffer(const juce::File& file, int slotIndex);
    static std::vector<ChopSlice> computeTransientSlices(const juce::AudioBuffer<float>& buffer, double sampleRate);
    static double estimateBpmFromBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);
    static juce::String estimateMusicalKeyFromBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);

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
    juce::File slot1MidiFile;
    juce::File slot2MidiFile;
    std::atomic<int> activeSlot { 1 };
    std::atomic<double> loadedFileSampleRate { 44100.0 };

    // Auto-Chop MPC & BPM/Key state per slot
    std::atomic<bool> autoChopEnabled { true };
    std::vector<ChopSlice> slot1Slices;
    std::vector<ChopSlice> slot2Slices;
    std::atomic<double> slot1Bpm { 0.0 };
    std::atomic<double> slot2Bpm { 0.0 };
    juce::String slot1Key { "--" };
    juce::String slot2Key { "--" };
    std::atomic<double> currentHostBpm { 120.0 };

    // FX Macro state
    std::atomic<int> fxTargetSlot { 1 };
    std::atomic<float> fxLofiAmount { 0.0f };       // 0.0 .. 1.0 (Lo-Fi 12-Bit & sample hold)
    std::atomic<float> fxPitchSemitones { 0.0f };   // -12.0 .. +12.0 semitones
    std::atomic<float> fxToneFilter { 1.0f };       // 0.1 .. 1.0 low-pass warmth
    std::atomic<bool> autoSyncEnabled { false };
    float lofiFilterStateL = 0.0f;
    float lofiFilterStateR = 0.0f;

    // Stored session parameters
    std::atomic<int> selectedPlatform { 0 }; // 0 = YouTube, 1 = TikTok, 2 = Instagram
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

    // Real-time playback state (supports fractional resampling for Pitch + Auto-Sync + Auto-Chop slice bounds)
    std::atomic<bool> isPlaying { false };
    std::atomic<double> fractionalPlayhead { 0.0 };
    std::atomic<int> playheadPosition { 0 };
    std::atomic<int> playbackEndSample { 0 };
    std::atomic<int> totalSampleCount { 0 };
    std::atomic<int> activePlayingSlice { -1 };

    std::shared_ptr<std::atomic<bool>> aliveFlag = std::make_shared<std::atomic<bool>>(true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSampler10AudioProcessor)
};

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "../Services/AudioExtractorWorker.h"

/**
 * ClipSampler10AudioProcessor
 * 
 * VSTi / AAX Virtual Instrument DSP Engine & Session Coordinator.
 * Features:
 * - Real-time juce::Synthesiser with juce::SamplerSound / juce::SamplerVoice
 *   allowing full MIDI Piano Roll triggering (Note-On with pitch tracking).
 * - Wait-free ScopedTryLock buffer streaming in processBlock for GUI preview.
 * - Owns AudioExtractorWorker so background extractions continue even if the GUI window is closed.
 * - Extensible AudioProcessorValueTreeState (APVTS) for future DSP parameters and DAW state persistence.
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
    // ClipSampler10 Custom Control & Extraction API
    //==============================================================================
    /** Starts a background 10-second YouTube extraction managed by the Processor. */
    void startSampleExtraction(const juce::String& url, const juce::String& startTime);

    /** Triggers an asynchronous yt-dlp -U update managed safely by the Processor. */
    void startExtractorUpdate();

    /** Cleans cached .wav files while preserving the currently loaded sample. */
    int cleanAudioCache();

    /** Loads a .wav file into both the SamplerVoice synthesiser and preview buffer. */
    bool loadAudioFile(const juce::File& file);

    /** Triggers playback of the 10s sample from the start. */
    void startPlayback();

    /** Immediately stops sample playback. */
    void stopPlayback();

    /** Checks if audio is currently rendering in processBlock. */
    bool isAudioPlaying() const { return isPlaying.load(std::memory_order_relaxed); }

    /** Returns normalized playback progress (0.0 to 1.0). */
    float getPlaybackProgress() const;

    /** Returns the currently loaded audio file. */
    juce::File getCurrentAudioFile() const;

    /** Returns the sample rate of the currently loaded audio file (or DAW sample rate). */
    double getLoadedFileSampleRate() const { return loadedFileSampleRate.load(std::memory_order_relaxed); }

    /** Extraction & Update status accessors for the UI */
    bool isExtractionRunning() const { return extractionInProgress.load(std::memory_order_relaxed); }
    bool isUpdateRunning() const { return updateInProgress.load(std::memory_order_relaxed); }
    float getExtractionProgress() const { return extractionProgress.load(std::memory_order_relaxed); }
    juce::String getStatusMessage() const;
    void setStatusMessage(const juce::String& msg);

    /** Waveform thumbnail access for the GUI. */
    juce::AudioThumbnail& getThumbnail() { return thumbnail; }

    juce::AudioFormatManager& getFormatManager() { return formatManager; }

    // Session state accessors
    juce::String getLastUrl() const { return lastUrl; }
    void setLastUrl(const juce::String& url) { lastUrl = url; }

    juce::String getLastStartTime() const { return lastStartTime; }
    void setLastStartTime(const juce::String& time) { lastStartTime = time; }

private:
    // Extensible parameter tree for future controls (Gain, Pitch, ADSR, Filter, Loop)
    juce::AudioProcessorValueTreeState apvts;

    // Core Synthesiser engine for MIDI triggering (Piano Roll)
    juce::Synthesiser sampler;
    static constexpr int NUM_VOICES = 8;

    // Background worker owned by Processor so closing Editor never aborts extraction
    AudioExtractorWorker extractorWorker;

    // Audio formats and waveform thumbnail
    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 5 };
    juce::AudioThumbnail thumbnail;

    // Sample playback buffer (swapped in O(1), read via ScopedTryLock in processBlock)
    mutable juce::CriticalSection bufferLock;
    juce::AudioBuffer<float> sampleBuffer;
    juce::File currentLoadedFile;
    std::atomic<double> loadedFileSampleRate { 44100.0 };

    // Stored session parameters
    juce::String lastUrl;
    juce::String lastStartTime = "00:00";

    // Thread-safe status state for Editor synchronization
    mutable juce::CriticalSection statusLock;
    juce::String currentStatusMessage;
    std::atomic<bool> extractionInProgress { false };
    std::atomic<bool> updateInProgress { false };
    std::atomic<float> extractionProgress { 0.0f };

    // Atomic playback status
    std::atomic<bool> isPlaying { false };
    std::atomic<int> playheadPosition { 0 };
    std::atomic<int> totalSampleCount { 0 };

    // Lifetime guard for static async callbacks
    std::shared_ptr<std::atomic<bool>> aliveFlag = std::make_shared<std::atomic<bool>>(true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSampler10AudioProcessor)
};

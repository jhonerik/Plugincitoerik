#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include <atomic>

/**
 * AudioExtractorWorker
 * 
 * Background worker thread responsible for executing yt-dlp and ffmpeg to download
 * a precise 10-second slice of audio from a given YouTube URL and converting it to
 * an uncompressed 24-bit PCM .wav file dynamically matched to the DAW's sample rate.
 * 
 * Features:
 * - Dynamic DAW Sample Rate Synchronization (e.g. 44.1k, 48k, 96k, 192k)
 * - Single-pass Anti-Click DSP Filtering: 10ms micro-fade in & out plus -1dB normalization
 * - Clean descriptive filenames: %(title).40s_%(section_start)s.%(ext)s
 * - Cached executable resolution (yt-dlp, ffmpeg) with zero hardcoded C:\ paths
 * - Safe cache cleanup that preserves the currently loaded sample
 */
class AudioExtractorWorker : public juce::Thread
{
public:
    using ProgressCallback = std::function<void(float progress0to1, const juce::String& statusMessage)>;
    using CompletionCallback = std::function<void(bool success, const juce::File& resultFile, const juce::String& errorMessage)>;

    AudioExtractorWorker();
    ~AudioExtractorWorker() override;

    /** Starts an extraction job on a background thread with DAW sample rate matching. */
    void startExtraction(const juce::String& youtubeUrl,
                         const juce::String& startTimeString,
                         int targetSampleRate,
                         ProgressCallback progressCb,
                         CompletionCallback completionCb);

    /** Cancels any running extraction and cleans up child processes. */
    void cancelExtraction();

    /** Checks if a background job is actively processing. */
    bool isExtracting() const { return isThreadRunning(); }

    /** Helper to parse "MM:SS", "HH:MM:SS" or raw seconds into integer seconds. */
    static int parseTimeToSeconds(const juce::String& timeString);

    /** Helper to format seconds back to "HH:MM:SS" */
    static juce::String formatSecondsToTime(int totalSeconds);

    /** Locates cache folder used by ClipSampler10. */
    static juce::File getCacheDirectory();

    /**
     * Deletes cached .wav and temporary files, excluding the currently active file in the processor.
     * Returns the number of deleted files.
     */
    static int clearCacheDirectory(const juce::File& activeFileToKeep = {});

    /** Runs yt-dlp -U asynchronously with real timeout protection. */
    static void runYtDlpUpdate(std::function<void(bool success, const juce::String& outputMsg)> onComplete);

    /** Locates executable binaries (yt-dlp, ffmpeg) across AppData, plugin tools and PATH (with in-memory cache). */
    static juce::File findExecutable(const juce::String& executableName);

    /** Strictly enforced duration limit in seconds. */
    static constexpr int MAX_DURATION_SECONDS = 10;

private:
    void run() override;

    // Audio file post-validation (ensures <= 10.0s exact cut, 10ms anti-click fades and -1 dB peak normalization)
    bool validateAndEnforceDuration(const juce::File& audioFile, int targetRate);

    // Terminates any orphaned child processes on Windows when cancelling
    void killActiveProcessTree();

    juce::String currentUrl;
    juce::String currentStartTimeStr;
    int currentSampleRate = 44100;
    ProgressCallback progressCallback;
    CompletionCallback completionCallback;

    juce::ChildProcess activeProcess;
    std::atomic<bool> isCancelled { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExtractorWorker)
};

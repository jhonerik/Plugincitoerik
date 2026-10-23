#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include <atomic>

/**
 * Supported source platforms for multi-platform audio extraction & analysis.
 */
enum class SourcePlatform
{
    YouTube = 0,
    TikTok = 1,
    Instagram = 2
};

/**
 * AudioExtractorWorker (v2.4 Multi-Platform Engine)
 * 
 * Background worker thread responsible for:
 * 1. Inspecting video duration, creator, & copyright metadata across YouTube, TikTok, and Instagram.
 * 2. Platform-specific URL normalization, anti-block headers (User-Agent / Referer), and stream selection.
 * 3. Downloading a 3-second, 10-second, or 30-second slice of audio from YouTube, TikTok, or Instagram
 *    and converting it to an uncompressed 24-bit PCM .wav file matched to the DAW's sample rate.
 */
class AudioExtractorWorker : public juce::Thread
{
public:
    using ProgressCallback = std::function<void(float progress0to1, const juce::String& statusMessage)>;
    using CompletionCallback = std::function<void(bool success, const juce::File& resultFile, const juce::String& errorMessage)>;

    struct VideoMetadata
    {
        bool success = false;
        SourcePlatform platform = SourcePlatform::YouTube;
        int durationSeconds = 0;
        juce::String title;
        juce::String licenseInfo;
        juce::String trackInfo;
        bool hasCopyrightMetadata = false;
        juce::String errorMessage;
    };

    AudioExtractorWorker();
    ~AudioExtractorWorker() override;

    /** Detects whether a URL belongs to YouTube, TikTok, or Instagram. */
    static SourcePlatform detectPlatformFromUrl(const juce::String& url);

    /** Returns display name ("YouTube", "TikTok", "Instagram"). */
    static juce::String getPlatformName(SourcePlatform platform);

    /** Normalizes platform links (e.g., Instagram /reels/ -> /reel/, strips tracking garbage). */
    static juce::String normalizeUrlForPlatform(const juce::String& rawUrl, SourcePlatform platform);

    /** Validates whether the URL matches the selected platform (or is a valid supported media URL). */
    static bool isValidPlatformUrl(const juce::String& url, SourcePlatform platform, juce::String& outReason);

    /** Starts an extraction job on a background thread with platform-specific handling and duration (3s, 10s, 30s). */
    void startExtraction(const juce::String& mediaUrl,
                         SourcePlatform platform,
                         const juce::String& startTimeString,
                         int durationSeconds,
                         int targetSampleRate,
                         ProgressCallback progressCb,
                         CompletionCallback completionCb);

    /** Cancels any running extraction and cleans up child processes. */
    void cancelExtraction();

    /** Checks if a background job is actively processing. */
    bool isExtracting() const { return isThreadRunning(); }

    /** Fetches video total duration and copyright metadata asynchronously for YouTube, TikTok, or Instagram. */
    static void fetchVideoMetadata(const juce::String& mediaUrl,
                                   SourcePlatform platform,
                                   std::function<void(const VideoMetadata&)> onComplete);

    /** Helper to parse "MM:SS", "HH:MM:SS" or raw seconds into integer seconds. */
    static int parseTimeToSeconds(const juce::String& timeString);

    /** Helper to format seconds back to "HH:MM:SS" */
    static juce::String formatSecondsToTime(int totalSeconds);

    /** Helper to format seconds to compact "MM:SS" (or "HH:MM:SS" if >= 1 hour). */
    static juce::String formatSecondsToMMSS(int totalSeconds);

    /** Locates cache folder used by ClipSampler. */
    static juce::File getCacheDirectory();

    /** Deletes cached .wav and temporary files, excluding the currently active files. */
    static int clearCacheDirectory(const juce::File& activeFile1 = {}, const juce::File& activeFile2 = {});

    /** Runs yt-dlp -U asynchronously with real timeout protection. */
    static void runYtDlpUpdate(std::function<void(bool success, const juce::String& outputMsg)> onComplete);

    /** Locates executable binaries (yt-dlp, ffmpeg) across AppData, plugin tools and PATH. */
    static juce::File findExecutable(const juce::String& executableName);

private:
    void run() override;

    bool validateAndEnforceDuration(const juce::File& audioFile, int targetRate, int startOffsetSec, int maxDurationSec);
    void killActiveProcessTree();
    static void appendPlatformSpecificArgs(juce::StringArray& args, SourcePlatform platform);

    juce::String currentUrl;
    SourcePlatform currentPlatform = SourcePlatform::YouTube;
    juce::String currentStartTimeStr;
    int currentDurationSec = 10;
    int currentSampleRate = 44100;
    ProgressCallback progressCallback;
    CompletionCallback completionCallback;

    juce::ChildProcess activeProcess;
    std::atomic<bool> isCancelled { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExtractorWorker)
};

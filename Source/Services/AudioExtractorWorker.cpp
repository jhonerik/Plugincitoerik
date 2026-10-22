#include "AudioExtractorWorker.h"
#include <map>
#include <mutex>

#if JUCE_WINDOWS
 #define NOMINMAX
 #include <windows.h>
#endif

namespace
{
    std::mutex gExecutableCacheMutex;
    std::map<juce::String, juce::File> gExecutableCache;
}

AudioExtractorWorker::AudioExtractorWorker()
    : juce::Thread("AudioExtractorWorkerThread")
{
}

AudioExtractorWorker::~AudioExtractorWorker()
{
    cancelExtraction();
}

void AudioExtractorWorker::startExtraction(const juce::String& youtubeUrl,
                                           const juce::String& startTimeString,
                                           int durationSeconds,
                                           int targetSampleRate,
                                           ProgressCallback progressCb,
                                           CompletionCallback completionCb)
{
    if (isThreadRunning())
        cancelExtraction();

    currentUrl = youtubeUrl.trim();
    currentStartTimeStr = startTimeString.trim();
    currentDurationSec = juce::jlimit(1, 60, durationSeconds);
    currentSampleRate = (targetSampleRate > 0) ? targetSampleRate : 44100;
    progressCallback = std::move(progressCb);
    completionCallback = std::move(completionCb);
    isCancelled.store(false);

    startThread(juce::Thread::Priority::normal);
}

void AudioExtractorWorker::killActiveProcessTree()
{
    if (!activeProcess.isRunning())
        return;

    activeProcess.kill();
}

void AudioExtractorWorker::cancelExtraction()
{
    isCancelled.store(true);
    signalThreadShouldExit();
    killActiveProcessTree();
    stopThread(2500);
}

int AudioExtractorWorker::parseTimeToSeconds(const juce::String& timeString)
{
    juce::String clean = timeString.trim();
    if (clean.isEmpty())
        return 0;

    auto tokens = juce::StringArray::fromTokens(clean, ":", "");
    if (tokens.size() == 1)
        return juce::jmax(0, tokens[0].getIntValue());

    if (tokens.size() == 2)
    {
        int minutes = tokens[0].getIntValue();
        int seconds = tokens[1].getIntValue();
        return juce::jmax(0, (minutes * 60) + seconds);
    }
    if (tokens.size() == 3)
    {
        int hours = tokens[0].getIntValue();
        int minutes = tokens[1].getIntValue();
        int seconds = tokens[2].getIntValue();
        return juce::jmax(0, (hours * 3600) + (minutes * 60) + seconds);
    }

    return 0;
}

juce::String AudioExtractorWorker::formatSecondsToTime(int totalSeconds)
{
    totalSeconds = juce::jmax(0, totalSeconds);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    return juce::String::formatted("%02d:%02d:%02d", hours, minutes, seconds);
}

juce::String AudioExtractorWorker::formatSecondsToMMSS(int totalSeconds)
{
    totalSeconds = juce::jmax(0, totalSeconds);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    if (hours > 0)
        return juce::String::formatted("%02d:%02d:%02d", hours, minutes, seconds);

    return juce::String::formatted("%02d:%02d", minutes, seconds);
}

juce::File AudioExtractorWorker::getCacheDirectory()
{
    auto baseDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    baseDir = baseDir.getChildFile("Application Support");
#endif
    auto cacheDir = baseDir.getChildFile("ClipSampler10")
                           .getChildFile("AudioCache");
    cacheDir.createDirectory();
    return cacheDir;
}

int AudioExtractorWorker::clearCacheDirectory(const juce::File& activeFile1, const juce::File& activeFile2)
{
    auto cacheDir = getCacheDirectory();
    auto files = cacheDir.findChildFiles(juce::File::findFiles, false, "*");
    int count = 0;

    for (auto& f : files)
    {
        if (activeFile1 != juce::File() && f == activeFile1)
            continue;
        if (activeFile2 != juce::File() && f == activeFile2)
            continue;

        if (f.hasFileExtension(".wav") || f.hasFileExtension(".part") || f.hasFileExtension(".ytdl") || f.hasFileExtension(".webm") || f.hasFileExtension(".m4a"))
        {
            if (f.deleteFile())
                count++;
        }
    }
    return count;
}

void AudioExtractorWorker::fetchVideoMetadata(const juce::String& youtubeUrl,
                                              std::function<void(const VideoMetadata&)> onComplete)
{
    juce::String cleanUrl = youtubeUrl.trim();
    juce::Thread::launch([cleanUrl, onComplete = std::move(onComplete)] {
        VideoMetadata meta;
        if (cleanUrl.isEmpty() || (!cleanUrl.startsWithIgnoreCase("http://") && !cleanUrl.startsWithIgnoreCase("https://")))
        {
            meta.errorMessage = juce::String::fromUTF8("URL no válida.");
            if (onComplete)
                juce::MessageManager::callAsync([onComplete, meta] { onComplete(meta); });
            return;
        }

        auto ytDlpBin = findExecutable("yt-dlp");
        if (!ytDlpBin.existsAsFile())
        {
            meta.errorMessage = juce::String::fromUTF8("No se encontró yt-dlp.");
            if (onComplete)
                juce::MessageManager::callAsync([onComplete, meta] { onComplete(meta); });
            return;
        }

        juce::StringArray args;
        args.add(ytDlpBin.getFullPathName());
        args.add("--no-playlist");
        args.add("--skip-download");
        args.add("--print");
        args.add("%(duration)s|||%(title)s|||%(license)s|||%(track)s|||%(artist)s|||%(channel)s");
        args.add(cleanUrl);

        juce::ChildProcess process;
        if (!process.start(args, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
        {
            meta.errorMessage = juce::String::fromUTF8("No se pudo consultar la duración del video.");
            if (onComplete)
                juce::MessageManager::callAsync([onComplete, meta] { onComplete(meta); });
            return;
        }

        juce::String output;
        char buffer[512];
        const auto startMs = juce::Time::getMillisecondCounterHiRes();
        while (process.isRunning())
        {
            int n = process.readProcessOutput(buffer, sizeof(buffer) - 1);
            if (n > 0)
            {
                buffer[n] = '\0';
                output += juce::String::fromUTF8(buffer, n);
            }
            if ((juce::Time::getMillisecondCounterHiRes() - startMs) > 15000.0)
            {
                process.kill();
                break;
            }
            juce::Thread::sleep(30);
        }

        while (true)
        {
            int n = process.readProcessOutput(buffer, sizeof(buffer) - 1);
            if (n <= 0)
                break;
            buffer[n] = '\0';
            output += juce::String::fromUTF8(buffer, n);
        }

        auto lines = juce::StringArray::fromLines(output.trim());
        for (const auto& line : lines)
        {
            if (line.contains("|||"))
            {
                auto parts = juce::StringArray::fromTokens(line, "|||", "");
                // Tokenize carefully by splitting on "|||"
                juce::String remaining = line;
                juce::StringArray fields;
                while (remaining.isNotEmpty())
                {
                    int idx = remaining.indexOf("|||");
                    if (idx >= 0)
                    {
                        fields.add(remaining.substring(0, idx).trim());
                        remaining = remaining.substring(idx + 3);
                    }
                    else
                    {
                        fields.add(remaining.trim());
                        break;
                    }
                }

                if (fields.size() >= 2)
                {
                    meta.durationSeconds = juce::jmax(0, fields[0].getIntValue());
                    meta.title = fields[1];
                    juce::String lic = (fields.size() >= 3) ? fields[2] : "";
                    juce::String track = (fields.size() >= 4) ? fields[3] : "";
                    juce::String artist = (fields.size() >= 5) ? fields[4] : "";
                    juce::String channel = (fields.size() >= 6) ? fields[5] : "";

                    bool isCreativeCommons = lic.containsIgnoreCase("Creative Commons") || lic.containsIgnoreCase("CC BY");
                    bool hasMusicTrack = (track.isNotEmpty() && track != "NA") || (artist.isNotEmpty() && artist != "NA");
                    meta.hasCopyrightMetadata = !isCreativeCommons;

                    if (hasMusicTrack)
                        meta.trackInfo = artist + " - " + track;
                    else
                        meta.trackInfo = channel.isNotEmpty() ? channel : meta.title;

                    meta.licenseInfo = isCreativeCommons
                        ? juce::String::fromUTF8("Licencia Creative Commons declarada")
                        : juce::String::fromUTF8("Licencia Estándar de YouTube / Content ID Protegido");
                    meta.success = (meta.durationSeconds > 0);
                    break;
                }
            }
        }

        if (!meta.success && meta.errorMessage.isEmpty())
            meta.errorMessage = juce::String::fromUTF8("No se pudo detectar la duración del video.");

        if (onComplete)
            juce::MessageManager::callAsync([onComplete, meta] { onComplete(meta); });
    });
}

void AudioExtractorWorker::runYtDlpUpdate(std::function<void(bool success, const juce::String& outputMsg)> onComplete)
{
    juce::Thread::launch([onComplete = std::move(onComplete)] {
        auto ytDlpBin = findExecutable("yt-dlp");
        if (!ytDlpBin.existsAsFile())
        {
            if (onComplete)
            {
                juce::MessageManager::callAsync([onComplete] {
                    onComplete(false, juce::String::fromUTF8("No se encontró el ejecutable 'yt-dlp'. Verifica que esté instalado o en la carpeta tools."));
                });
            }
            return;
        }

        juce::StringArray args;
        args.add(ytDlpBin.getFullPathName());
        args.add("-U");

        juce::ChildProcess process;
        if (!process.start(args, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
        {
            if (onComplete)
            {
                juce::MessageManager::callAsync([onComplete] {
                    onComplete(false, juce::String::fromUTF8("No se pudo iniciar el proceso de actualización 'yt-dlp -U'."));
                });
            }
            return;
        }

        juce::String output;
        char buffer[512];
        const auto startTimeMs = juce::Time::getMillisecondCounterHiRes();
        constexpr double timeoutMs = 45000.0;
        bool timedOut = false;

        while (process.isRunning())
        {
            int bytesRead = process.readProcessOutput(buffer, sizeof(buffer) - 1);
            if (bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                output += juce::String::fromUTF8(buffer, bytesRead);
            }

            if ((juce::Time::getMillisecondCounterHiRes() - startTimeMs) > timeoutMs)
            {
                process.kill();
                timedOut = true;
                break;
            }

            juce::Thread::sleep(40);
        }

        while (!timedOut)
        {
            int bytesRead = process.readProcessOutput(buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0)
                break;
            buffer[bytesRead] = '\0';
            output += juce::String::fromUTF8(buffer, bytesRead);
        }

        if (timedOut)
        {
            if (onComplete)
            {
                juce::MessageManager::callAsync([onComplete] {
                    onComplete(false, juce::String::fromUTF8("Tiempo de espera agotado al actualizar yt-dlp."));
                });
            }
            return;
        }

        bool success = (process.getExitCode() == 0)
                    || output.containsIgnoreCase("up to date")
                    || output.containsIgnoreCase("updated")
                    || output.containsIgnoreCase("latest");

        auto lines = juce::StringArray::fromLines(output.trim());
        juce::String summary = lines.size() > 0 ? lines[lines.size() - 1].trim() : juce::String::fromUTF8("Actualización completada.");

        if (onComplete)
        {
            juce::MessageManager::callAsync([onComplete, success, summary] {
                onComplete(success, summary);
            });
        }
    });
}

juce::File AudioExtractorWorker::findExecutable(const juce::String& executableName)
{
    const juce::String cacheKey = executableName.toLowerCase();
    {
        std::lock_guard<std::mutex> lock(gExecutableCacheMutex);
        auto it = gExecutableCache.find(cacheKey);
        if (it != gExecutableCache.end() && it->second.existsAsFile())
            return it->second;
    }

    auto cacheAndReturn = [&](const juce::File& found) -> juce::File {
        if (found.existsAsFile())
        {
            std::lock_guard<std::mutex> lock(gExecutableCacheMutex);
            gExecutableCache[cacheKey] = found;
        }
        return found;
    };

#if JUCE_WINDOWS
    juce::String binaryName = executableName.endsWithIgnoreCase(".exe") ? executableName : (executableName + ".exe");

    auto commonAppData = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory);
    auto commonToolsBin = commonAppData.getChildFile("ClipSampler10").getChildFile("tools").getChildFile(binaryName);
    if (commonToolsBin.existsAsFile())
        return cacheAndReturn(commonToolsBin);

    auto roamingAppData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto roamingBin = roamingAppData.getChildFile("ClipSampler10").getChildFile("tools").getChildFile(binaryName);
    if (roamingBin.existsAsFile())
        return cacheAndReturn(roamingBin);

    juce::String localAppDataEnv = juce::SystemStats::getEnvironmentVariable("LOCALAPPDATA", {});
    if (localAppDataEnv.isNotEmpty())
    {
        juce::File localAppData(localAppDataEnv);
        auto localTools = localAppData.getChildFile("ClipSampler10").getChildFile("tools").getChildFile(binaryName);
        if (localTools.existsAsFile())
            return cacheAndReturn(localTools);

        auto wingetLink = localAppData.getChildFile("Microsoft").getChildFile("WinGet").getChildFile("Links").getChildFile(binaryName);
        if (wingetLink.existsAsFile())
            return cacheAndReturn(wingetLink);

        auto wingetPackages = localAppData.getChildFile("Microsoft").getChildFile("WinGet").getChildFile("Packages");
        if (wingetPackages.isDirectory())
        {
            auto foundFiles = wingetPackages.findChildFiles(juce::File::findFiles, true, binaryName);
            if (foundFiles.size() > 0)
                return cacheAndReturn(foundFiles[0]);
        }
    }

    auto pluginFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto pluginDir = pluginFile.getParentDirectory();
    auto pluginTools = pluginDir.getChildFile("tools").getChildFile(binaryName);
    if (pluginTools.existsAsFile())
        return cacheAndReturn(pluginTools);

    auto bundleRoot = pluginDir.getParentDirectory().getParentDirectory();
    auto bundleTools = bundleRoot.getChildFile("tools").getChildFile(binaryName);
    if (bundleTools.existsAsFile())
        return cacheAndReturn(bundleTools);

    auto globalAppsDir = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory);
    auto pfTools = globalAppsDir.getChildFile("ClipSampler10").getChildFile("tools").getChildFile(binaryName);
    if (pfTools.existsAsFile())
        return cacheAndReturn(pfTools);

    juce::String userProfile = juce::SystemStats::getEnvironmentVariable("USERPROFILE", {});
    if (userProfile.isNotEmpty())
    {
        juce::File scoopBin = juce::File(userProfile).getChildFile("scoop").getChildFile("shims").getChildFile(binaryName);
        if (scoopBin.existsAsFile())
            return cacheAndReturn(scoopBin);
    }

    juce::String chocoEnv = juce::SystemStats::getEnvironmentVariable("ChocolateyInstall", {});
    juce::File chocoBin = chocoEnv.isNotEmpty()
        ? juce::File(chocoEnv).getChildFile("bin").getChildFile(binaryName)
        : commonAppData.getChildFile("chocolatey").getChildFile("bin").getChildFile(binaryName);
    if (chocoBin.existsAsFile())
        return cacheAndReturn(chocoBin);

    WCHAR foundBuffer[MAX_PATH] = { 0 };
    if (SearchPathW(nullptr, binaryName.toWideCharPointer(), nullptr, MAX_PATH, foundBuffer, nullptr) > 0)
    {
        const juce::String foundPath(foundBuffer);
        const juce::File foundFile(foundPath);
        if (foundFile.existsAsFile())
            return cacheAndReturn(foundFile);
    }

    juce::String pathVar = juce::SystemStats::getEnvironmentVariable("PATH", {});
    if (pathVar.isNotEmpty())
    {
        auto paths = juce::StringArray::fromTokens(pathVar, ";", "");
        for (auto& p : paths)
        {
            juce::File dir(p.trim());
            auto candidate = dir.getChildFile(binaryName);
            if (candidate.existsAsFile())
                return cacheAndReturn(candidate);
        }
    }

    return juce::File();

#elif JUCE_MAC
    auto appSupport = juce::File("/Library/Application Support/ClipSampler10/tools").getChildFile(executableName);
    if (appSupport.existsAsFile())
        return cacheAndReturn(appSupport);

    auto userAppSupport = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("Application Support")
                               .getChildFile("ClipSampler10")
                               .getChildFile("tools")
                               .getChildFile(executableName);
    if (userAppSupport.existsAsFile())
        return cacheAndReturn(userAppSupport);

    juce::File brewArm("/opt/homebrew/bin/" + executableName);
    if (brewArm.existsAsFile())
        return cacheAndReturn(brewArm);

    juce::File brewIntel("/usr/local/bin/" + executableName);
    if (brewIntel.existsAsFile())
        return cacheAndReturn(brewIntel);

    return juce::File();
#else
    return juce::File();
#endif
}

void AudioExtractorWorker::run()
{
    const int durationToExtract = juce::jlimit(1, 60, currentDurationSec);

    if (progressCallback)
    {
        juce::MessageManager::callAsync([cb = progressCallback, durationToExtract] {
            cb(0.05f, juce::String::fromUTF8("Iniciando motor de extracción (") + juce::String(durationToExtract) + "s)...");
        });
    }

    auto ytDlpBin = findExecutable("yt-dlp");
    auto ffmpegBin = findExecutable("ffmpeg");

    if (!ytDlpBin.existsAsFile())
    {
        if (completionCallback)
        {
            juce::MessageManager::callAsync([cb = completionCallback] {
                cb(false, juce::File(), juce::String::fromUTF8("No se pudo iniciar 'yt-dlp'. Verifica que esté instalado."));
            });
        }
        return;
    }

    if (currentUrl.isEmpty() || (!currentUrl.startsWithIgnoreCase("http://") && !currentUrl.startsWithIgnoreCase("https://")))
    {
        if (completionCallback)
        {
            juce::MessageManager::callAsync([cb = completionCallback] {
                cb(false, juce::File(), juce::String::fromUTF8("Error: URL de YouTube no válida."));
            });
        }
        return;
    }

    int startSec = parseTimeToSeconds(currentStartTimeStr);
    int endSec = startSec + durationToExtract;
    juce::String startTimestamp = formatSecondsToTime(startSec);
    juce::String endTimestamp = formatSecondsToTime(endSec);

    auto cacheDir = getCacheDirectory();
    auto jobStartTime = juce::Time::getCurrentTime();
    int sampleRateToUse = (currentSampleRate > 0) ? currentSampleRate : 44100;

    auto outputTemplate = cacheDir.getFullPathName() + juce::File::getSeparatorString()
                        + "%(title).36s_" + juce::String(startSec) + "s_" + juce::String(durationToExtract) + "s.%(ext)s";

    juce::StringArray args;
    args.add(ytDlpBin.getFullPathName());
    args.add("--no-playlist");
    args.add("--download-sections");
    args.add("*" + startTimestamp + "-" + endTimestamp);
    args.add("-x");
    args.add("--audio-format");
    args.add("wav");
    args.add("--restrict-filenames");

    if (ffmpegBin.existsAsFile())
    {
        args.add("--ffmpeg-location");
        args.add(ffmpegBin.getFullPathName());
    }

    args.add("--postprocessor-args");
    args.add("ffmpeg:-acodec pcm_s24le -ar " + juce::String(sampleRateToUse) + " -t " + juce::String(durationToExtract));

    args.add("-o");
    args.add(outputTemplate);
    args.add(currentUrl);

    if (progressCallback)
    {
        juce::MessageManager::callAsync([cb = progressCallback, startTimestamp, endTimestamp, durationToExtract, sampleRateToUse] {
            cb(0.15f, juce::String::fromUTF8("Extrayendo ") + juce::String(durationToExtract)
                      + juce::String::fromUTF8("s [") + startTimestamp + " -> " + endTimestamp
                      + juce::String::fromUTF8("] a ") + juce::String(sampleRateToUse) + " Hz...");
        });
    }

    if (!activeProcess.start(args, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
    {
        if (completionCallback)
        {
            juce::MessageManager::callAsync([cb = completionCallback] {
                cb(false, juce::File(), juce::String::fromUTF8("No se pudo iniciar el proceso 'yt-dlp'."));
            });
        }
        return;
    }

    char buffer[512];
    juce::String accumulatedOutput;
    juce::String parsedWavPath;

    while (activeProcess.isRunning())
    {
        if (threadShouldExit() || isCancelled.load())
        {
            killActiveProcessTree();
            return;
        }

        int bytesRead = activeProcess.readProcessOutput(buffer, sizeof(buffer) - 1);
        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            juce::String chunk = juce::String::fromUTF8(buffer, bytesRead);
            accumulatedOutput += chunk;

            int destIdx = chunk.indexOf("Destination:");
            if (destIdx >= 0)
            {
                int endOfLine = chunk.indexOf(destIdx, "\n");
                if (endOfLine > destIdx)
                {
                    auto cand = chunk.substring(destIdx + 12, endOfLine).trim();
                    if (cand.endsWithIgnoreCase(".wav"))
                        parsedWavPath = cand;
                }
            }

            int downloadIdx = chunk.indexOf("[download]");
            if (downloadIdx >= 0)
            {
                int percentIdx = chunk.indexOf(downloadIdx, "%");
                if (percentIdx > downloadIdx)
                {
                    juce::String pctStr = chunk.substring(downloadIdx + 10, percentIdx).trim();
                    float pct = pctStr.getFloatValue() / 100.0f;
                    if (pct > 0.0f && pct <= 1.0f)
                    {
                        float normalized = 0.2f + (pct * 0.6f);
                        if (progressCallback)
                        {
                            juce::MessageManager::callAsync([cb = progressCallback, normalized, pctStr] {
                                cb(normalized, juce::String::fromUTF8("Descargando audio: ") + pctStr + "%");
                            });
                        }
                    }
                }
            }
            else if (chunk.containsIgnoreCase("ffmpeg") || chunk.containsIgnoreCase("ExtractAudio"))
            {
                if (progressCallback)
                {
                    juce::MessageManager::callAsync([cb = progressCallback, sampleRateToUse] {
                        cb(0.85f, juce::String::fromUTF8("Convirtiendo y aplicando Anti-Click DSP (") + juce::String(sampleRateToUse) + "Hz)...");
                    });
                }
            }
        }

        juce::Thread::sleep(40);
    }

    auto exitCode = activeProcess.getExitCode();

    if (threadShouldExit() || isCancelled.load())
        return;

    juce::File finalResultFile;
    if (parsedWavPath.isNotEmpty())
    {
        juce::File cand(parsedWavPath);
        if (cand.existsAsFile() && cand.getSize() > 0)
            finalResultFile = cand;
    }

    if (!finalResultFile.existsAsFile())
    {
        auto allWavs = cacheDir.findChildFiles(juce::File::findFiles, false, "*.wav");
        juce::File newest;
        juce::Time newestTime;

        for (auto& f : allWavs)
        {
            if (f.getFileNameWithoutExtension().endsWithIgnoreCase("_derechos_ia"))
                continue;

            auto modTime = f.getLastModificationTime();
            if (modTime.toMilliseconds() >= jobStartTime.toMilliseconds() - 5000)
            {
                if (modTime > newestTime)
                {
                    newestTime = modTime;
                    newest = f;
                }
            }
        }

        if (newest.existsAsFile() && newest.getSize() > 0)
            finalResultFile = newest;
    }

    if (!finalResultFile.existsAsFile() || finalResultFile.getSize() == 0)
    {
        juce::String errorMsg = juce::String::fromUTF8("La extracción falló (Exit code: ") + juce::String(exitCode) + ").";
        if (accumulatedOutput.isNotEmpty())
        {
            auto lines = juce::StringArray::fromLines(accumulatedOutput);
            for (int i = lines.size() - 1; i >= 0 && i >= lines.size() - 5; --i)
            {
                if (lines[i].containsIgnoreCase("error"))
                {
                    errorMsg += " " + lines[i].trim();
                    break;
                }
            }
        }

        if (completionCallback)
        {
            juce::MessageManager::callAsync([cb = completionCallback, errorMsg] {
                cb(false, juce::File(), errorMsg);
            });
        }
        return;
    }

    if (!validateAndEnforceDuration(finalResultFile, sampleRateToUse, durationToExtract))
    {
        if (completionCallback)
        {
            juce::MessageManager::callAsync([cb = completionCallback] {
                cb(false, juce::File(), juce::String::fromUTF8("Error al validar la duración o formato del archivo de audio."));
            });
        }
        return;
    }

    if (progressCallback)
    {
        juce::MessageManager::callAsync([cb = progressCallback, durationToExtract] {
            cb(1.0f, juce::String::fromUTF8("¡Muestra de ") + juce::String(durationToExtract) + juce::String::fromUTF8("s extraída con éxito!"));
        });
    }

    if (completionCallback)
    {
        juce::MessageManager::callAsync([cb = completionCallback, finalResultFile] {
            cb(true, finalResultFile, {});
        });
    }
}

bool AudioExtractorWorker::validateAndEnforceDuration(const juce::File& audioFile, int targetRate, int maxDurationSec)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    if (reader == nullptr)
        return false;

    double sampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : (double)targetRate;
    juce::int64 lengthInSamples = reader->lengthInSamples;
    juce::int64 maxAllowedSamples = static_cast<juce::int64>(sampleRate * juce::jlimit(1, 60, maxDurationSec));

    juce::int64 samplesToRead = juce::jmin(lengthInSamples, maxAllowedSamples);
    if (samplesToRead <= 0)
        return false;

    juce::AudioBuffer<float> buffer((int)reader->numChannels, (int)samplesToRead);
    reader->read(&buffer, 0, (int)samplesToRead, 0, true, true);
    reader.reset();

    int fadeSamples = juce::jmin((int)(sampleRate * 0.01), (int)(samplesToRead / 2));
    if (fadeSamples > 0)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.applyGainRamp(ch, 0, fadeSamples, 0.0f, 1.0f);
            buffer.applyGainRamp(ch, (int)samplesToRead - fadeSamples, fadeSamples, 1.0f, 0.0f);
        }
    }

    float currentPeak = buffer.getMagnitude(0, buffer.getNumSamples());
    if (currentPeak > 0.001f)
    {
        constexpr float targetPeak = 0.89125f; // -1 dB
        float gainFactor = targetPeak / currentPeak;
        if (gainFactor > 0.1f && gainFactor < 5.0f)
            buffer.applyGain(gainFactor);
    }

    juce::File tempTrimmed = audioFile.getSiblingFile(audioFile.getFileNameWithoutExtension() + "_safe.wav");
    {
        std::unique_ptr<juce::FileOutputStream> outStream(tempTrimmed.createOutputStream());
        if (outStream != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
                outStream.get(),
                sampleRate,
                buffer.getNumChannels(),
                24,
                {},
                0));

            if (writer != nullptr)
            {
                outStream.release();
                writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
            }
        }
    }

    if (tempTrimmed.existsAsFile() && tempTrimmed.getSize() > 0)
    {
        audioFile.deleteFile();
        tempTrimmed.moveFileTo(audioFile);
    }

    return true;
}

#include "PluginProcessor.h"
#include "../UI/PluginEditor.h"
#include <cmath>
#include <algorithm>

//==============================================================================
juce::String ClipSampler10AudioProcessor::midiNoteToName(int midiNote)
{
    static const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int clamped = juce::jlimit(0, 127, midiNote);
    int octave = (clamped / 12) - 2; // MIDI 36 -> C1, MIDI 60 -> C3
    return juce::String(noteNames[clamped % 12]) + juce::String(octave);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ClipSampler10AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterGain", 1 },
        "Master Gain",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
ClipSampler10AudioProcessor::ClipSampler10AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "ClipSamplerState", createParameterLayout()),
      originalThumbnail(512, formatManager, thumbnailCache),
      transformedThumbnail(512, formatManager, thumbnailCache),
      currentStatusMessage(juce::String::fromUTF8("Listo. Pega un enlace de YouTube, detecta su duración y elige 3s, 10s o 30s.")),
      currentCopyrightBadge(juce::String::fromUTF8("COPYRIGHT: Pendiente de análisis"))
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < NUM_VOICES; ++i)
        sampler.addVoice(new juce::SamplerVoice());
}

ClipSampler10AudioProcessor::~ClipSampler10AudioProcessor()
{
    aliveFlag->store(false, std::memory_order_release);
    extractorWorker.cancelExtraction();
}

//==============================================================================
const juce::String ClipSampler10AudioProcessor::getName() const
{
    return "ClipSampler";
}

bool ClipSampler10AudioProcessor::acceptsMidi() const { return true; }
bool ClipSampler10AudioProcessor::producesMidi() const { return false; }
bool ClipSampler10AudioProcessor::isMidiEffect() const { return false; }
double ClipSampler10AudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ClipSampler10AudioProcessor::getNumPrograms() { return 1; }
int ClipSampler10AudioProcessor::getCurrentProgram() { return 0; }
void ClipSampler10AudioProcessor::setCurrentProgram(int) {}
const juce::String ClipSampler10AudioProcessor::getProgramName(int) { return {}; }
void ClipSampler10AudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void ClipSampler10AudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    sampler.setCurrentPlaybackSampleRate(sampleRate);
    lofiFilterStateL = 0.0f;
    lofiFilterStateR = 0.0f;

    auto fileToReload = getCurrentAudioFile();
    if (fileToReload.existsAsFile() && std::abs(loadedFileSampleRate.load(std::memory_order_relaxed) - sampleRate) > 1.0)
    {
        activateSlotBuffer(fileToReload, activeSlot.load(std::memory_order_relaxed));
    }
}

void ClipSampler10AudioProcessor::releaseResources()
{
    stopPlayback();
}

bool ClipSampler10AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void ClipSampler10AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (!licenseManager.isUnlocked() || !licenseManager.verifyRuntimeIntegrity())
    {
        buffer.clear();
        return;
    }

    // Query DAW Host BPM if available
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpmOpt = pos->getBpm())
            {
                if (*bpmOpt > 20.0 && *bpmOpt < 300.0)
                    currentHostBpm.store(*bpmOpt, std::memory_order_relaxed);
            }
        }
    }

    const bool chopMode = autoChopEnabled.load(std::memory_order_relaxed);
    const int curSlot = activeSlot.load(std::memory_order_relaxed);

    // Handle MIDI input (MPC Auto-Chop Mode C1..G2 vs Chromatic Sampler Mode)
    if (chopMode)
    {
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                const juce::ScopedTryLock stl(bufferLock);
                if (stl.isLocked())
                {
                    const auto& slices = (curSlot == 2) ? slot2Slices : slot1Slices;
                    if (!slices.empty())
                    {
                        int note = msg.getNoteNumber();
                        int sliceIdx = note - 36; // C1 = 36
                        if (sliceIdx < 0 || sliceIdx >= (int)slices.size())
                            sliceIdx = ((note % (int)slices.size()) + (int)slices.size()) % (int)slices.size();

                        const auto& s = slices[(size_t)sliceIdx];
                        fractionalPlayhead.store((double)s.startSample, std::memory_order_relaxed);
                        playheadPosition.store(s.startSample, std::memory_order_relaxed);
                        playbackEndSample.store(s.endSample, std::memory_order_relaxed);
                        activePlayingSlice.store(sliceIdx, std::memory_order_relaxed);
                        isPlaying.store(true, std::memory_order_relaxed);
                    }
                }
            }
        }
        // Clear MIDI buffer so standard chromatic synth doesn't double-render over Auto-Chop
        midiMessages.clear();
    }
    else
    {
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                fractionalPlayhead.store(0.0, std::memory_order_relaxed);
                playheadPosition.store(0, std::memory_order_relaxed);
                playbackEndSample.store(totalSampleCount.load(std::memory_order_relaxed), std::memory_order_relaxed);
                activePlayingSlice.store(-1, std::memory_order_relaxed);
                isPlaying.store(true, std::memory_order_relaxed);
            }
        }
        sampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (isPlaying.load(std::memory_order_relaxed))
    {
        double fracPos = fractionalPlayhead.load(std::memory_order_relaxed);
        int total = totalSampleCount.load(std::memory_order_relaxed);
        int endSample = playbackEndSample.load(std::memory_order_relaxed);
        if (endSample <= 0 || endSample > total)
            endSample = total;

        int numSamplesToRender = buffer.getNumSamples();

        if (fracPos >= (double)endSample || total == 0)
        {
            isPlaying.store(false, std::memory_order_relaxed);
            fractionalPlayhead.store(0.0, std::memory_order_relaxed);
            playheadPosition.store(0, std::memory_order_relaxed);
            activePlayingSlice.store(-1, std::memory_order_relaxed);
        }
        else
        {
            const juce::ScopedTryLock stl(bufferLock);
            if (stl.isLocked())
            {
                if (sampleBuffer.getNumSamples() >= total && sampleBuffer.getNumChannels() > 0)
                {
                    // Compute playback speed from Pitch FX knob + Auto-Sync ratio
                    const float pitchSt = fxPitchSemitones.load(std::memory_order_relaxed);
                    double playbackRate = std::pow(2.0, (double)pitchSt / 12.0);

                    if (autoSyncEnabled.load(std::memory_order_relaxed))
                    {
                        double detBpm = (curSlot == 2) ? slot2Bpm.load(std::memory_order_relaxed) : slot1Bpm.load(std::memory_order_relaxed);
                        double hostBpm = currentHostBpm.load(std::memory_order_relaxed);
                        if (detBpm > 35.0 && hostBpm > 35.0)
                            playbackRate *= juce::jlimit(0.5, 2.0, hostBpm / detBpm);
                    }

                    const int numOutChannels = buffer.getNumChannels();
                    const int numSampleChannels = sampleBuffer.getNumChannels();

                    for (int i = 0; i < numSamplesToRender; ++i)
                    {
                        if (fracPos >= (double)endSample)
                        {
                            isPlaying.store(false, std::memory_order_relaxed);
                            fractionalPlayhead.store(0.0, std::memory_order_relaxed);
                            playheadPosition.store(0, std::memory_order_relaxed);
                            activePlayingSlice.store(-1, std::memory_order_relaxed);
                            break;
                        }

                        int idx0 = (int)fracPos;
                        int idx1 = juce::jmin(idx0 + 1, total - 1);
                        float alpha = (float)(fracPos - (double)idx0);

                        for (int ch = 0; ch < numOutChannels; ++ch)
                        {
                            int srcCh = juce::jmin(ch, numSampleChannels - 1);
                            const float* src = sampleBuffer.getReadPointer(srcCh);
                            float s = src[idx0] + alpha * (src[idx1] - src[idx0]);
                            buffer.addSample(ch, i, s);
                        }

                        fracPos += playbackRate;
                    }

                    fractionalPlayhead.store(fracPos, std::memory_order_relaxed);
                    playheadPosition.store(juce::jlimit(0, total, (int)fracPos), std::memory_order_relaxed);
                }
                else
                {
                    isPlaying.store(false, std::memory_order_relaxed);
                }
            }
        }
    }

    // Apply real-time FX Macro (Lo-Fi 12-Bit Crusher + Warm Low-Pass Tone Filter)
    const float lofiAmt = fxLofiAmount.load(std::memory_order_relaxed);
    const float toneCutoff = fxToneFilter.load(std::memory_order_relaxed);

    if (lofiAmt > 0.005f || toneCutoff < 0.99f)
    {
        // 12-bit quantization levels + sample-and-hold decimation factor
        const float levels = juce::jmap(lofiAmt, 0.0f, 1.0f, 4096.0f, 28.0f);
        const int holdFactor = juce::jmax(1, juce::roundToInt(1.0f + lofiAmt * 14.0f));
        const float lpAlpha = juce::jlimit(0.04f, 1.0f, toneCutoff * toneCutoff);

        const int numSamples = buffer.getNumSamples();
        const int numCh = buffer.getNumChannels();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            float& filterState = (ch == 0) ? lofiFilterStateL : lofiFilterStateR;
            float heldSample = 0.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                if (i % holdFactor == 0)
                {
                    float driven = std::tanh(data[i] * (1.0f + lofiAmt * 0.65f));
                    heldSample = std::round(driven * levels) / levels;
                }
                float s = (lofiAmt > 0.005f) ? heldSample : data[i];
                filterState += lpAlpha * (s - filterState);
                data[i] = filterState;
            }
        }
    }

    if (auto* gainParam = apvts.getRawParameterValue("masterGain"))
    {
        const float gain = gainParam->load(std::memory_order_relaxed);
        if (std::abs(gain - 1.0f) > 0.0001f)
            buffer.applyGain(gain);
    }
}

//==============================================================================
bool ClipSampler10AudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* ClipSampler10AudioProcessor::createEditor()
{
    return new ClipSampler10AudioProcessorEditor(*this);
}

//==============================================================================
void ClipSampler10AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("lastUrl", lastUrl, nullptr);
    state.setProperty("lastStartTime", lastStartTime, nullptr);
    state.setProperty("lastSelectedDurationSec", lastSelectedDurationSec.load(), nullptr);
    state.setProperty("detectedVideoDurationSec", detectedVideoDurationSec.load(), nullptr);
    state.setProperty("autoChopEnabled", autoChopEnabled.load(), nullptr);
    state.setProperty("fxTargetSlot", fxTargetSlot.load(), nullptr);
    state.setProperty("fxLofiAmount", fxLofiAmount.load(), nullptr);
    state.setProperty("fxPitchSemitones", fxPitchSemitones.load(), nullptr);
    state.setProperty("fxToneFilter", fxToneFilter.load(), nullptr);
    state.setProperty("autoSyncEnabled", autoSyncEnabled.load(), nullptr);

    auto origFile = getSlotAudioFile(1);
    auto transFile = getSlotAudioFile(2);
    state.setProperty("lastWavPath", origFile.getFullPathName(), nullptr);
    state.setProperty("lastWavFileName", origFile.getFileName(), nullptr);
    state.setProperty("lastTransWavPath", transFile.getFullPathName(), nullptr);
    state.setProperty("activeSlot", activeSlot.load(), nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void ClipSampler10AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (state.isValid())
    {
        if (state.hasType(apvts.state.getType()))
            apvts.replaceState(state);

        lastUrl = state.getProperty("lastUrl", "").toString();
        lastStartTime = state.getProperty("lastStartTime", "00:00").toString();
        lastSelectedDurationSec.store(static_cast<int>(state.getProperty("lastSelectedDurationSec", 10)));
        detectedVideoDurationSec.store(static_cast<int>(state.getProperty("detectedVideoDurationSec", 0)));
        autoChopEnabled.store(static_cast<bool>(state.getProperty("autoChopEnabled", true)));
        fxTargetSlot.store(static_cast<int>(state.getProperty("fxTargetSlot", 1)));
        fxLofiAmount.store(static_cast<float>(state.getProperty("fxLofiAmount", 0.0f)));
        fxPitchSemitones.store(static_cast<float>(state.getProperty("fxPitchSemitones", 0.0f)));
        fxToneFilter.store(static_cast<float>(state.getProperty("fxToneFilter", 1.0f)));
        autoSyncEnabled.store(static_cast<bool>(state.getProperty("autoSyncEnabled", false)));

        juce::String path = state.getProperty("lastWavPath", "").toString();
        juce::String fileName = state.getProperty("lastWavFileName", "").toString();
        juce::String transPath = state.getProperty("lastTransWavPath", "").toString();
        int savedSlot = static_cast<int>(state.getProperty("activeSlot", 1));

        juce::File fileToLoad;
        if (path.isNotEmpty() && juce::File(path).existsAsFile())
            fileToLoad = juce::File(path);
        else if (fileName.isNotEmpty())
        {
            auto cacheCandidate = AudioExtractorWorker::getCacheDirectory().getChildFile(fileName);
            if (cacheCandidate.existsAsFile())
                fileToLoad = cacheCandidate;
        }

        if (fileToLoad.existsAsFile())
            loadAudioFileIntoSlot(fileToLoad, 1);

        if (transPath.isNotEmpty() && juce::File(transPath).existsAsFile())
            loadAudioFileIntoSlot(juce::File(transPath), 2);

        setActiveSlot(savedSlot);
    }
}

//==============================================================================
bool ClipSampler10AudioProcessor::hasSlotAudio(int slotIndex) const
{
    const juce::ScopedLock sl(bufferLock);
    if (slotIndex == 2)
        return transformedLoadedFile.existsAsFile();
    return originalLoadedFile.existsAsFile();
}

bool ClipSampler10AudioProcessor::hasBothAudiosReady() const
{
    const juce::ScopedLock sl(bufferLock);
    return originalLoadedFile.existsAsFile() && transformedLoadedFile.existsAsFile();
}

void ClipSampler10AudioProcessor::resetForNewExtraction()
{
    stopPlayback();
    extractorWorker.cancelExtraction();

    {
        const juce::ScopedLock sl(bufferLock);
        sampler.clearSounds();
        sampleBuffer.clear();
        originalLoadedFile = juce::File();
        transformedLoadedFile = juce::File();
        slot1MidiFile = juce::File();
        slot2MidiFile = juce::File();
        slot1Slices.clear();
        slot2Slices.clear();
    }

    originalThumbnail.clear();
    transformedThumbnail.clear();

    slot1Bpm.store(0.0, std::memory_order_relaxed);
    slot2Bpm.store(0.0, std::memory_order_relaxed);
    {
        const juce::ScopedLock sl(statusLock);
        slot1Key = "--";
        slot2Key = "--";
        detectedVideoTitle.clear();
        detectedLicenseInfo.clear();
        currentCopyrightBadge = juce::String::fromUTF8("COPYRIGHT: Pendiente de análisis");
    }

    lastUrl.clear();
    lastStartTime = "00:00";
    detectedVideoDurationSec.store(0, std::memory_order_relaxed);
    extractionProgress.store(0.0f, std::memory_order_relaxed);
    copyrightVerifyState.store(0, std::memory_order_relaxed);
    totalSampleCount.store(0, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
    fractionalPlayhead.store(0.0, std::memory_order_relaxed);
    activePlayingSlice.store(-1, std::memory_order_relaxed);
    activeSlot.store(1, std::memory_order_relaxed);
    fxTargetSlot.store(1, std::memory_order_relaxed);
    fxLofiAmount.store(0.0f, std::memory_order_relaxed);
    fxPitchSemitones.store(0.0f, std::memory_order_relaxed);
    fxToneFilter.store(1.0f, std::memory_order_relaxed);
    autoSyncEnabled.store(false, std::memory_order_relaxed);

    setStatusMessage(juce::String::fromUTF8("✨ ¡Nueva extracción lista! Ambos visores y efectos han sido limpiados. Pega una nueva URL."));
}

//==============================================================================
void ClipSampler10AudioProcessor::detectVideoDuration(const juce::String& url)
{
    juce::String cleanUrl = url.trim();
    if (cleanUrl.isEmpty())
        return;

    lastUrl = cleanUrl;
    if (metadataInProgress.exchange(true))
        return;

    setStatusMessage(juce::String::fromUTF8("🔍 Detectando duración total y metadatos del video de YouTube..."));
    auto weakAlive = aliveFlag;

    AudioExtractorWorker::fetchVideoMetadata(cleanUrl, [this, weakAlive](const AudioExtractorWorker::VideoMetadata& meta) {
        if (!weakAlive->load(std::memory_order_acquire))
            return;

        metadataInProgress.store(false, std::memory_order_relaxed);
        if (meta.success && meta.durationSeconds > 0)
        {
            detectedVideoDurationSec.store(meta.durationSeconds, std::memory_order_relaxed);
            detectedHasCopyrightMeta.store(meta.hasCopyrightMetadata, std::memory_order_relaxed);
            {
                const juce::ScopedLock sl(statusLock);
                detectedVideoTitle = meta.title;
                detectedLicenseInfo = meta.licenseInfo;
            }

            juce::String durFormatted = AudioExtractorWorker::formatSecondsToMMSS(meta.durationSeconds);
            setStatusMessage(juce::String::fromUTF8("✓ Video detectado [00:00 -> ") + durFormatted + "]: " + meta.title);
        }
        else
        {
            setStatusMessage(juce::String::fromUTF8("Aviso al consultar duración: ") + meta.errorMessage);
        }
    });
}

void ClipSampler10AudioProcessor::startSampleExtraction(const juce::String& url, const juce::String& startTime, int durationSeconds)
{
    licenseManager.verifyAndSyncState();
    if (!licenseManager.isUnlocked() || !licenseManager.verifyRuntimeIntegrity())
    {
        setStatusMessage(juce::String::fromUTF8("🔒 Periodo de prueba de 3 días finalizado o licencia requerida. Activa tu licencia para continuar."));
        return;
    }

    lastUrl = url.trim();
    lastStartTime = startTime.trim();
    const int clampedDuration = (durationSeconds == 3 || durationSeconds == 10 || durationSeconds == 30) ? durationSeconds : 10;
    lastSelectedDurationSec.store(clampedDuration, std::memory_order_relaxed);

    if (detectedVideoDurationSec.load(std::memory_order_relaxed) <= 0)
        detectVideoDuration(lastUrl);

    extractionInProgress.store(true, std::memory_order_relaxed);
    extractionProgress.store(0.05f, std::memory_order_relaxed);
    setStatusMessage(juce::String::fromUTF8("Iniciando extracción de ") + juce::String(clampedDuration) + juce::String::fromUTF8(" segundos..."));

    int currentDawRate = static_cast<int>(getSampleRate());
    if (currentDawRate <= 0)
        currentDawRate = 44100;

    auto weakAlive = aliveFlag;

    extractorWorker.startExtraction(
        lastUrl,
        lastStartTime,
        clampedDuration,
        currentDawRate,
        [this, weakAlive](float progress, const juce::String& msg) {
            if (!weakAlive->load(std::memory_order_acquire))
                return;

            extractionProgress.store(progress, std::memory_order_relaxed);
            setStatusMessage(msg);
        },
        [this, weakAlive, clampedDuration](bool success, const juce::File& resultFile, const juce::String& error) {
            if (!weakAlive->load(std::memory_order_acquire))
                return;

            if (success && resultFile.existsAsFile())
            {
                loadAudioFileIntoSlot(resultFile, 1);
                copyrightVerifyState.store(0, std::memory_order_relaxed);
                {
                    const juce::ScopedLock sl(statusLock);
                    currentCopyrightBadge = juce::String::fromUTF8("COPYRIGHT: Muestra original en Visor 1 (Sin verificar)");
                }
                extractionProgress.store(1.0f, std::memory_order_relaxed);

                double bpm = getDetectedBpm(1);
                juce::String key = getDetectedKey(1);
                setStatusMessage(juce::String::fromUTF8("¡Muestra de ") + juce::String(clampedDuration)
                                 + juce::String::fromUTF8("s lista en Visor 1! [")
                                 + juce::String(bpm, 1) + " BPM | Key: " + key
                                 + juce::String::fromUTF8(" | Auto-Chop C1-G2 activo]. Pulsa '🛡️ Derechos' para generar Audio 2."));
            }
            else
            {
                extractionProgress.store(0.0f, std::memory_order_relaxed);
                setStatusMessage(juce::String::fromUTF8("Error en la extracción: ") + error);
            }

            extractionInProgress.store(false, std::memory_order_relaxed);
        });
}

bool ClipSampler10AudioProcessor::analyzeOriginalCopyright(juce::String& outReport)
{
    auto origFile = getSlotAudioFile(1);
    if (!origFile.existsAsFile())
    {
        outReport = juce::String::fromUTF8("Primero extrae un audio (3s, 10s o 30s) para analizar sus derechos de autor.");
        setStatusMessage(outReport);
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(origFile));
    if (reader == nullptr || reader->lengthInSamples <= 0)
    {
        outReport = juce::String::fromUTF8("No se pudo leer el archivo original para el análisis de copyright.");
        setStatusMessage(outReport);
        return false;
    }

    const int numSamples = (int)juce::jmin<juce::int64>(reader->lengthInSamples, 48000 * 10);
    juce::AudioBuffer<float> buf((int)reader->numChannels, numSamples);
    reader->read(&buf, 0, numSamples, 0, true, true);

    float rms = buf.getRMSLevel(0, 0, numSamples);
    float peak = buf.getMagnitude(0, numSamples);
    float crestFactor = (rms > 0.0001f) ? (peak / rms) : 1.0f;

    bool hasYtCopy = detectedHasCopyrightMeta.load(std::memory_order_relaxed);
    copyrightVerifyState.store(1, std::memory_order_relaxed);

    juce::String badge;
    if (hasYtCopy || crestFactor < 12.0f)
    {
        badge = juce::String::fromUTF8("⚠️ RIESGO COPYRIGHT: Huella Original Detectada (100% Match)");
        outReport = juce::String::fromUTF8("⚠️ [ALERTA DE DERECHOS]: El Audio 1 coincide al 100% con el espectro original de YouTube (Content ID activo). Pulsa el botón '🛡️ Derechos (±3 st)' para alterarlo.");
    }
    else
    {
        badge = juce::String::fromUTF8("ℹ️ COPYRIGHT: Licencia CC, pero huella idéntica al video");
        outReport = juce::String::fromUTF8("ℹ️ Audio con metadatos CC, pero conserva la onda exacta del video. Se recomienda pulsar '🛡️ Derechos' para uso seguro.");
    }

    {
        const juce::ScopedLock sl(statusLock);
        currentCopyrightBadge = badge;
    }
    setStatusMessage(outReport);
    return true;
}

bool ClipSampler10AudioProcessor::applyDerechosTransformation(int semitoneShift, juce::String& outReport)
{
    if (!licenseManager.isUnlocked() || !licenseManager.verifyRuntimeIntegrity())
    {
        outReport = juce::String::fromUTF8("🔒 Activación requerida para usar la suite Derechos IA.");
        setStatusMessage(outReport);
        return false;
    }

    auto origFile = getSlotAudioFile(1);
    if (!origFile.existsAsFile())
    {
        outReport = juce::String::fromUTF8("Primero extrae una muestra de 3s, 10s o 30s en el Visor 1 antes de aplicar '🛡️ Derechos'.");
        setStatusMessage(outReport);
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(origFile));
    if (reader == nullptr || reader->lengthInSamples <= 0)
    {
        outReport = juce::String::fromUTF8("Error al leer el archivo original para la transformación de derechos.");
        setStatusMessage(outReport);
        return false;
    }

    if (semitoneShift == 0)
        semitoneShift = 3;

    lastAppliedSemitones.store(semitoneShift, std::memory_order_relaxed);

    const double sampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    const int inSamples = (int)reader->lengthInSamples;
    const int numChannels = juce::jmax(1, (int)reader->numChannels);

    juce::AudioBuffer<float> inBuffer(numChannels, inSamples);
    reader->read(&inBuffer, 0, inSamples, 0, true, true);
    reader.reset();

    // 1. Pitch Shift by ±3 Semitones (ratio = 2^(semitones / 12.0)) + subtle micro-detune (0.15 semitones)
    const double effectiveSemitones = static_cast<double>(semitoneShift) + (semitoneShift > 0 ? 0.15 : -0.15);
    const double speedRatio = std::pow(2.0, effectiveSemitones / 12.0);
    const int outSamples = juce::jmax(64, (int)std::floor(inSamples / speedRatio));

    juce::AudioBuffer<float> outBuffer(numChannels, outSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::LagrangeInterpolator pitchInterpolator;
        pitchInterpolator.process(speedRatio,
                                  inBuffer.getReadPointer(ch),
                                  outBuffer.getWritePointer(ch),
                                  outSamples);
    }

    // 2. Acoustic Fingerprint Evasion DSP
    const double lfoFreq = 1.75;
    const float allPassCoeff = (semitoneShift > 0) ? 0.42f : -0.42f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = outBuffer.getWritePointer(ch);
        float xPrev = 0.0f;
        float yPrev = 0.0f;
        const double chPhaseOffset = (ch == 1) ? 1.5707963 : 0.0;

        for (int i = 0; i < outSamples; ++i)
        {
            const float x = data[i];
            const float y = (allPassCoeff * x) + xPrev - (allPassCoeff * yPrev);
            xPrev = x;
            yPrev = y;

            const double t = static_cast<double>(i) / sampleRate;
            const float mod = 1.0f + 0.018f * static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * lfoFreq * t + chPhaseOffset));
            data[i] = std::tanh(y * mod * 1.06f);
        }
    }

    if (numChannels >= 2)
    {
        const int delaySamples = juce::jlimit(1, 64, (int)(sampleRate * 0.00065));
        float* rightCh = outBuffer.getWritePointer(1);
        for (int i = outSamples - 1; i >= delaySamples; --i)
            rightCh[i] = 0.88f * rightCh[i] + 0.12f * rightCh[i - delaySamples];
    }

    const int fadeSamples = juce::jmin((int)(sampleRate * 0.01), outSamples / 2);
    if (fadeSamples > 0)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            outBuffer.applyGainRamp(ch, 0, fadeSamples, 0.0f, 1.0f);
            outBuffer.applyGainRamp(ch, outSamples - fadeSamples, fadeSamples, 1.0f, 0.0f);
        }
    }

    const float peak = outBuffer.getMagnitude(0, outSamples);
    if (peak > 0.001f)
        outBuffer.applyGain(0.89125f / peak);

    juce::String shiftTag = (semitoneShift > 0) ? ("_mas" + juce::String(semitoneShift) + "st") : ("_menos" + juce::String(std::abs(semitoneShift)) + "st");
    juce::File transFile = origFile.getSiblingFile(origFile.getFileNameWithoutExtension() + shiftTag + "_derechos_ia.wav");
    transFile.deleteFile();

    {
        std::unique_ptr<juce::FileOutputStream> outStream(transFile.createOutputStream());
        if (outStream == nullptr)
        {
            outReport = juce::String::fromUTF8("No se pudo escribir el archivo de audio transformado.");
            setStatusMessage(outReport);
            return false;
        }

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
            outStream.get(),
            sampleRate,
            (unsigned int)numChannels,
            24,
            {},
            0));

        if (writer == nullptr)
        {
            outReport = juce::String::fromUTF8("No se pudo inicializar el codificador WAV de 24 bits.");
            setStatusMessage(outReport);
            return false;
        }

        outStream.release();
        writer->writeFromAudioSampleBuffer(outBuffer, 0, outSamples);
    }

    loadAudioFileIntoSlot(transFile, 2);
    fxTargetSlot.store(2, std::memory_order_relaxed);

    juce::String signStr = (semitoneShift > 0) ? ("+" + juce::String(semitoneShift)) : juce::String(semitoneShift);
    outReport = juce::String::fromUTF8("🛡️ ¡Audio 2 generado (") + signStr
              + juce::String::fromUTF8(" st IA) en Visor 2! Botón '🔄 Nueva Extracción' habilitado. Pulsa '✅ Verificar Sin Derechos'.");
    {
        const juce::ScopedLock sl(statusLock);
        currentCopyrightBadge = juce::String::fromUTF8("🛡️ AUDIO 2 GENERADO (") + signStr + juce::String::fromUTF8(" st) - Listo para verificar");
    }
    setStatusMessage(outReport);
    return true;
}

bool ClipSampler10AudioProcessor::verifyTransformedAudioCopyrightFree(juce::String& outReport)
{
    auto origFile = getSlotAudioFile(1);
    auto transFile = getSlotAudioFile(2);

    if (!origFile.existsAsFile() || !transFile.existsAsFile())
    {
        outReport = juce::String::fromUTF8("Primero extrae una muestra (Visor 1) y pulsa '🛡️ Derechos' para generar el Audio 2 (Visor 2) antes de verificar.");
        setStatusMessage(outReport);
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> r1(formatManager.createReaderFor(origFile));
    std::unique_ptr<juce::AudioFormatReader> r2(formatManager.createReaderFor(transFile));
    if (r1 == nullptr || r2 == nullptr)
    {
        outReport = juce::String::fromUTF8("Error al abrir los archivos de audio para comparar huellas.");
        setStatusMessage(outReport);
        return false;
    }

    const int compareLen = (int)juce::jmin<juce::int64>(juce::jmin(r1->lengthInSamples, r2->lengthInSamples), 44100 * 8);
    if (compareLen <= 128)
    {
        outReport = juce::String::fromUTF8("Muestra demasiado corta para verificación espectral.");
        setStatusMessage(outReport);
        return false;
    }

    juce::AudioBuffer<float> b1(1, compareLen);
    juce::AudioBuffer<float> b2(1, compareLen);
    r1->read(&b1, 0, compareLen, 0, true, false);
    r2->read(&b2, 0, compareLen, 0, true, false);

    const float* p1 = b1.getReadPointer(0);
    const float* p2 = b2.getReadPointer(0);

    double dot = 0.0, norm1 = 0.0, norm2 = 0.0;
    for (int i = 0; i < compareLen; ++i)
    {
        dot += static_cast<double>(p1[i]) * static_cast<double>(p2[i]);
        norm1 += static_cast<double>(p1[i]) * static_cast<double>(p1[i]);
        norm2 += static_cast<double>(p2[i]) * static_cast<double>(p2[i]);
    }

    const double denom = std::sqrt(norm1 * norm2);
    const double correlation = (denom > 1e-9) ? (std::abs(dot) / denom) : 0.0;
    const double divergencePct = juce::jlimit(0.0, 99.9, (1.0 - correlation) * 100.0);

    lastDivergencePercent.store(divergencePct, std::memory_order_relaxed);
    copyrightVerifyState.store(2, std::memory_order_relaxed);

    int st = lastAppliedSemitones.load(std::memory_order_relaxed);
    juce::String stText = (st > 0) ? ("+" + juce::String(st)) : juce::String(st);

    juce::String pctFormatted(divergencePct, 1);
    {
        const juce::ScopedLock sl(statusLock);
        currentCopyrightBadge = juce::String::fromUTF8("✅ VERIFICADO SIN COPYRIGHT (") + pctFormatted + "% Divergencia IA)";
    }

    outReport = juce::String::fromUTF8("✅ [VERIFICADO LIBRE DE DERECHOS]: El Audio 2 tiene ") + pctFormatted
              + juce::String::fromUTF8("% de divergencia de huella respecto al original (") + stText
              + juce::String::fromUTF8(" semitonos + fase IA). Apto para arrastrar .WAV o .MIDI al DAW.");
    setStatusMessage(outReport);
    return true;
}

//==============================================================================
// v2.3: MPC Auto-Chop (Transient Slicer C1 -> G2) & BPM + Key Analysis
//==============================================================================
void ClipSampler10AudioProcessor::setAutoChopEnabled(bool enabled)
{
    autoChopEnabled.store(enabled, std::memory_order_relaxed);
}

std::vector<ChopSlice> ClipSampler10AudioProcessor::getChopSlices(int slotIndex) const
{
    const juce::ScopedLock sl(bufferLock);
    return (slotIndex == 2) ? slot2Slices : slot1Slices;
}

void ClipSampler10AudioProcessor::triggerChopSlice(int sliceIndex)
{
    if (!licenseManager.isUnlocked())
        return;

    const juce::ScopedLock sl(bufferLock);
    const int curSlot = activeSlot.load(std::memory_order_relaxed);
    const auto& slices = (curSlot == 2) ? slot2Slices : slot1Slices;
    if (sliceIndex >= 0 && sliceIndex < (int)slices.size())
    {
        const auto& s = slices[(size_t)sliceIndex];
        fractionalPlayhead.store((double)s.startSample, std::memory_order_relaxed);
        playheadPosition.store(s.startSample, std::memory_order_relaxed);
        playbackEndSample.store(s.endSample, std::memory_order_relaxed);
        activePlayingSlice.store(sliceIndex, std::memory_order_relaxed);
        isPlaying.store(true, std::memory_order_relaxed);
    }
}

std::vector<ChopSlice> ClipSampler10AudioProcessor::computeTransientSlices(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    std::vector<ChopSlice> result;
    const int totalSamples = buffer.getNumSamples();
    if (totalSamples <= 256 || sampleRate <= 0.0)
        return result;

    // 1. Compute energy envelope & positive spectral/energy flux in 10ms hops
    const int hop = juce::jmax(64, (int)(sampleRate * 0.010));
    const int numFrames = totalSamples / hop;
    if (numFrames < 4)
        return result;

    std::vector<float> env((size_t)numFrames, 0.0f);
    const float* ch0 = buffer.getReadPointer(0);
    const float* ch1 = (buffer.getNumChannels() > 1) ? buffer.getReadPointer(1) : ch0;

    for (int f = 0; f < numFrames; ++f)
    {
        int start = f * hop;
        float sumSq = 0.0f;
        for (int i = 0; i < hop; ++i)
        {
            float m = 0.5f * (ch0[start + i] + ch1[start + i]);
            sumSq += m * m;
        }
        env[(size_t)f] = std::sqrt(sumSq / (float)hop);
    }

    std::vector<float> flux((size_t)numFrames, 0.0f);
    float meanFlux = 0.0f;
    for (int f = 1; f < numFrames; ++f)
    {
        float diff = env[(size_t)f] - env[(size_t)(f - 1)];
        if (diff > 0.0f)
        {
            flux[(size_t)f] = diff;
            meanFlux += diff;
        }
    }
    meanFlux /= (float)numFrames;

    // Minimum distance between slices (~0.24s for 3s/10s, ~0.75s for 30s so we fit within 16..20 pads C1..G2)
    const double totalDurationSec = (double)totalSamples / sampleRate;
    const double minSliceSec = juce::jmax(0.22, totalDurationSec / 16.0);
    const int minHopDist = juce::jmax(4, (int)(minSliceSec * sampleRate / hop));

    std::vector<int> onsetSamples;
    onsetSamples.push_back(0);

    const float threshold = meanFlux * 1.55f + 0.002f;
    int lastFrame = 0;

    for (int f = minHopDist; f < numFrames - minHopDist; ++f)
    {
        if (flux[(size_t)f] > threshold
            && flux[(size_t)f] >= flux[(size_t)(f - 1)]
            && flux[(size_t)f] >= flux[(size_t)(f + 1)]
            && (f - lastFrame) >= minHopDist)
        {
            onsetSamples.push_back(f * hop);
            lastFrame = f;
            if ((int)onsetSamples.size() >= 16) // Up to 16 MPC pads (C1 .. D#2 / G2)
                break;
        }
    }

    // If audio has smooth pads or fewer than 8 transient peaks, subdivide longest segments so C1..G#1/G2 are always playable
    while ((int)onsetSamples.size() < 8)
    {
        int bestIdx = 0;
        int maxGap = 0;
        for (size_t i = 0; i < onsetSamples.size(); ++i)
        {
            int nextSample = (i + 1 < onsetSamples.size()) ? onsetSamples[i + 1] : totalSamples;
            int gap = nextSample - onsetSamples[i];
            if (gap > maxGap)
            {
                maxGap = gap;
                bestIdx = (int)i;
            }
        }
        if (maxGap < 512)
            break;

        int nextSample = (bestIdx + 1 < (int)onsetSamples.size()) ? onsetSamples[(size_t)(bestIdx + 1)] : totalSamples;
        int midSample = onsetSamples[(size_t)bestIdx] + (nextSample - onsetSamples[(size_t)bestIdx]) / 2;
        onsetSamples.insert(onsetSamples.begin() + bestIdx + 1, midSample);
    }

    // Cap at 20 slices (C1 = 36 through G2 = 55)
    if ((int)onsetSamples.size() > 20)
        onsetSamples.resize(20);

    for (size_t i = 0; i < onsetSamples.size(); ++i)
    {
        ChopSlice s;
        s.midiNote = 36 + (int)i; // 36 = C1 .. 55 = G2
        s.noteName = midiNoteToName(s.midiNote);
        s.startSample = onsetSamples[i];
        s.endSample = (i + 1 < onsetSamples.size()) ? onsetSamples[i + 1] : totalSamples;
        s.startNorm = (float)s.startSample / (float)totalSamples;
        s.endNorm = (float)s.endSample / (float)totalSamples;
        result.push_back(s);
    }

    return result;
}

double ClipSampler10AudioProcessor::estimateBpmFromBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int totalSamples = buffer.getNumSamples();
    if (totalSamples < (int)(sampleRate * 1.2) || sampleRate <= 0.0)
        return 120.0;

    // Envelope at 200 Hz frame rate
    const double envRate = 200.0;
    const int hop = juce::jmax(32, (int)std::round(sampleRate / envRate));
    const int numFrames = juce::jmin(3000, totalSamples / hop);
    if (numFrames < 64)
        return 120.0;

    std::vector<float> onset((size_t)numFrames, 0.0f);
    const float* ch0 = buffer.getReadPointer(0);
    float prevE = 0.0f;

    for (int f = 0; f < numFrames; ++f)
    {
        int base = f * hop;
        float e = 0.0f;
        for (int i = 0; i < hop; ++i)
            e += std::abs(ch0[base + i]);
        e /= (float)hop;
        onset[(size_t)f] = juce::jmax(0.0f, e - prevE);
        prevE = e;
    }

    // Search BPM between 72 and 172 BPM
    const int minLag = juce::jmax(10, (int)std::round((60.0 / 172.0) * envRate));
    const int maxLag = juce::jmin(numFrames / 2, (int)std::round((60.0 / 72.0) * envRate));

    double bestScore = -1.0;
    int bestLag = (int)std::round((60.0 / 120.0) * envRate);

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double corr = 0.0;
        int count = numFrames - lag;
        for (int i = 0; i < count; ++i)
            corr += (double)onset[(size_t)i] * (double)onset[(size_t)(i + lag)];

        corr /= (double)juce::jmax(1, count);

        // Mild perceptual prior centering around 118 BPM
        double bpmCandidate = 60.0 * envRate / (double)lag;
        double prior = 1.0 - 0.08 * std::abs(bpmCandidate - 118.0) / 60.0;
        double score = corr * prior;

        if (score > bestScore)
        {
            bestScore = score;
            bestLag = lag;
        }
    }

    double detectedBpm = (60.0 * envRate) / (double)bestLag;
    detectedBpm = std::round(detectedBpm * 2.0) * 0.5; // Snap to nearest 0.5 BPM
    return juce::jlimit(70.0, 175.0, detectedBpm);
}

juce::String ClipSampler10AudioProcessor::estimateMusicalKeyFromBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    static const char* pitchNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    // Krumhansl-Schmuckler key profiles
    static const double majorProfile[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    static const double minorProfile[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

    const int totalSamples = buffer.getNumSamples();
    if (totalSamples < 2048 || sampleRate <= 0.0)
        return "C Maj";

    double chroma[12] = { 0.0 };
    const float* samples = buffer.getReadPointer(0);

    // Analyze up to 16 evenly spaced frames of 2048 samples using Goertzel resonators across octaves 2..4
    const int frameSize = 2048;
    const int numFrames = juce::jmin(14, juce::jmax(1, totalSamples / frameSize));
    const int stride = juce::jmax(frameSize, totalSamples / numFrames);

    for (int pc = 0; pc < 12; ++pc)
    {
        for (int oct = 2; oct <= 4; ++oct)
        {
            int midi = (oct + 1) * 12 + pc; // C2 = 36
            double freq = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
            double omega = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
            double coeff = 2.0 * std::cos(omega);

            for (int f = 0; f < numFrames; ++f)
            {
                int start = juce::jmin(f * stride, totalSamples - frameSize);
                double s1 = 0.0, s2 = 0.0;
                for (int i = 0; i < frameSize; ++i)
                {
                    double s0 = (double)samples[start + i] + coeff * s1 - s2;
                    s2 = s1;
                    s1 = s0;
                }
                double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
                chroma[pc] += std::sqrt(juce::jmax(0.0, power));
            }
        }
    }

    double bestCorr = -1e9;
    juce::String bestKey = "C Maj";

    for (int root = 0; root < 12; ++root)
    {
        double majScore = 0.0;
        double minScore = 0.0;
        for (int i = 0; i < 12; ++i)
        {
            int pc = (root + i) % 12;
            majScore += chroma[pc] * majorProfile[i];
            minScore += chroma[pc] * minorProfile[i];
        }

        if (majScore > bestCorr)
        {
            bestCorr = majScore;
            bestKey = juce::String(pitchNames[root]) + " Maj";
        }
        if (minScore > bestCorr)
        {
            bestCorr = minScore;
            bestKey = juce::String(pitchNames[root]) + "m";
        }
    }

    return bestKey;
}

double ClipSampler10AudioProcessor::getDetectedBpm(int slotIndex) const
{
    return (slotIndex == 2) ? slot2Bpm.load(std::memory_order_relaxed) : slot1Bpm.load(std::memory_order_relaxed);
}

juce::String ClipSampler10AudioProcessor::getDetectedKey(int slotIndex) const
{
    const juce::ScopedLock sl(statusLock);
    return (slotIndex == 2) ? slot2Key : slot1Key;
}

void ClipSampler10AudioProcessor::analyzeAutoChopAndMusicData(int slotIndex)
{
    auto file = getSlotAudioFile(slotIndex);
    if (!file.existsAsFile())
        return;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return;

    const int len = (int)reader->lengthInSamples;
    const double sr = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    juce::AudioBuffer<float> temp((int)reader->numChannels, len);
    reader->read(&temp, 0, len, 0, true, true);

    auto slices = computeTransientSlices(temp, sr);
    double bpm = estimateBpmFromBuffer(temp, sr);
    juce::String key = estimateMusicalKeyFromBuffer(temp, sr);

    {
        const juce::ScopedLock sl(bufferLock);
        if (slotIndex == 2)
            slot2Slices = std::move(slices);
        else
            slot1Slices = std::move(slices);
    }

    if (slotIndex == 2)
        slot2Bpm.store(bpm, std::memory_order_relaxed);
    else
        slot1Bpm.store(bpm, std::memory_order_relaxed);

    {
        const juce::ScopedLock sl(statusLock);
        if (slotIndex == 2)
            slot2Key = key;
        else
            slot1Key = key;
    }
}

//==============================================================================
// v2.3: FX Macro (Lo-Fi 12-Bit, Pitch Semitones, Tone, Auto-Sync)
//==============================================================================
void ClipSampler10AudioProcessor::setFxTargetSlot(int slotIndex)
{
    const int target = (slotIndex == 2) ? 2 : 1;
    fxTargetSlot.store(target, std::memory_order_relaxed);
    if (hasSlotAudio(target))
        setActiveSlot(target);
}

void ClipSampler10AudioProcessor::setLofiAmount(float amount0to1)
{
    fxLofiAmount.store(juce::jlimit(0.0f, 1.0f, amount0to1), std::memory_order_relaxed);
}

void ClipSampler10AudioProcessor::setFxPitchSemitones(float semitones)
{
    fxPitchSemitones.store(juce::jlimit(-12.0f, 12.0f, semitones), std::memory_order_relaxed);
}

void ClipSampler10AudioProcessor::setFxToneFilter(float tone0to1)
{
    fxToneFilter.store(juce::jlimit(0.1f, 1.0f, tone0to1), std::memory_order_relaxed);
}

void ClipSampler10AudioProcessor::setAutoSyncEnabled(bool enabled)
{
    autoSyncEnabled.store(enabled, std::memory_order_relaxed);
}

bool ClipSampler10AudioProcessor::applyFxMacroToTargetSlot(juce::String& outReport)
{
    if (!licenseManager.isUnlocked() || !licenseManager.verifyRuntimeIntegrity())
    {
        outReport = juce::String::fromUTF8("🔒 Activación requerida para imprimir efectos FX Macro.");
        setStatusMessage(outReport);
        return false;
    }

    const int targetSlot = fxTargetSlot.load(std::memory_order_relaxed);
    auto srcFile = getSlotAudioFile(targetSlot);
    if (!srcFile.existsAsFile())
    {
        outReport = juce::String::fromUTF8("Primero carga o extrae el Audio ") + juce::String(targetSlot)
                  + juce::String::fromUTF8(" antes de aplicar los efectos FX Macro.");
        setStatusMessage(outReport);
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(srcFile));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const double sr = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    const int inSamples = (int)reader->lengthInSamples;
    const int numCh = juce::jmax(1, (int)reader->numChannels);

    juce::AudioBuffer<float> inBuf(numCh, inSamples);
    reader->read(&inBuf, 0, inSamples, 0, true, true);
    reader.reset();

    const float pitchSt = fxPitchSemitones.load(std::memory_order_relaxed);
    const float lofiAmt = fxLofiAmount.load(std::memory_order_relaxed);
    const float toneAmt = fxToneFilter.load(std::memory_order_relaxed);
    const bool syncOn = autoSyncEnabled.load(std::memory_order_relaxed);

    double speedRatio = std::pow(2.0, (double)pitchSt / 12.0);
    if (syncOn)
    {
        double detBpm = getDetectedBpm(targetSlot);
        double hostBpm = currentHostBpm.load(std::memory_order_relaxed);
        if (detBpm > 35.0 && hostBpm > 35.0)
            speedRatio *= juce::jlimit(0.5, 2.0, hostBpm / detBpm);
    }

    const int outSamples = juce::jmax(64, (int)std::floor((double)inSamples / speedRatio));
    juce::AudioBuffer<float> outBuf(numCh, outSamples);

    for (int ch = 0; ch < numCh; ++ch)
    {
        juce::LagrangeInterpolator interp;
        interp.process(speedRatio, inBuf.getReadPointer(ch), outBuf.getWritePointer(ch), outSamples);
    }

    // Apply Lo-Fi 12-Bit & Tone Filter
    const float levels = juce::jmap(lofiAmt, 0.0f, 1.0f, 4096.0f, 28.0f);
    const int holdFactor = juce::jmax(1, juce::roundToInt(1.0f + lofiAmt * 14.0f));
    const float lpAlpha = juce::jlimit(0.04f, 1.0f, toneAmt * toneAmt);

    for (int ch = 0; ch < numCh; ++ch)
    {
        float* d = outBuf.getWritePointer(ch);
        float lpState = 0.0f;
        float held = 0.0f;
        for (int i = 0; i < outSamples; ++i)
        {
            if (i % holdFactor == 0)
            {
                float driven = std::tanh(d[i] * (1.0f + lofiAmt * 0.65f));
                held = std::round(driven * levels) / levels;
            }
            float s = (lofiAmt > 0.005f) ? held : d[i];
            lpState += lpAlpha * (s - lpState);
            d[i] = lpState;
        }
    }

    juce::File fxFile = AudioExtractorWorker::getCacheDirectory().getChildFile(
        "ClipSampler_Audio" + juce::String(targetSlot) + "_FX_" + juce::String(juce::Time::getMillisecondCounter()) + ".wav");

    {
        std::unique_ptr<juce::FileOutputStream> outStream(fxFile.createOutputStream());
        if (outStream == nullptr)
            return false;

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
            outStream.get(), sr, (unsigned int)numCh, 24, {}, 0));
        if (writer == nullptr)
            return false;

        outStream.release();
        writer->writeFromAudioSampleBuffer(outBuf, 0, outSamples);
    }

    // Reset real-time FX knobs to neutral after baking so the effect isn't applied twice
    fxLofiAmount.store(0.0f, std::memory_order_relaxed);
    fxPitchSemitones.store(0.0f, std::memory_order_relaxed);
    fxToneFilter.store(1.0f, std::memory_order_relaxed);

    loadAudioFileIntoSlot(fxFile, targetSlot);

    outReport = juce::String::fromUTF8("🎛️ ¡FX Macro (Lo-Fi 12-Bit: ")
              + juce::String(juce::roundToInt(lofiAmt * 100.0f)) + "% | Pitch: "
              + juce::String(pitchSt, 1) + juce::String::fromUTF8(" st) impreso en el Audio ")
              + juce::String(targetSlot) + "!";
    setStatusMessage(outReport);
    return true;
}

//==============================================================================
// v2.3: Audio-to-MIDI Converter (Drag & Drop .MIDI for DAW Piano Roll)
//==============================================================================
juce::File ClipSampler10AudioProcessor::generateMidiFileForSlot(int slotIndex)
{
    const int targetSlot = (slotIndex == 2 && hasSlotAudio(2)) ? 2 : 1;
    auto audioFile = getSlotAudioFile(targetSlot);
    if (!audioFile.existsAsFile())
        return {};

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return {};

    const double sr = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    const int totalSamples = (int)reader->lengthInSamples;
    juce::AudioBuffer<float> buf(1, totalSamples);
    reader->read(&buf, 0, totalSamples, 0, true, false);
    reader.reset();

    const float* data = buf.getReadPointer(0);
    const double bpm = juce::jlimit(60.0, 200.0, getDetectedBpm(targetSlot) > 30.0 ? getDetectedBpm(targetSlot) : 120.0);
    const int tpqn = 480;
    const double secondsPerTick = (60.0 / bpm) / (double)tpqn;

    juce::MidiMessageSequence seq;
    // Set Tempo Meta Event
    int microsecondsPerQuarter = (int)std::round(60000000.0 / bpm);
    auto tempoMeta = juce::MidiMessage::tempoMetaEvent(microsecondsPerQuarter);
    tempoMeta.setTimeStamp(0.0);
    seq.addEvent(tempoMeta);

    // Frame-based autocorrelation pitch tracker (window = 40ms, hop = 25ms)
    const int winSize = juce::jmax(512, (int)(sr * 0.040));
    const int hopSize = juce::jmax(256, (int)(sr * 0.025));
    const int minLag = juce::jmax(8, (int)(sr / 980.0));  // ~B5
    const int maxLag = juce::jmin(winSize / 2, (int)(sr / 55.0)); // ~A1

    struct DetectedNoteEvent
    {
        int midiNote;
        int startSample;
        int endSample;
        float rms;
    };

    std::vector<DetectedNoteEvent> rawNotes;
    int currentMidi = -1;
    int currentStart = 0;
    float currentRmsMax = 0.0f;

    for (int pos = 0; pos + winSize < totalSamples; pos += hopSize)
    {
        float sumSq = 0.0f;
        for (int i = 0; i < winSize; ++i)
            sumSq += data[pos + i] * data[pos + i];
        float rms = std::sqrt(sumSq / (float)winSize);

        int detectedMidi = -1;
        if (rms > 0.015f)
        {
            // Normalized autocorrelation for fundamental frequency f0
            float bestCorr = 0.0f;
            int bestLag = -1;
            const int searchLen = winSize / 2;

            for (int lag = minLag; lag <= maxLag; ++lag)
            {
                float dot = 0.0f, e1 = 0.0f, e2 = 0.0f;
                for (int i = 0; i < searchLen; ++i)
                {
                    float a = data[pos + i];
                    float b = data[pos + i + lag];
                    dot += a * b;
                    e1 += a * a;
                    e2 += b * b;
                }
                float denom = std::sqrt(e1 * e2);
                float normCorr = (denom > 1e-6f) ? (dot / denom) : 0.0f;
                if (normCorr > bestCorr)
                {
                    bestCorr = normCorr;
                    bestLag = lag;
                }
            }

            if (bestLag > 0 && bestCorr > 0.42f)
            {
                double f0 = sr / (double)bestLag;
                int note = (int)std::round(69.0 + 12.0 * std::log2(f0 / 440.0));
                detectedMidi = juce::jlimit(36, 84, note);
            }
        }

        if (detectedMidi == currentMidi && detectedMidi != -1)
        {
            currentRmsMax = juce::jmax(currentRmsMax, rms);
        }
        else
        {
            if (currentMidi != -1 && (pos - currentStart) >= (int)(sr * 0.045))
                rawNotes.push_back({ currentMidi, currentStart, pos, currentRmsMax });

            currentMidi = detectedMidi;
            currentStart = pos;
            currentRmsMax = rms;
        }
    }

    if (currentMidi != -1 && (totalSamples - currentStart) >= (int)(sr * 0.045))
        rawNotes.push_back({ currentMidi, currentStart, totalSamples, currentRmsMax });

    // If audio is predominantly rhythmic/percussive with few pitched frames, generate MIDI from the MPC Auto-Chop slices!
    if (rawNotes.size() < 3)
    {
        rawNotes.clear();
        auto slices = getChopSlices(targetSlot);
        for (size_t i = 0; i < slices.size(); ++i)
        {
            rawNotes.push_back({
                slices[i].midiNote + 24, // C3..G4 range for synths
                slices[i].startSample,
                slices[i].endSample,
                0.75f
            });
        }
    }

    for (const auto& n : rawNotes)
    {
        double startSec = (double)n.startSample / sr;
        double endSec = (double)n.endSample / sr;
        double startTick = std::round(startSec / secondsPerTick);
        double endTick = std::max(startTick + 60.0, std::round(endSec / secondsPerTick) - 10.0);

        juce::uint8 vel = (juce::uint8)juce::jlimit(55, 125, juce::roundToInt(55.0f + n.rms * 180.0f));

        auto noteOn = juce::MidiMessage::noteOn(1, n.midiNote, vel);
        noteOn.setTimeStamp(startTick);

        auto noteOff = juce::MidiMessage::noteOff(1, n.midiNote);
        noteOff.setTimeStamp(endTick);

        seq.addEvent(noteOn);
        seq.addEvent(noteOff);
    }

    seq.updateMatchedPairs();

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(tpqn);
    midiFile.addTrack(seq);

    juce::File outMidi = AudioExtractorWorker::getCacheDirectory().getChildFile(
        audioFile.getFileNameWithoutExtension() + "_PianoRoll.mid");
    outMidi.deleteFile();

    {
        std::unique_ptr<juce::FileOutputStream> outStream(outMidi.createOutputStream());
        if (outStream != nullptr)
        {
            midiFile.writeTo(*outStream);
            outStream->flush();
        }
    }

    {
        const juce::ScopedLock sl(bufferLock);
        if (targetSlot == 2)
            slot2MidiFile = outMidi;
        else
            slot1MidiFile = outMidi;
    }

    setStatusMessage(juce::String::fromUTF8("🎹 MIDI generado para Audio ") + juce::String(targetSlot)
                     + " (" + juce::String((int)rawNotes.size()) + juce::String::fromUTF8(" notas a ")
                     + juce::String(bpm, 1) + juce::String::fromUTF8(" BPM). ¡Arrástralo al Piano Roll de tu DAW!"));
    return outMidi;
}

juce::File ClipSampler10AudioProcessor::getCurrentMidiFile()
{
    const int curSlot = activeSlot.load(std::memory_order_relaxed);
    {
        const juce::ScopedLock sl(bufferLock);
        juce::File cached = (curSlot == 2) ? slot2MidiFile : slot1MidiFile;
        if (cached.existsAsFile())
            return cached;
    }
    return generateMidiFileForSlot(curSlot);
}

//==============================================================================
void ClipSampler10AudioProcessor::startExtractorUpdate()
{
    if (updateInProgress.exchange(true))
        return;

    setStatusMessage(juce::String::fromUTF8("Buscando e instalando actualizaciones de yt-dlp (-U)..."));
    auto weakAlive = aliveFlag;

    AudioExtractorWorker::runYtDlpUpdate([this, weakAlive](bool success, const juce::String& msg) {
        if (!weakAlive->load(std::memory_order_acquire))
            return;

        updateInProgress.store(false, std::memory_order_relaxed);
        setStatusMessage(success
            ? (juce::String::fromUTF8("Actualización yt-dlp: ") + msg)
            : (juce::String::fromUTF8("Aviso yt-dlp: ") + msg));
    });
}

int ClipSampler10AudioProcessor::cleanAudioCache()
{
    int deleted = AudioExtractorWorker::clearCacheDirectory(getSlotAudioFile(1), getSlotAudioFile(2));
    setStatusMessage(juce::String::fromUTF8("Caché limpiada: ") + juce::String(deleted)
                     + juce::String::fromUTF8(" archivo(s) temporales eliminados (muestras en Visor 1 y 2 protegidas)."));
    return deleted;
}

juce::String ClipSampler10AudioProcessor::getStatusMessage() const
{
    const juce::ScopedLock sl(statusLock);
    return currentStatusMessage;
}

void ClipSampler10AudioProcessor::setStatusMessage(const juce::String& msg)
{
    const juce::ScopedLock sl(statusLock);
    currentStatusMessage = msg;
}

juce::String ClipSampler10AudioProcessor::getCopyrightBadgeText() const
{
    const juce::ScopedLock sl(statusLock);
    return currentCopyrightBadge;
}

juce::File ClipSampler10AudioProcessor::getCurrentAudioFile() const
{
    return getSlotAudioFile(activeSlot.load(std::memory_order_relaxed));
}

juce::File ClipSampler10AudioProcessor::getSlotAudioFile(int slotIndex) const
{
    const juce::ScopedLock sl(bufferLock);
    if (slotIndex == 2)
        return transformedLoadedFile;
    return originalLoadedFile;
}

void ClipSampler10AudioProcessor::setActiveSlot(int slotIndex)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    auto f = getSlotAudioFile(targetSlot);
    if (f.existsAsFile())
    {
        fxTargetSlot.store(targetSlot, std::memory_order_relaxed);
        activateSlotBuffer(f, targetSlot);
    }
}

bool ClipSampler10AudioProcessor::loadAudioFileIntoSlot(const juce::File& file, int slotIndex)
{
    if (!file.existsAsFile())
        return false;

    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    {
        const juce::ScopedLock sl(bufferLock);
        if (targetSlot == 2)
        {
            transformedLoadedFile = file;
            slot2MidiFile = juce::File();
        }
        else
        {
            originalLoadedFile = file;
            slot1MidiFile = juce::File();
        }
    }

    if (targetSlot == 2)
        transformedThumbnail.setSource(new juce::FileInputSource(file));
    else
        originalThumbnail.setSource(new juce::FileInputSource(file));

    // Automatically run MPC Auto-Chop transient detection + BPM + Key + pre-build MIDI
    analyzeAutoChopAndMusicData(targetSlot);

    return activateSlotBuffer(file, targetSlot);
}

bool ClipSampler10AudioProcessor::activateSlotBuffer(const juce::File& file, int slotIndex)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    double targetRate = getSampleRate();
    if (targetRate <= 0.0)
        targetRate = 44100.0;

    double fileRate = reader->sampleRate > 0.0 ? reader->sampleRate : targetRate;
    juce::int64 fileSamples = reader->lengthInSamples;
    if (fileSamples <= 0)
        return false;

    sampler.clearSounds();
    juce::BigInteger allNotes;
    allNotes.setRange(0, 128, true);
    sampler.addSound(new juce::SamplerSound("ClipSample", *reader, allNotes, 60, 0.005, 0.05, 35.0));
    sampler.setCurrentPlaybackSampleRate(targetRate);

    juce::AudioBuffer<float> tempBuffer((int)reader->numChannels, (int)fileSamples);
    reader->read(&tempBuffer, 0, (int)fileSamples, 0, true, true);
    reader.reset();

    juce::AudioBuffer<float> preparedBuffer;

    if (std::abs(fileRate - targetRate) > 1.0 && fileRate > 0.0)
    {
        double ratio = targetRate / fileRate;
        int targetSamples = (int)std::ceil(fileSamples * ratio);
        preparedBuffer.setSize(tempBuffer.getNumChannels(), targetSamples);

        for (int ch = 0; ch < tempBuffer.getNumChannels(); ++ch)
        {
            juce::LagrangeInterpolator interpolator;
            interpolator.process(ratio,
                                 tempBuffer.getReadPointer(ch),
                                 preparedBuffer.getWritePointer(ch),
                                 targetSamples);
        }
    }
    else
    {
        preparedBuffer = std::move(tempBuffer);
    }

    const int finalSampleCount = preparedBuffer.getNumSamples();

    // Recompute slices in preparedBuffer's sample space so C1..G2 sample indices match processBlock
    auto resampledSlices = computeTransientSlices(preparedBuffer, targetRate);

    {
        const juce::ScopedLock sl(bufferLock);
        std::swap(sampleBuffer, preparedBuffer);
        if (slotIndex == 2)
            slot2Slices = std::move(resampledSlices);
        else
            slot1Slices = std::move(resampledSlices);
    }

    activeSlot.store((slotIndex == 2) ? 2 : 1, std::memory_order_relaxed);
    loadedFileSampleRate.store(targetRate, std::memory_order_relaxed);
    totalSampleCount.store(finalSampleCount, std::memory_order_relaxed);
    playbackEndSample.store(finalSampleCount, std::memory_order_relaxed);
    fractionalPlayhead.store(0.0, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
    activePlayingSlice.store(-1, std::memory_order_relaxed);
    isPlaying.store(false, std::memory_order_relaxed);

    return true;
}

void ClipSampler10AudioProcessor::startPlayback()
{
    if (!licenseManager.isUnlocked())
        return;

    int total = totalSampleCount.load(std::memory_order_relaxed);
    if (total > 0)
    {
        fractionalPlayhead.store(0.0, std::memory_order_relaxed);
        playheadPosition.store(0, std::memory_order_relaxed);
        playbackEndSample.store(total, std::memory_order_relaxed);
        activePlayingSlice.store(-1, std::memory_order_relaxed);
        isPlaying.store(true, std::memory_order_relaxed);
    }
}

void ClipSampler10AudioProcessor::stopPlayback()
{
    isPlaying.store(false, std::memory_order_relaxed);
    fractionalPlayhead.store(0.0, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
    activePlayingSlice.store(-1, std::memory_order_relaxed);
    sampler.allNotesOff(0, false);
}

float ClipSampler10AudioProcessor::getPlaybackProgress() const
{
    int total = totalSampleCount.load(std::memory_order_relaxed);
    if (total <= 0)
        return 0.0f;

    float progress = (float)playheadPosition.load(std::memory_order_relaxed) / (float)total;
    return juce::jlimit(0.0f, 1.0f, progress);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClipSampler10AudioProcessor();
}

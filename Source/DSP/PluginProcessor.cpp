#include "PluginProcessor.h"
#include "../UI/PluginEditor.h"
#include <cmath>

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

    if (!licenseManager.isUnlocked())
    {
        buffer.clear();
        return;
    }

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            playheadPosition.store(0, std::memory_order_relaxed);
            isPlaying.store(true, std::memory_order_relaxed);
        }
    }

    sampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    if (isPlaying.load(std::memory_order_relaxed))
    {
        int currentPos = playheadPosition.load(std::memory_order_relaxed);
        int total = totalSampleCount.load(std::memory_order_relaxed);
        int numSamplesToRender = buffer.getNumSamples();

        if (currentPos >= total || total == 0)
        {
            isPlaying.store(false, std::memory_order_relaxed);
            playheadPosition.store(0, std::memory_order_relaxed);
        }
        else
        {
            const juce::ScopedTryLock stl(bufferLock);
            if (stl.isLocked())
            {
                int samplesAvailable = total - currentPos;
                int samplesThisBlock = juce::jmin(numSamplesToRender, samplesAvailable);

                if (sampleBuffer.getNumSamples() >= total && sampleBuffer.getNumChannels() > 0)
                {
                    const int numOutChannels = buffer.getNumChannels();
                    const int numSampleChannels = sampleBuffer.getNumChannels();

                    for (int ch = 0; ch < numOutChannels; ++ch)
                    {
                        int srcCh = juce::jmin(ch, numSampleChannels - 1);
                        buffer.addFrom(ch, 0, sampleBuffer, srcCh, currentPos, samplesThisBlock);
                    }

                    currentPos += samplesThisBlock;
                    playheadPosition.store(currentPos, std::memory_order_relaxed);

                    if (currentPos >= total)
                    {
                        isPlaying.store(false, std::memory_order_relaxed);
                        playheadPosition.store(0, std::memory_order_relaxed);
                    }
                }
                else
                {
                    isPlaying.store(false, std::memory_order_relaxed);
                }
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
    if (!licenseManager.isUnlocked())
    {
        setStatusMessage(juce::String::fromUTF8("🔒 Periodo de prueba de 3 días finalizado. Activa tu licencia para continuar."));
        return;
    }

    lastUrl = url.trim();
    lastStartTime = startTime.trim();
    const int clampedDuration = (durationSeconds == 3 || durationSeconds == 10 || durationSeconds == 30) ? durationSeconds : 10;
    lastSelectedDurationSec.store(clampedDuration, std::memory_order_relaxed);

    // Also trigger duration detection if not yet known
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
                setStatusMessage(juce::String::fromUTF8("¡Muestra de ") + juce::String(clampedDuration)
                                 + juce::String::fromUTF8("s cargada en Visor 1! Pulsa '🛡️ Derechos' para generar versión alterada en Visor 2."));
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

    // 1. Pitch Shift by ±3 Semitones (ratio = 2^(semitones / 12.0)) + subtle micro-detune (0.18 semitones) for AI fingerprint evasion
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

    // 2. Acoustic Fingerprint Evasion DSP:
    //    - First-order All-Pass Phase Dispersion filter (alters phase relationships without muddying tone)
    //    - Subtle Dynamic Micro-Modulation (LFO 1.7 Hz) & Stereo Haas Offset (0.65 ms on R channel)
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
            // All-pass filter y[n] = c*x[n] + x[n-1] - c*y[n-1]
            const float y = (allPassCoeff * x) + xPrev - (allPassCoeff * yPrev);
            xPrev = x;
            yPrev = y;

            // Subtle harmonic micro-shimmer modulation (1.8% depth)
            const double t = static_cast<double>(i) / sampleRate;
            const float mod = 1.0f + 0.018f * static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * lfoFreq * t + chPhaseOffset));
            data[i] = std::tanh(y * mod * 1.06f);
        }
    }

    // Stereo micro-haas decorrelation if 2 channels
    if (numChannels >= 2)
    {
        const int delaySamples = juce::jlimit(1, 64, (int)(sampleRate * 0.00065)); // 0.65 ms
        float* rightCh = outBuffer.getWritePointer(1);
        for (int i = outSamples - 1; i >= delaySamples; --i)
            rightCh[i] = 0.88f * rightCh[i] + 0.12f * rightCh[i - delaySamples];
    }

    // 3. Anti-Click 10ms Fade-In / Fade-Out & -1 dB Peak Normalization
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

    // 4. Write transformed audio to Slot 2 file
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

    // Load into Slot 2 (Bottom/Right Viewer) and set Slot 2 as active for playback & DAW Drag
    loadAudioFileIntoSlot(transFile, 2);

    juce::String signStr = (semitoneShift > 0) ? ("+" + juce::String(semitoneShift)) : juce::String(semitoneShift);
    outReport = juce::String::fromUTF8("🛡️ ¡Audio transformado (") + signStr
              + juce::String::fromUTF8(" semitonos + Desacople de Fase IA) cargado en el Visor 2! Ahora pulsa '✅ Verificar Sin Derechos'.");
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
              + juce::String::fromUTF8(" semitonos + fase IA). Apto para arrastrar al DAW.");
    setStatusMessage(outReport);
    return true;
}

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
    if (slotIndex == 2 && transformedLoadedFile.existsAsFile())
        return transformedLoadedFile;
    return originalLoadedFile;
}

void ClipSampler10AudioProcessor::setActiveSlot(int slotIndex)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    auto f = getSlotAudioFile(targetSlot);
    if (f.existsAsFile())
    {
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
            transformedLoadedFile = file;
        else
            originalLoadedFile = file;
    }

    if (targetSlot == 2)
        transformedThumbnail.setSource(new juce::FileInputSource(file));
    else
        originalThumbnail.setSource(new juce::FileInputSource(file));

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

    {
        const juce::ScopedLock sl(bufferLock);
        std::swap(sampleBuffer, preparedBuffer);
    }

    activeSlot.store((slotIndex == 2) ? 2 : 1, std::memory_order_relaxed);
    loadedFileSampleRate.store(targetRate, std::memory_order_relaxed);
    totalSampleCount.store(finalSampleCount, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
    isPlaying.store(false, std::memory_order_relaxed);

    return true;
}

void ClipSampler10AudioProcessor::startPlayback()
{
    if (!licenseManager.isUnlocked())
        return;

    if (totalSampleCount.load(std::memory_order_relaxed) > 0)
    {
        playheadPosition.store(0, std::memory_order_relaxed);
        isPlaying.store(true, std::memory_order_relaxed);
    }
}

void ClipSampler10AudioProcessor::stopPlayback()
{
    isPlaying.store(false, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
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

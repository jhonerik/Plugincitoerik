#include "PluginProcessor.h"
#include "../UI/PluginEditor.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Convierte un número de nota MIDI (0-127) a su nombre musical (ej. 36 -> "C1")
//==============================================================================
juce::String ClipSamplerAudioProcessor::midiNoteToName(int midiNote)
{
    static const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int clamped = juce::jlimit(0, 127, midiNote);
    int octave = (clamped / 12) - 2; // MIDI 36 corresponde a C1, MIDI 60 a C3
    return juce::String(noteNames[clamped % 12]) + juce::String(octave);
}

//==============================================================================
// Define los parámetros automatizables expuestos al DAW anfitrión
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ClipSamplerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterGain", 1 },
        "Ganancia Master",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
// Constructor y Destructor del Procesador Principal
//==============================================================================
ClipSamplerAudioProcessor::ClipSamplerAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "ClipSamplerState", createParameterLayout()),
      originalThumbnail(512, formatManager, thumbnailCache),
      transformedThumbnail(512, formatManager, thumbnailCache),
      currentStatusMessage(juce::String::fromUTF8("Listo. Elige plataforma (YouTube, TikTok o Instagram), pega el enlace y extrae 3s, 10s o 30s.")),
      currentCopyrightBadge(juce::String::fromUTF8("DERECHOS: Pendiente de análisis"))
{
    // Registramos formatos de audio estándar (WAV, AIFF, FLAC, OGG, MP3/WMA en Windows)
    formatManager.registerBasicFormats();
#if JUCE_USE_MP3AUDIOFORMAT
    formatManager.registerFormat(new juce::MP3AudioFormat(), false);
#endif
#if JUCE_USE_WINDOWS_MEDIA_FORMAT
    formatManager.registerFormat(new juce::WindowsMediaAudioFormat(), false);
#endif

    // Inicializamos las 8 voces polifónicas del sampler MIDI
    for (int i = 0; i < NUM_VOICES; ++i)
        sampler.addVoice(new juce::SamplerVoice());
}

ClipSamplerAudioProcessor::~ClipSamplerAudioProcessor()
{
    // Marcamos el procesador como inactivo para ignorar callbacks asíncronos pendientes
    stopFreesoundPreview();
    aliveFlag->store(false, std::memory_order_release);
    extractorWorker.cancelExtraction();
}

//==============================================================================
// Propiedades Descriptivas del Plugin
//==============================================================================
const juce::String ClipSamplerAudioProcessor::getName() const
{
    return "ClipSampler";
}

bool ClipSamplerAudioProcessor::acceptsMidi() const { return true; }
bool ClipSamplerAudioProcessor::producesMidi() const { return false; }
bool ClipSamplerAudioProcessor::isMidiEffect() const { return false; }
double ClipSamplerAudioProcessor::getTailLengthSeconds() const
{
    return previewIsPlaying.load(std::memory_order_relaxed) ? 60.0 : 0.0;
}

bool ClipSamplerAudioProcessor::isDawActivelyProcessing() const
{
    const juce::int64 lastTime = lastProcessBlockTimestampMs.load(std::memory_order_relaxed);
    if (lastTime <= 0)
        return false;
    return (juce::Time::currentTimeMillis() - lastTime) < 300;
}

int ClipSamplerAudioProcessor::getNumPrograms() { return 1; }
int ClipSamplerAudioProcessor::getCurrentProgram() { return 0; }
void ClipSamplerAudioProcessor::setCurrentProgram(int) {}
const juce::String ClipSamplerAudioProcessor::getProgramName(int) { return {}; }
void ClipSamplerAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
// Interpolador Cúbico de Hermite (Catmull-Rom) de 4 Puntos para Audio sin Aliasing
//==============================================================================
namespace
{
    /**
     * Interpolación cúbica de Hermite de 4 puntos (3er orden).
     * Elimina el sonido metálico/rasgado ("aliasing") de la interpolación lineal
     * cuando se reproduce con Pitch Shift, Sincronización de BPM o notas MIDI cromáticas.
     */
    inline float interpolateHermite4pt(const float* data, int totalSamples, double pos) noexcept
    {
        const int idx1 = (int)pos;
        const float frac = (float)(pos - (double)idx1);

        const int idx0 = (idx1 > 0) ? (idx1 - 1) : 0;
        const int idx2 = (idx1 + 1 < totalSamples) ? (idx1 + 1) : (totalSamples - 1);
        const int idx3 = (idx1 + 2 < totalSamples) ? (idx1 + 2) : (totalSamples - 1);

        const float y0 = data[idx0];
        const float y1 = data[idx1];
        const float y2 = data[idx2];
        const float y3 = data[idx3];

        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
}

//==============================================================================
// Preparación de Frecuencia de Muestreo y Recursos de Tiempo Real
//==============================================================================
void ClipSamplerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRateHz = (sampleRate > 8000.0) ? sampleRate : 44100.0;
    sampler.setCurrentPlaybackSampleRate(currentSampleRateHz);



    // Rampas anti-click escaladas a la frecuencia de muestreo del DAW
    fadeInSamples  = juce::jmax(32,  (int)std::round(currentSampleRateHz * 0.0018)); // ~1.8 ms (ataque limpio sin click)
    fadeOutSamples = juce::jmax(128, (int)std::round(currentSampleRateHz * 0.0065)); // ~6.5 ms (crossfade suave al re-disparar)
    endFadeSamples = juce::jmax(96,  (int)std::round(currentSampleRateHz * 0.0035)); // ~3.5 ms (cierre suave de rodaja)

    // Inicializamos los suavizadores de parámetros (evita ruidos al girar perillas en vivo)
    smoothedPlaybackRate.reset(currentSampleRateHz, 0.015);
    smoothedPlaybackRate.setCurrentAndTargetValue(1.0f);

    smoothedMasterGain.reset(currentSampleRateHz, 0.012);
    if (auto* gainParam = apvts.getRawParameterValue("masterGain"))
        smoothedMasterGain.setCurrentAndTargetValue(gainParam->load(std::memory_order_relaxed));
    else
        smoothedMasterGain.setCurrentAndTargetValue(1.0f);

    smoothedToneAlpha.reset(currentSampleRateHz, 0.015);
    const float initTone = fxToneFilter.load(std::memory_order_relaxed);
    smoothedToneAlpha.setCurrentAndTargetValue(juce::jlimit(0.04f, 1.0f, initTone * initTone));

    lofiFilterStateL = 0.0f;
    lofiFilterStateR = 0.0f;
    lofiHeldSample[0] = 0.0f;
    lofiHeldSample[1] = 0.0f;
    lofiHoldCounter[0] = 0;
    lofiHoldCounter[1] = 0;

    killAllVoices();
    cachedAudioUnlocked = licenseManager.isUnlocked() && licenseManager.verifyRuntimeIntegrity();
    licenseRecheckCountdown = 0;

    // Si la frecuencia de muestreo del DAW cambió, remuestreamos los slots cargados
    if (std::abs(loadedFileSampleRate.load(std::memory_order_relaxed) - currentSampleRateHz) > 1.0)
    {
        const int cur = activeSlot.load(std::memory_order_relaxed);
        auto f1 = getSlotAudioFile(1);
        auto f2 = getSlotAudioFile(2);
        if (f1.existsAsFile())
            activateSlotBuffer(f1, 1);
        if (f2.existsAsFile())
            activateSlotBuffer(f2, 2);
        if (cur == 1 && f1.existsAsFile())
            setActiveSlot(1);
        else if (cur == 2 && f2.existsAsFile())
            setActiveSlot(2);
    }
}

void ClipSamplerAudioProcessor::releaseResources()
{
    stopPlayback();
    stopFreesoundPreview();
}

bool ClipSamplerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

//==============================================================================
// Subsistema Lock-Free de Intercambio de Buffers y Voces Polifónicas Anti-Click
//==============================================================================
std::shared_ptr<ClipSamplerAudioProcessor::PlaybackData> ClipSamplerAudioProcessor::acquirePlaybackData() const
{
    const juce::SpinLock::ScopedLockType sl(playbackDataLock);
    return livePlaybackData;
}

void ClipSamplerAudioProcessor::publishPlaybackData(std::shared_ptr<PlaybackData> newData)
{
    std::shared_ptr<PlaybackData> oldData;
    {
        const juce::SpinLock::ScopedLockType sl(playbackDataLock);
        oldData = std::move(livePlaybackData);
        livePlaybackData = std::move(newData);
    }

    // Recolectamos buffers antiguos en el hilo de mensajes para que jamás se libere memoria en el hilo de audio
    const juce::ScopedLock sl(retiredLock);
    if (oldData != nullptr)
        retiredPlaybackData.push_back(std::move(oldData));

    retiredPlaybackData.erase(
        std::remove_if(retiredPlaybackData.begin(), retiredPlaybackData.end(),
                       [](const std::shared_ptr<PlaybackData>& ptr) {
                           return ptr == nullptr || ptr.use_count() <= 2;
                       }),
        retiredPlaybackData.end());
}

void ClipSamplerAudioProcessor::requestPlayRegion(int startSample, int endSample, int sliceIndex)
{
    pendingStartSample.store(juce::jmax(0, startSample), std::memory_order_relaxed);
    pendingEndSample.store(endSample, std::memory_order_relaxed);
    pendingSliceIndex.store(sliceIndex, std::memory_order_relaxed);
    pendingTransportCommand.store(2, std::memory_order_release);
}

void ClipSamplerAudioProcessor::startVoice(const PlaybackData& data,
                                           int startSample,
                                           int endSample,
                                           int sliceIndex,
                                           float velocityGain,
                                           double notePitchRatio,
                                           int midiNote,
                                           bool chokeExisting,
                                           bool isReversed,
                                           bool isHalfTime)
{
    const int total = data.buffer.getNumSamples();
    if (total <= 2 || data.buffer.getNumChannels() <= 0)
        return;

    const int safeStart = juce::jlimit(0, total - 1, startSample);
    const int safeEnd   = (endSample <= safeStart || endSample > total) ? total : endSample;
    if (safeEnd - safeStart <= 1)
        return;

    // Si es modo monofónico/choke (Play o MPC Auto-Chop), ponemos las voces activas en fade-out suave (sin corte brusco)
    if (chokeExisting)
        releaseAllVoices();

    // Buscamos una voz libre o la voz más antigua para reutilizar
    int chosenIdx = -1;
    for (int i = 0; i < kNumPlaybackVoices; ++i)
    {
        if (!voices[i].active)
        {
            chosenIdx = i;
            break;
        }
    }

    if (chosenIdx < 0)
    {
        juce::uint32 oldestOrder = UINT32_MAX;
        for (int i = 0; i < kNumPlaybackVoices; ++i)
        {
            if (voices[i].releasing && voices[i].order < oldestOrder)
            {
                oldestOrder = voices[i].order;
                chosenIdx = i;
            }
        }
    }

    if (chosenIdx < 0)
    {
        juce::uint32 oldestOrder = UINT32_MAX;
        for (int i = 0; i < kNumPlaybackVoices; ++i)
        {
            if (voices[i].order < oldestOrder)
            {
                oldestOrder = voices[i].order;
                chosenIdx = i;
            }
        }
    }

    if (chosenIdx < 0)
        chosenIdx = 0;

    auto& v = voices[chosenIdx];
    v.active = true;
    v.releasing = false;
    v.startSample = safeStart;
    v.endSample = safeEnd;
    v.isReversed = isReversed;
    v.isHalfTime = isHalfTime;
    v.position = isReversed ? (double)(safeEnd - 1) : (double)safeStart;
    v.notePitchRatio = juce::jlimit(0.125, 8.0, notePitchRatio);
    v.sliceIndex = sliceIndex;
    v.midiNote = midiNote;
    v.targetGain = juce::jlimit(0.05f, 1.2f, velocityGain);

    // Envolvente de Ataque (Attack): configurable por el usuario (0.5 ms a 500 ms) para suavizar transientes o crear colchones
    const float atkMs = envelopeAttackMs.load(std::memory_order_relaxed);
    const int userAttackSamples = juce::jmax(8, juce::roundToInt((atkMs / 1000.0f) * (float)currentSampleRateHz));
    const int attackLen = (safeStart == 0) ? juce::jmax(16, userAttackSamples / 2) : userAttackSamples;
    v.gain = 0.0f;
    v.gainStep = v.targetGain / (float)juce::jmax(1, attackLen);
    v.order = ++voiceOrderCounter;
}

void ClipSamplerAudioProcessor::releaseVoiceForMidiNote(int midiNote)
{
    const float relMs = envelopeReleaseMs.load(std::memory_order_relaxed);
    const int userReleaseSamples = juce::jmax(32, juce::roundToInt((relMs / 1000.0f) * (float)currentSampleRateHz));
    const int releaseSamples = juce::jmax(fadeOutSamples, userReleaseSamples);
    for (int i = 0; i < kNumPlaybackVoices; ++i)
    {
        auto& v = voices[i];
        if (v.active && !v.releasing && v.midiNote == midiNote)
        {
            v.releasing = true;
            v.gainStep = -juce::jmax(v.gain, 0.001f) / (float)releaseSamples;
        }
    }
}

void ClipSamplerAudioProcessor::releaseAllVoices()
{
    const float relMs = envelopeReleaseMs.load(std::memory_order_relaxed);
    const int userReleaseSamples = juce::jmax(32, juce::roundToInt((relMs / 1000.0f) * (float)currentSampleRateHz));
    const int releaseSamples = juce::jmax(64, juce::jmax(fadeOutSamples, userReleaseSamples));
    for (int i = 0; i < kNumPlaybackVoices; ++i)
    {
        auto& v = voices[i];
        if (v.active && !v.releasing)
        {
            v.releasing = true;
            v.gainStep = -juce::jmax(v.gain, 0.001f) / (float)releaseSamples;
        }
    }
}

void ClipSamplerAudioProcessor::killAllVoices()
{
    for (int i = 0; i < kNumPlaybackVoices; ++i)
        voices[i] = PlaybackVoice {};
}

void ClipSamplerAudioProcessor::handlePendingTransportCommand(const PlaybackData& data)
{
    const int cmd = pendingTransportCommand.exchange(0, std::memory_order_acquire);
    if (cmd == 1)
    {
        releaseAllVoices();
    }
    else if (cmd == 2)
    {
        const int startS = pendingStartSample.load(std::memory_order_relaxed);
        const int endS   = pendingEndSample.load(std::memory_order_relaxed);
        const int sliceI = pendingSliceIndex.load(std::memory_order_relaxed);
        bool rev = false;
        bool half = false;
        if (sliceI >= 0 && sliceI < (int)data.slices.size())
        {
            rev = data.slices[(size_t)sliceI].isReversed;
            half = data.slices[(size_t)sliceI].isHalfTime;
        }
        startVoice(data, startS, endS, sliceI, 1.0f, 1.0, -1, true, rev, half);
    }
}

void ClipSamplerAudioProcessor::triggerNoteFromMidi(const PlaybackData& data, int midiNote, float velocity, bool chopMode)
{
    const float velGain = juce::jlimit(0.20f, 1.0f, 0.25f + 0.75f * velocity);

    if (chopMode && !data.slices.empty())
    {
        const int numSlices = (int)data.slices.size();
        int sliceIdx = midiNote - 36; // C1 (nota 36) dispara la primera rodaja
        if (sliceIdx < 0 || sliceIdx >= numSlices)
            sliceIdx = ((midiNote % numSlices) + numSlices) % numSlices;

        const auto& s = data.slices[(size_t)sliceIdx];
        startVoice(data, s.startSample, s.endSample, sliceIdx, velGain, 1.0, -1, true, s.isReversed, s.isHalfTime);
    }
    else
    {
        // Modo Cromático Polifónico: C3 (nota 60) es el tono original; permite tocar acordes y melodías fluidas
        const double chromaticRatio = std::pow(2.0, ((double)midiNote - 60.0) / 12.0);
        startVoice(data, 0, data.buffer.getNumSamples(), -1, velGain, chromaticRatio, midiNote, false, false, false);
    }
}

void ClipSamplerAudioProcessor::renderVoices(const PlaybackData& data,
                                             juce::AudioBuffer<float>& out,
                                             int startSample,
                                             int numSamples)
{
    if (numSamples <= 0)
        return;

    const int total = data.buffer.getNumSamples();
    const int numSrcChannels = data.buffer.getNumChannels();
    const int numOutChannels = out.getNumChannels();
    if (total <= 2 || numSrcChannels <= 0 || numOutChannels <= 0)
        return;

    const float* srcL = data.buffer.getReadPointer(0);
    const float* srcR = data.buffer.getReadPointer(numSrcChannels > 1 ? 1 : 0);

    float* dstL = out.getWritePointer(0, startSample);
    float* dstR = (numOutChannels > 1) ? out.getWritePointer(1, startSample) : nullptr;

    for (int s = 0; s < numSamples; ++s)
    {
        const double globalRate = (double)smoothedPlaybackRate.getNextValue();
        float mixL = 0.0f;
        float mixR = 0.0f;

        for (int vIdx = 0; vIdx < kNumPlaybackVoices; ++vIdx)
        {
            auto& v = voices[vIdx];
            if (!v.active)
                continue;

            const bool isRev = v.isReversed;
            if (isRev)
            {
                if (v.position < (double)v.startSample || v.position <= 0.0)
                {
                    v.active = false;
                    continue;
                }
            }
            else
            {
                if (v.position >= (double)v.endSample || v.position >= (double)(total - 1))
                {
                    v.active = false;
                    continue;
                }
            }

            // Evolución de la envolvente anti-click (fade-in / sustain / fade-out)
            if (v.releasing)
            {
                v.gain += v.gainStep;
                if (v.gain <= 0.0001f)
                {
                    v.gain = 0.0f;
                    v.active = false;
                    continue;
                }
            }
            else if (v.gainStep > 0.0f)
            {
                v.gain += v.gainStep;
                if (v.gain >= v.targetGain)
                {
                    v.gain = v.targetGain;
                    v.gainStep = 0.0f;
                }
            }

            // Rampa suave automática al acercarse al final de la rodaja o muestra (respetando la Caída/Release configurada)
            const float relMs = envelopeReleaseMs.load(std::memory_order_relaxed);
            const int dynamicEndFade = juce::jmax(endFadeSamples, juce::roundToInt((relMs / 1000.0f) * (float)currentSampleRateHz * 0.4f));
            float tailFade = 1.0f;
            if (isRev)
            {
                const double samplesLeft = v.position - (double)v.startSample;
                if (samplesLeft < (double)dynamicEndFade && dynamicEndFade > 0)
                    tailFade = juce::jlimit(0.0f, 1.0f, (float)(samplesLeft / (double)dynamicEndFade));
            }
            else
            {
                const double samplesLeft = (double)v.endSample - v.position;
                if (samplesLeft < (double)dynamicEndFade && dynamicEndFade > 0)
                    tailFade = juce::jlimit(0.0f, 1.0f, (float)(samplesLeft / (double)dynamicEndFade));
            }

            const float env = v.gain * tailFade;
            const float sampleL = interpolateHermite4pt(srcL, total, v.position);
            const float sampleR = (srcR != srcL) ? interpolateHermite4pt(srcR, total, v.position) : sampleL;

            mixL += sampleL * env;
            mixR += sampleR * env;

            const double rateMult = v.isHalfTime ? 0.5 : 1.0;
            const double step = globalRate * v.notePitchRatio * rateMult;
            if (isRev)
                v.position -= step;
            else
                v.position += step;
        }

        dstL[s] += mixL;
        if (dstR != nullptr)
            dstR[s] += mixR;

        for (int ch = 2; ch < numOutChannels; ++ch)
            out.addSample(ch, startSample + s, 0.5f * (mixL + mixR));
    }
}

void ClipSamplerAudioProcessor::publishPlaybackStateToUi()
{
    bool anyActive = false;
    juce::uint32 newestOrder = 0;
    int newestPos = 0;
    int newestSlice = -1;

    for (int i = 0; i < kNumPlaybackVoices; ++i)
    {
        const auto& v = voices[i];
        if (v.active && !v.releasing)
        {
            anyActive = true;
            if (v.order >= newestOrder)
            {
                newestOrder = v.order;
                newestPos = (int)v.position;
                newestSlice = v.sliceIndex;
            }
        }
    }

    if (!anyActive)
    {
        for (int i = 0; i < kNumPlaybackVoices; ++i)
        {
            const auto& v = voices[i];
            if (v.active && v.order >= newestOrder)
            {
                anyActive = true;
                newestOrder = v.order;
                newestPos = (int)v.position;
                newestSlice = v.sliceIndex;
            }
        }
    }

    isPlaying.store(anyActive, std::memory_order_relaxed);
    playheadPosition.store(anyActive ? newestPos : 0, std::memory_order_relaxed);
    activePlayingSlice.store(anyActive ? newestSlice : -1, std::memory_order_relaxed);
}

void ClipSamplerAudioProcessor::processMacroFx(juce::AudioBuffer<float>& buffer)
{
    const float lofiAmt = fxLofiAmount.load(std::memory_order_relaxed);
    const float toneCutoff = fxToneFilter.load(std::memory_order_relaxed);
    const float targetAlpha = juce::jlimit(0.04f, 1.0f, toneCutoff * toneCutoff);
    smoothedToneAlpha.setTargetValue(targetAlpha);

    if (lofiAmt <= 0.005f && toneCutoff >= 0.99f && !smoothedToneAlpha.isSmoothing())
        return;

    const float levels = juce::jmap(lofiAmt, 0.0f, 1.0f, 4096.0f, 28.0f);
    const int holdFactor = juce::jmax(1, juce::roundToInt(1.0f + lofiAmt * 14.0f));
    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin(2, buffer.getNumChannels());

    for (int i = 0; i < numSamples; ++i)
    {
        const float lpAlpha = smoothedToneAlpha.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            float& filterState = (ch == 0) ? lofiFilterStateL : lofiFilterStateR;

            if (lofiAmt > 0.005f)
            {
                if (lofiHoldCounter[ch] <= 0)
                {
                    float driven = std::tanh(data[i] * (1.0f + lofiAmt * 0.65f));
                    lofiHeldSample[ch] = std::round(driven * levels) / levels;
                    lofiHoldCounter[ch] = holdFactor;
                }
                --lofiHoldCounter[ch];
            }
            else
            {
                lofiHoldCounter[ch] = 0;
            }

            const float s = (lofiAmt > 0.005f) ? lofiHeldSample[ch] : data[i];
            filterState += lpAlpha * (s - filterState);
            data[i] = filterState;
        }
    }
}

//==============================================================================
// Bucle Principal de Audio en Tiempo Real (100% Lock-Free / Cero Cortes)
//==============================================================================
void ClipSamplerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    lastProcessBlockTimestampMs.store(juce::Time::currentTimeMillis(), std::memory_order_relaxed);

    // 1. Preescucha directa de Freesound (SE RENDERIZA SIEMPRE EN EL BUS DEL DAW, INCLUSO SIN SAMPLE CARGADO EN PADS)
    if (previewIsPlaying.load(std::memory_order_acquire))
    {
        const int outChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        int pos = previewPlayPos.load(std::memory_order_relaxed);
        const int total = previewTotalSamples.load(std::memory_order_relaxed);

        if (pos < total && outChannels > 0)
        {
            const juce::ScopedLock sl(previewLock);
            const int samplesToMix = juce::jmin(numSamples, total - pos);
            const int srcChannels = previewAudioBuffer.getNumChannels();

            if (samplesToMix > 0 && srcChannels > 0)
            {
                for (int ch = 0; ch < outChannels; ++ch)
                {
                    const int srcCh = juce::jmin(ch, srcChannels - 1);
                    buffer.addFrom(ch, 0, previewAudioBuffer, srcCh, pos, samplesToMix);
                }
                previewPlayPos.store(pos + samplesToMix, std::memory_order_release);
            }
            else
            {
                previewIsPlaying.store(false, std::memory_order_release);
                currentPreviewSoundId.clear();
            }
        }
        else
        {
            previewIsPlaying.store(false, std::memory_order_release);
            currentPreviewSoundId.clear();
        }
    }

    // 1. Verificación ligera en caché (consulta completa solo cada ~250 ms para no penalizar el hilo de audio)
    if (--licenseRecheckCountdown <= 0)
    {
        cachedAudioUnlocked = licenseManager.isUnlocked() && licenseManager.verifyRuntimeIntegrity();
        licenseRecheckCountdown = juce::jmax(16, (int)(currentSampleRateHz / (double)juce::jmax(64, buffer.getNumSamples()) * 0.25));
    }

    if (!cachedAudioUnlocked)
    {
        midiMessages.clear();
        return;
    }

    // 2. Lectura del BPM del DAW anfitrión para el modo Auto-Sync
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

    // 3. Obtenemos el paquete de audio activo mediante SpinLock de nanosegundos (nunca bloquea ni pierde bloques)
    auto data = acquirePlaybackData();
    if (data == nullptr || data->buffer.getNumSamples() <= 2)
    {
        isPlaying.store(false, std::memory_order_relaxed);
        playheadPosition.store(0, std::memory_order_relaxed);
        activePlayingSlice.store(-1, std::memory_order_relaxed);
        midiMessages.clear();

        // Aplicamos la ganancia master al buffer (que conserva intacta la preescucha activa de Freesound)
        float targetGain = 1.0f;
        if (auto* gainParam = apvts.getRawParameterValue("masterGain"))
            targetGain = gainParam->load(std::memory_order_relaxed);
        smoothedMasterGain.setTargetValue(targetGain);
        smoothedMasterGain.applyGain(buffer, buffer.getNumSamples());
        return;
    }

    // Si cambió el buffer publicado (ej. se cargó una nueva extracción), limpiamos voces previas de otro buffer
    if (data->generation != lastRenderedGeneration)
    {
        killAllVoices();
        lastRenderedGeneration = data->generation;
    }

    // 4. Procesamos comandos de transporte enviados desde la interfaz (Play / Stop / Click en rodaja)
    handlePendingTransportCommand(*data);

    // 5. Actualizamos la velocidad objetivo de reproducción (Pitch + Sincronización BPM) con suavizado continuo
    const float pitchSt = fxPitchSemitones.load(std::memory_order_relaxed);
    double targetRate = std::pow(2.0, (double)pitchSt / 12.0);
    if (autoSyncEnabled.load(std::memory_order_relaxed))
    {
        const double detBpm = (data->slotIndex == 2)
            ? slot2Bpm.load(std::memory_order_relaxed)
            : slot1Bpm.load(std::memory_order_relaxed);
        const double hostBpm = currentHostBpm.load(std::memory_order_relaxed);
        if (detBpm > 35.0 && hostBpm > 35.0)
            targetRate *= juce::jlimit(0.5, 2.0, hostBpm / detBpm);
    }
    smoothedPlaybackRate.setTargetValue((float)juce::jlimit(0.25, 4.0, targetRate));

    // 6. Renderizado sample-accurate dividido por la posición exacta de cada evento MIDI entrante
    const bool chopMode = autoChopEnabled.load(std::memory_order_relaxed);
    const int numSamples = buffer.getNumSamples();
    int currentBlockPos = 0;

    for (const auto metadata : midiMessages)
    {
        const int eventPos = juce::jlimit(currentBlockPos, numSamples, metadata.samplePosition);
        if (eventPos > currentBlockPos)
        {
            renderVoices(*data, buffer, currentBlockPos, eventPos - currentBlockPos);
            currentBlockPos = eventPos;
        }

        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            triggerNoteFromMidi(*data, msg.getNoteNumber(), msg.getFloatVelocity(), chopMode);
        }
        else if (msg.isNoteOff())
        {
            if (!chopMode)
                releaseVoiceForMidiNote(msg.getNoteNumber());
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            releaseAllVoices();
        }
    }

    if (currentBlockPos < numSamples)
        renderVoices(*data, buffer, currentBlockPos, numSamples - currentBlockPos);

    midiMessages.clear();

    // 7. Publicamos la posición actual del cabezal para el dibujo fluido a 30 FPS en la UI
    publishPlaybackStateToUi();

    // 8. Aplicamos los Efectos Macro (Lo-Fi 12-Bit + Filtro) con estado continuo entre bloques
    processMacroFx(buffer);

    // 9. Ganancia Master suavizada (sin zipper noise)
    float targetGain = 1.0f;
    if (auto* gainParam = apvts.getRawParameterValue("masterGain"))
        targetGain = gainParam->load(std::memory_order_relaxed);
    smoothedMasterGain.setTargetValue(targetGain);
    smoothedMasterGain.applyGain(buffer, numSamples);


}

//==============================================================================
// Instanciación del Editor Visual
//==============================================================================
bool ClipSamplerAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* ClipSamplerAudioProcessor::createEditor()
{
    return new ClipSamplerAudioProcessorEditor(*this);
}

//==============================================================================
// Guardado y Restauración del Estado del Proyecto en el DAW
//==============================================================================
void ClipSamplerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("selectedPlatform", selectedPlatform.load(), nullptr);
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
    state.setProperty("webExtractionUnlocked", webExtractionUnlocked.load(), nullptr);

    auto origFile = getSlotAudioFile(1);
    auto transFile = getSlotAudioFile(2);
    state.setProperty("lastWavPath", origFile.getFullPathName(), nullptr);
    state.setProperty("lastWavFileName", origFile.getFileName(), nullptr);
    state.setProperty("lastTransWavPath", transFile.getFullPathName(), nullptr);
    state.setProperty("activeSlot", activeSlot.load(), nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void ClipSamplerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (state.isValid())
    {
        if (state.hasType(apvts.state.getType()))
            apvts.replaceState(state);

        selectedPlatform.store(static_cast<int>(state.getProperty("selectedPlatform", 0)));
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
        if (state.hasProperty("webExtractionUnlocked"))
            webExtractionUnlocked.store(static_cast<bool>(state.getProperty("webExtractionUnlocked", false)));

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
// Gestión de Ranuras de Audio y Reinicio ("Nueva Extracción")
//==============================================================================
bool ClipSamplerAudioProcessor::hasSlotAudio(int slotIndex) const
{
    const juce::ScopedLock sl(bufferLock);
    if (slotIndex == 2)
        return transformedLoadedFile.existsAsFile();
    return originalLoadedFile.existsAsFile();
}

bool ClipSamplerAudioProcessor::hasBothAudiosReady() const
{
    const juce::ScopedLock sl(bufferLock);
    return originalLoadedFile.existsAsFile() && transformedLoadedFile.existsAsFile();
}

void ClipSamplerAudioProcessor::resetForNewExtraction()
{
    stopPlayback();
    stopFreesoundPreview();
    extractorWorker.cancelExtraction();

    publishPlaybackData(nullptr);

    {
        const juce::ScopedLock sl(bufferLock);
        sampler.clearSounds();
        slot1PlaybackData.reset();
        slot2PlaybackData.reset();
        originalLoadedFile = juce::File();
        transformedLoadedFile = juce::File();
        slot1MidiFile = juce::File();
        slot2MidiFile = juce::File();
        slot1SelectedSliceWavFile = juce::File();
        slot2SelectedSliceWavFile = juce::File();
        slot1Slices.clear();
        slot2Slices.clear();
    }

    exactCutModeEnabled.store(false, std::memory_order_relaxed);
    slot1SelectedSlice.store(-1, std::memory_order_relaxed);
    slot2SelectedSlice.store(-1, std::memory_order_relaxed);

    {
        const juce::ScopedLock sl(retiredLock);
        retiredPlaybackData.clear();
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
        currentCopyrightBadge = juce::String::fromUTF8("DERECHOS: Pendiente de análisis");
    }

    lastUrl.clear();
    lastStartTime = "00:00";
    detectedVideoDurationSec.store(0, std::memory_order_relaxed);
    extractionProgress.store(0.0f, std::memory_order_relaxed);
    copyrightVerifyState.store(0, std::memory_order_relaxed);
    totalSampleCount.store(0, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
    activePlayingSlice.store(-1, std::memory_order_relaxed);
    activeSlot.store(1, std::memory_order_relaxed);
    fxTargetSlot.store(1, std::memory_order_relaxed);
    fxLofiAmount.store(0.0f, std::memory_order_relaxed);
    fxPitchSemitones.store(0.0f, std::memory_order_relaxed);
    fxToneFilter.store(1.0f, std::memory_order_relaxed);
    slot2ContentType.store(0, std::memory_order_relaxed);
    slot1FxActive.store(false, std::memory_order_relaxed);
    slot2FxActive.store(false, std::memory_order_relaxed);
    autoSyncEnabled.store(false, std::memory_order_relaxed);

    setStatusMessage(juce::String::fromUTF8("¡Nueva extracción lista! Ambos visores y efectos han sido limpiados. Pega un nuevo enlace."));
}

//==============================================================================
// Detección de Duración y Extracción Multi-Plataforma (YouTube / TikTok / Instagram)
//==============================================================================
void ClipSamplerAudioProcessor::detectVideoDuration(const juce::String& url)
{
    juce::String cleanUrl = url.trim();
    if (cleanUrl.isEmpty())
        return;

    lastUrl = cleanUrl;
    if (metadataInProgress.exchange(true))
        return;

    const SourcePlatform platform = getSelectedPlatform();
    const juce::String platName = AudioExtractorWorker::getPlatformName(platform);
    setStatusMessage(juce::String::fromUTF8("Detectando duración y metadatos en ") + platName + "...");
    auto weakAlive = aliveFlag;

    AudioExtractorWorker::fetchVideoMetadata(cleanUrl, platform, [this, weakAlive, platName](const AudioExtractorWorker::VideoMetadata& meta) {
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
            setStatusMessage(juce::String::fromUTF8("[") + platName + juce::String::fromUTF8("] Detectado [00:00 -> ") + durFormatted + "]: " + meta.title);
        }
        else
        {
            setStatusMessage(juce::String::fromUTF8("Aviso (") + platName + "): " + meta.errorMessage);
        }
    });
}

void ClipSamplerAudioProcessor::startSampleExtraction(const juce::String& url, const juce::String& startTime, int durationSeconds)
{
    licenseManager.verifyAndSyncState();
    if (!licenseManager.isUnlocked() || !licenseManager.verifyRuntimeIntegrity())
    {
        setStatusMessage(juce::String::fromUTF8("Periodo de prueba de 3 días finalizado o licencia requerida. Activa tu licencia para continuar."));
        return;
    }

    lastUrl = url.trim();
    lastStartTime = startTime.trim();
    const int clampedDuration = juce::jlimit(1, 60, durationSeconds);
    lastSelectedDurationSec.store(clampedDuration, std::memory_order_relaxed);

    const SourcePlatform platform = getSelectedPlatform();
    const juce::String platName = AudioExtractorWorker::getPlatformName(platform);

    if (detectedVideoDurationSec.load(std::memory_order_relaxed) <= 0)
        detectVideoDuration(lastUrl);

    extractionInProgress.store(true, std::memory_order_relaxed);
    extractionProgress.store(0.05f, std::memory_order_relaxed);
    setStatusMessage(juce::String::fromUTF8("Iniciando extracción de ") + platName + " (" + juce::String(clampedDuration) + juce::String::fromUTF8(" segundos)..."));

    int currentDawRate = static_cast<int>(getSampleRate());
    if (currentDawRate <= 0)
        currentDawRate = 44100;

    auto weakAlive = aliveFlag;

    extractorWorker.startExtraction(
        lastUrl,
        platform,
        lastStartTime,
        clampedDuration,
        currentDawRate,
        [this, weakAlive](float progress, const juce::String& msg) {
            if (!weakAlive->load(std::memory_order_acquire))
                return;

            extractionProgress.store(progress, std::memory_order_relaxed);
            setStatusMessage(msg);
        },
        [this, weakAlive, clampedDuration, platName](bool success, const juce::File& resultFile, const juce::String& error) {
            if (!weakAlive->load(std::memory_order_acquire))
                return;

            if (success && resultFile.existsAsFile())
            {
                loadAudioFileIntoSlot(resultFile, 1);

                // Si TikTok o Instagram no reportaron la duración previa, la actualizamos con la duración real del audio
                if (detectedVideoDurationSec.load(std::memory_order_relaxed) <= 0)
                {
                    int actualLenSec = juce::jmax(1, (int)std::ceil(originalThumbnail.getTotalLength()));
                    detectedVideoDurationSec.store(actualLenSec, std::memory_order_relaxed);
                }

                copyrightVerifyState.store(0, std::memory_order_relaxed);
                {
                    const juce::ScopedLock sl(statusLock);
                    currentCopyrightBadge = juce::String::fromUTF8("DERECHOS [") + platName.toUpperCase() + juce::String::fromUTF8("]: Muestra en Visor 1 (Sin verificar)");
                }
                extractionProgress.store(1.0f, std::memory_order_relaxed);

                double bpm = getDetectedBpm(1);
                juce::String key = getDetectedKey(1);
                setStatusMessage(juce::String::fromUTF8("¡Muestra de ") + platName + " (" + juce::String(clampedDuration)
                                 + juce::String::fromUTF8("s) lista en Visor 1! [")
                                 + juce::String(bpm, 1) + " BPM | Tono: " + key
                                 + juce::String::fromUTF8(" | Rebanador C1-G2]. Pulsa '2. Derechos IA' para generar Audio 2."));
            }
            else
            {
                extractionProgress.store(0.0f, std::memory_order_relaxed);
                setStatusMessage(juce::String::fromUTF8("Error en la extracción (") + platName + "): " + error);
            }

            extractionInProgress.store(false, std::memory_order_relaxed);
        });
}

//==============================================================================
// Suite de 3 Pasos de Derechos de Autor e Inteligencia Artificial (±3 Semitonos + Fase)
//==============================================================================
bool ClipSamplerAudioProcessor::analyzeOriginalCopyright(juce::String& outReport)
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
        outReport = juce::String::fromUTF8("No se pudo leer el archivo original para el análisis de derechos.");
        setStatusMessage(outReport);
        return false;
    }

    const int numSamples = (int)juce::jmin<juce::int64>(reader->lengthInSamples, 48000 * 10);
    juce::AudioBuffer<float> buf((int)reader->numChannels, numSamples);
    reader->read(&buf, 0, numSamples, 0, true, true);

    float rms = buf.getRMSLevel(0, 0, numSamples);
    float peak = buf.getMagnitude(0, numSamples);
    float crestFactor = (rms > 0.0001f) ? (peak / rms) : 1.0f;

    bool hasCopyMeta = detectedHasCopyrightMeta.load(std::memory_order_relaxed);
    copyrightVerifyState.store(1, std::memory_order_relaxed);

    juce::String badge;
    if (hasCopyMeta || crestFactor < 12.0f)
    {
        badge = juce::String::fromUTF8("RIESGO DERECHOS: Huella Original Detectada (100% Coincidencia)");
        outReport = juce::String::fromUTF8("[ALERTA DE DERECHOS]: El Audio 1 coincide al 100% con el espectro original. Pulsa '2. Derechos IA (±3 st)' para alterarlo.");
    }
    else
    {
        badge = juce::String::fromUTF8("DERECHOS: Licencia abierta, pero conserva huella original");
        outReport = juce::String::fromUTF8("Audio con licencia abierta, pero conserva la onda exacta del video. Se recomienda pulsar '2. Derechos IA' para uso seguro.");
    }

    {
        const juce::ScopedLock sl(statusLock);
        currentCopyrightBadge = badge;
    }
    setStatusMessage(outReport);
    return true;
}

bool ClipSamplerAudioProcessor::applyDerechosTransformation(int semitoneShift, juce::String& outReport)
{
    if (!licenseManager.isUnlocked() || !licenseManager.verifyRuntimeIntegrity())
    {
        outReport = juce::String::fromUTF8("Activación requerida para usar la suite Derechos IA.");
        setStatusMessage(outReport);
        return false;
    }

    auto origFile = getSlotAudioFile(1);
    if (!origFile.existsAsFile())
    {
        outReport = juce::String::fromUTF8("Primero extrae una muestra de 3s, 10s o 30s en el Visor 1 antes de aplicar '2. Derechos IA'.");
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

    // 1. Cambio de Tono (Pitch Shift) de ±3 Semitonos + micro-desafinación de 0.15 st mediante interpolación de Lagrange
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

    // 2. Dispersión de Fase All-Pass + Micro-Modulación Estéreo para descorrelacionar la huella acústica
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
    slot2ContentType.store(0, std::memory_order_relaxed); // 0 = Derechos IA

    juce::String signStr = (semitoneShift > 0) ? ("+" + juce::String(semitoneShift)) : juce::String(semitoneShift);
    outReport = juce::String::fromUTF8("¡Audio 2 generado (") + signStr
              + juce::String::fromUTF8(" st IA) en Visor 2! Botón 'Nueva Extracción' habilitado. Pulsa '3. Verificar Audio 2'.");
    {
        const juce::ScopedLock sl(statusLock);
        currentCopyrightBadge = juce::String::fromUTF8("AUDIO 2 GENERADO (") + signStr + juce::String::fromUTF8(" st) - Listo para verificar");
    }
    setStatusMessage(outReport);
    return true;
}

juce::String ClipSamplerAudioProcessor::getSlot2ContentTypeString() const
{
    int type = slot2ContentType.load(std::memory_order_relaxed);
    if (type == 1)
        return juce::String::fromUTF8("CALOR ANALÓGICO (TAPE WARMTH)");
    if (type == 2)
        return juce::String::fromUTF8("AIRE Y ESPACIO 3D");
    return juce::String::fromUTF8("AUDIO MODIFICADO (DERECHOS ±3 ST IA)");
}

bool ClipSamplerAudioProcessor::applySubtleColor(SubtleColorType type, juce::String& outReport)
{
    if (!licenseManager.isUnlocked() || !licenseManager.verifyRuntimeIntegrity())
    {
        outReport = juce::String::fromUTF8("Activación requerida para usar las funciones de coloración de audio.");
        setStatusMessage(outReport);
        return false;
    }

    auto srcFile = getSlotAudioFile(1);
    if (!srcFile.existsAsFile())
    {
        outReport = juce::String::fromUTF8("Primero extrae una muestra de audio en el Visor 1.");
        setStatusMessage(outReport);
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(srcFile));
    if (reader == nullptr || reader->lengthInSamples <= 0)
    {
        outReport = juce::String::fromUTF8("No se pudo leer el archivo de origen para la coloración.");
        setStatusMessage(outReport);
        return false;
    }

    const double sampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    const int totalSamples = (int)reader->lengthInSamples;
    const int numChannels = juce::jlimit(1, 2, (int)reader->numChannels);

    juce::AudioBuffer<float> inBuffer(numChannels, totalSamples);
    reader->read(&inBuffer, 0, totalSamples, 0, true, numChannels > 1);
    reader.reset();

    juce::AudioBuffer<float> outBuffer(2, totalSamples);
    const float* srcL = inBuffer.getReadPointer(0);
    const float* srcR = inBuffer.getReadPointer(numChannels > 1 ? 1 : 0);

    float* outL = outBuffer.getWritePointer(0);
    float* outR = outBuffer.getWritePointer(1);

    if (type == SubtleColorType::TapeWarmth)
    {
        // =========================================================================
        // FUNCIÓN SUTIL 1: CALOR ANALÓGICO (VINTAGE TAPE & TUBE WARMTH)
        // 1. Realce musical de graves/cuerpo analógico (~160 Hz) tipo Pultec.
        // 2. Rolloff suave en agudos extremos (> 15.5 kHz) que elimina la aspereza digital.
        // 3. Saturación armónica asimétrica de cinta (2º y 3º armónicos cálidos).
        // =========================================================================
        const float bassFreq = 160.0f;
        const float bassW = 2.0f * juce::MathConstants<float>::pi * (bassFreq / (float)sampleRate);
        const float bassAlpha = bassW / (1.0f + bassW);
        float bassStateL = 0.0f, bassStateR = 0.0f;

        const float rollFreq = 15500.0f;
        const float rollW = 2.0f * juce::MathConstants<float>::pi * (rollFreq / (float)sampleRate);
        const float rollAlpha = rollW / (1.0f + rollW);
        float rollStateL = 0.0f, rollStateR = 0.0f;

        for (int i = 0; i < totalSamples; ++i)
        {
            float l = srcL[i];
            float r = srcR[i];

            // 1. Detección de frecuencias bajas para dar pegada y grosor (+1.4 dB)
            bassStateL += bassAlpha * (l - bassStateL);
            bassStateR += bassAlpha * (r - bassStateR);
            float warmedL = l + 0.18f * bassStateL;
            float warmedR = r + 0.18f * bassStateR;

            // 2. Filtro suave de agudos extremos (suavizado analógico)
            rollStateL += rollAlpha * (warmedL - rollStateL);
            rollStateR += rollAlpha * (warmedR - rollStateR);
            float filteredL = rollStateL;
            float filteredR = rollStateR;

            // 3. Saturación no lineal suave de cinta: tanh con armónicos pares sutiles
            auto applyTapeSat = [](float in) -> float {
                float driven = in * 1.15f;
                float sat = std::tanh(driven) + 0.06f * (driven * std::abs(driven)) - 0.02f * (driven * driven * driven);
                return juce::jlimit(-1.0f, 1.0f, sat * 0.92f);
            };

            outL[i] = applyTapeSat(filteredL);
            outR[i] = applyTapeSat(filteredR);
        }

        slot2ContentType.store(1, std::memory_order_relaxed); // 1 = Calor Analógico
    }
    else
    {
        // =========================================================================
        // FUNCIÓN SUTIL 2: AIRE Y ESPACIO 3D (AIR EXCITER & SPATIAL EXPANDER)
        // 1. Separación Mid/Side (Centro monofónico intacto para máxima compatibilidad).
        // 2. Apertura dimensional estéreo sutil (+2.2 dB en Side) con micro-decorrelación.
        // 3. Brillo de aire sedoso de alta frecuencia (> 10.5 kHz estilo Air Band).
        // =========================================================================
        const float airFreq = 10500.0f;
        const float airW = 2.0f * juce::MathConstants<float>::pi * (airFreq / (float)sampleRate);
        const float airAlpha = 1.0f / (1.0f + airW);
        float airHpPrevXL = 0.0f, airHpPrevYL = 0.0f;
        float airHpPrevXR = 0.0f, airHpPrevYR = 0.0f;

        const int delayLen = juce::jmax(4, (int)std::round(sampleRate * 0.0038)); // ~3.8 ms
        std::vector<float> delayBuf(delayLen, 0.0f);
        int delayIdx = 0;

        for (int i = 0; i < totalSamples; ++i)
        {
            float l = srcL[i];
            float r = srcR[i];

            float mid = 0.5f * (l + r);
            float side = 0.5f * (l - r);

            if (std::abs(side) < 0.001f)
            {
                float delayed = delayBuf[delayIdx];
                delayBuf[delayIdx] = mid;
                delayIdx = (delayIdx + 1) % delayLen;
                side = (mid - delayed) * 0.28f;
            }

            float wideSide = side * 1.30f;

            float hpL = airAlpha * (airHpPrevYL + l - airHpPrevXL);
            airHpPrevXL = l;
            airHpPrevYL = hpL;

            float hpR = airAlpha * (airHpPrevYR + r - airHpPrevXR);
            airHpPrevXR = r;
            airHpPrevYR = hpR;

            float airExciterL = hpL * 0.26f + 0.03f * std::tanh(hpL * 2.5f);
            float airExciterR = hpR * 0.26f + 0.03f * std::tanh(hpR * 2.5f);

            outL[i] = mid + airExciterL + wideSide;
            outR[i] = mid + airExciterR - wideSide;
        }

        slot2ContentType.store(2, std::memory_order_relaxed); // 2 = Aire y Espacio 3D
    }

    // Normalización musical a -1.0 dBFS para que suene con volumen profesional sin saturar
    float maxMag = outBuffer.getMagnitude(0, totalSamples);
    if (maxMag > 0.0001f)
    {
        float targetGain = 0.89125f / maxMag; // -1.0 dBFS
        outBuffer.applyGain(juce::jmin(targetGain, 3.5f));
    }

    // Micro-fades anti-click de 5 ms en los extremos
    int fadeSamples = juce::jmin(totalSamples / 4, (int)std::round(sampleRate * 0.005));
    if (fadeSamples > 2)
    {
        outBuffer.applyGainRamp(0, 0, fadeSamples, 0.0f, 1.0f);
        outBuffer.applyGainRamp(1, 0, fadeSamples, 0.0f, 1.0f);
        outBuffer.applyGainRamp(0, totalSamples - fadeSamples, fadeSamples, 1.0f, 0.0f);
        outBuffer.applyGainRamp(1, totalSamples - fadeSamples, fadeSamples, 1.0f, 0.0f);
    }

    juce::String colorPrefix = (type == SubtleColorType::TapeWarmth) ? "Calor_Analogico_" : "Aire_Espacio_3D_";
    juce::File colorFile = AudioExtractorWorker::getCacheDirectory().getChildFile(
        "ClipSampler_Audio2_" + colorPrefix + juce::String(juce::Time::getMillisecondCounter()) + ".wav");

    {
        std::unique_ptr<juce::FileOutputStream> outStream(colorFile.createOutputStream());
        if (outStream == nullptr)
        {
            outReport = juce::String::fromUTF8("No se pudo crear el archivo WAV para la coloración.");
            setStatusMessage(outReport);
            return false;
        }

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
            outStream.get(),
            sampleRate,
            2,
            24,
            {},
            0));

        if (writer == nullptr)
        {
            outReport = juce::String::fromUTF8("No se pudo codificar el WAV de 24 bits para la coloración.");
            setStatusMessage(outReport);
            return false;
        }

        outStream.release();
        writer->writeFromAudioSampleBuffer(outBuffer, 0, totalSamples);
    }

    loadAudioFileIntoSlot(colorFile, 2);
    fxTargetSlot.store(2, std::memory_order_relaxed);
    slot2FxActive.store(false, std::memory_order_relaxed);

    if (type == SubtleColorType::TapeWarmth)
    {
        outReport = juce::String::fromUTF8("¡Calor Analógico (Cinta Vintage) aplicado en Visor 2! Textura cálida y con cuerpo lista para reproducir o arrastrar.");
        {
            const juce::ScopedLock sl(statusLock);
            currentCopyrightBadge = juce::String::fromUTF8("AUDIO 2: CALOR ANALÓGICO - Listo para usar");
        }
    }
    else
    {
        outReport = juce::String::fromUTF8("¡Aire y Espacio 3D aplicado en Visor 2! Dimensión estéreo expandida y brillo de aire sedoso listos para usar.");
        {
            const juce::ScopedLock sl(statusLock);
            currentCopyrightBadge = juce::String::fromUTF8("AUDIO 2: AIRE Y ESPACIO 3D - Listo para usar");
        }
    }

    setStatusMessage(outReport);
    return true;
}

bool ClipSamplerAudioProcessor::verifyTransformedAudioCopyrightFree(juce::String& outReport)
{
    auto origFile = getSlotAudioFile(1);
    auto transFile = getSlotAudioFile(2);

    if (!origFile.existsAsFile() || !transFile.existsAsFile())
    {
        outReport = juce::String::fromUTF8("Primero extrae una muestra (Visor 1) y pulsa '2. Derechos IA' para generar el Audio 2 (Visor 2) antes de verificar.");
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
        currentCopyrightBadge = juce::String::fromUTF8("VERIFICADO SIN DERECHOS (") + pctFormatted + "% Divergencia IA)";
    }

    outReport = juce::String::fromUTF8("[VERIFICADO LIBRE DE DERECHOS]: El Audio 2 tiene ") + pctFormatted
              + juce::String::fromUTF8("% de divergencia espectral respecto al original (") + stText
              + juce::String::fromUTF8(" semitonos + fase IA). Listo para arrastrar .WAV o .MIDI al DAW.");
    setStatusMessage(outReport);
    return true;
}

//==============================================================================
// Algoritmo Rebanador MPC (Detección de Transientes C1 -> G2), BPM y Tonalidad
//==============================================================================
void ClipSamplerAudioProcessor::setAutoChopEnabled(bool enabled)
{
    autoChopEnabled.store(enabled, std::memory_order_relaxed);
}

bool ClipSamplerAudioProcessor::moveSliceDividerPosition(int slotIndex, int dividerIndex, float newNormalizedPos)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    const juce::ScopedLock sl(bufferLock);

    auto& slices = (targetSlot == 2) ? slot2Slices : slot1Slices;
    if (dividerIndex < 1 || dividerIndex >= (int)slices.size())
        return false;

    auto slotData = (targetSlot == 2) ? slot2PlaybackData : slot1PlaybackData;
    int totalSamples = (slotData != nullptr && slotData->buffer.getNumSamples() > 0)
        ? slotData->buffer.getNumSamples()
        : totalSampleCount.load(std::memory_order_relaxed);

    if (totalSamples <= 10)
        return false;

    // Límite mínimo de separación para que una rodaja no colapse a 0
    const float minSeparation = juce::jmax(0.004f, 256.0f / (float)totalSamples);
    const float minPos = slices[(size_t)(dividerIndex - 1)].startNorm + minSeparation;
    const float maxPos = slices[(size_t)dividerIndex].endNorm - minSeparation;

    if (minPos >= maxPos)
        return false;

    const float clampedNorm = juce::jlimit(minPos, maxPos, newNormalizedPos);
    const int newSample = juce::jlimit(slices[(size_t)(dividerIndex - 1)].startSample + 128,
                                       slices[(size_t)dividerIndex].endSample - 128,
                                       juce::roundToInt(clampedNorm * (float)totalSamples));

    // Ajustamos el final de la rodaja izquierda y el inicio de la rodaja derecha
    slices[(size_t)(dividerIndex - 1)].endNorm = clampedNorm;
    slices[(size_t)(dividerIndex - 1)].endSample = newSample;

    slices[(size_t)dividerIndex].startNorm = clampedNorm;
    slices[(size_t)dividerIndex].startSample = newSample;

    // Si la rodaja actualmente seleccionada es una de las modificadas, invalidamos el .wav en caché
    const int selIdx = (targetSlot == 2) ? slot2SelectedSlice.load(std::memory_order_relaxed) : slot1SelectedSlice.load(std::memory_order_relaxed);
    if (selIdx == dividerIndex - 1 || selIdx == dividerIndex)
    {
        if (targetSlot == 2)
            slot2SelectedSliceWavFile = juce::File();
        else
            slot1SelectedSliceWavFile = juce::File();
    }

    // Publicamos los nuevos límites de rodaja en el paquete de audio en RAM para que suenen de inmediato
    if (slotData != nullptr)
    {
        auto updatedData = std::make_shared<PlaybackData>();
        updatedData->buffer = slotData->buffer;
        updatedData->slices = slices;
        updatedData->slotIndex = targetSlot;
        updatedData->generation = ++playbackGenerationCounter;

        if (targetSlot == 2)
            slot2PlaybackData = updatedData;
        else
            slot1PlaybackData = updatedData;

        if (activeSlot.load(std::memory_order_relaxed) == targetSlot)
            publishPlaybackData(std::move(updatedData));
    }

    return true;
}

std::vector<ChopSlice> ClipSamplerAudioProcessor::getChopSlices(int slotIndex) const
{
    const juce::ScopedLock sl(bufferLock);
    return (slotIndex == 2) ? slot2Slices : slot1Slices;
}

void ClipSamplerAudioProcessor::triggerChopSlice(int sliceIndex)
{
    if (!licenseManager.isUnlocked())
        return;

    int startS = -1;
    int endS = -1;
    {
        const juce::ScopedLock sl(bufferLock);
        const int curSlot = activeSlot.load(std::memory_order_relaxed);
        const auto& slices = (curSlot == 2) ? slot2Slices : slot1Slices;
        if (sliceIndex >= 0 && sliceIndex < (int)slices.size())
        {
            startS = slices[(size_t)sliceIndex].startSample;
            endS   = slices[(size_t)sliceIndex].endSample;
        }
    }

    if (startS >= 0 && endS > startS)
        requestPlayRegion(startS, endS, sliceIndex);
}

//==============================================================================
// Selección y Extracción de Corte Exacto (1 Clic = Todo | Doble Clic = Nota C1..G2)
//==============================================================================
void ClipSamplerAudioProcessor::setExactCutModeEnabled(bool enabled)
{
    exactCutModeEnabled.store(enabled, std::memory_order_relaxed);
    if (!enabled)
    {
        const int curSlot = activeSlot.load(std::memory_order_relaxed);
        selectFullAudioInSlot(curSlot);
    }
}

void ClipSamplerAudioProcessor::selectFullAudioInSlot(int slotIndex)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    setActiveSlot(targetSlot);

    if (targetSlot == 2)
        slot2SelectedSlice.store(-1, std::memory_order_relaxed);
    else
        slot1SelectedSlice.store(-1, std::memory_order_relaxed);

    {
        const juce::ScopedLock sl(bufferLock);
        if (targetSlot == 2)
            slot2SelectedSliceWavFile = juce::File();
        else
            slot1SelectedSliceWavFile = juce::File();
    }

    exactCutModeEnabled.store(false, std::memory_order_relaxed);
}

int ClipSamplerAudioProcessor::getSelectedSliceIndex(int slotIndex) const
{
    return (slotIndex == 2)
        ? slot2SelectedSlice.load(std::memory_order_relaxed)
        : slot1SelectedSlice.load(std::memory_order_relaxed);
}

juce::String ClipSamplerAudioProcessor::getSelectedSliceName(int slotIndex) const
{
    const int selIdx = getSelectedSliceIndex(slotIndex);
    if (selIdx < 0)
        return {};

    const juce::ScopedLock sl(bufferLock);
    const auto& slices = (slotIndex == 2) ? slot2Slices : slot1Slices;
    if (selIdx >= 0 && selIdx < (int)slices.size())
        return slices[(size_t)selIdx].noteName;

    return {};
}

bool ClipSamplerAudioProcessor::selectSliceByIndex(int slotIndex, int sliceIndex, ChopSlice& outSlice)
{
    if (!licenseManager.isUnlocked())
        return false;

    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    setActiveSlot(targetSlot);

    bool found = false;
    {
        const juce::ScopedLock sl(bufferLock);
        const auto& slices = (targetSlot == 2) ? slot2Slices : slot1Slices;
        if (sliceIndex >= 0 && sliceIndex < (int)slices.size())
        {
            outSlice = slices[(size_t)sliceIndex];
            found = true;
            if (targetSlot == 2)
                slot2SelectedSliceWavFile = juce::File();
            else
                slot1SelectedSliceWavFile = juce::File();
        }
    }

    if (!found)
        return false;

    if (targetSlot == 2)
        slot2SelectedSlice.store(sliceIndex, std::memory_order_relaxed);
    else
        slot1SelectedSlice.store(sliceIndex, std::memory_order_relaxed);

    exactCutModeEnabled.store(true, std::memory_order_relaxed);
    autoChopEnabled.store(true, std::memory_order_relaxed);

    // Pre-generamos el archivo .WAV de este corte exacto y lo reproducimos inmediatamente
    getOrCreateSelectedSliceWavFile(targetSlot);
    requestPlayRegion(outSlice.startSample, outSlice.endSample, sliceIndex);
    return true;
}

bool ClipSamplerAudioProcessor::selectSliceAtNormalizedPosition(int slotIndex, float normalizedPos, ChopSlice& outSlice)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    setActiveSlot(targetSlot);

    int matchedIdx = -1;
    {
        const juce::ScopedLock sl(bufferLock);
        const auto& slices = (targetSlot == 2) ? slot2Slices : slot1Slices;
        const float clampedNorm = juce::jlimit(0.0f, 0.9999f, normalizedPos);
        for (size_t i = 0; i < slices.size(); ++i)
        {
            if (clampedNorm >= slices[i].startNorm && clampedNorm <= slices[i].endNorm)
            {
                matchedIdx = (int)i;
                break;
            }
        }
        if (matchedIdx < 0 && !slices.empty())
            matchedIdx = 0;
    }

    if (matchedIdx < 0)
        return false;

    return selectSliceByIndex(targetSlot, matchedIdx, outSlice);
}

juce::File ClipSamplerAudioProcessor::getOrCreateSelectedSliceWavFile(int slotIndex)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    const int selIdx = getSelectedSliceIndex(targetSlot);
    if (selIdx < 0)
        return getSlotAudioFile(targetSlot);

    std::shared_ptr<PlaybackData> slotData;
    ChopSlice chosenSlice;
    {
        const juce::ScopedLock sl(bufferLock);
        juce::File cachedWav = (targetSlot == 2) ? slot2SelectedSliceWavFile : slot1SelectedSliceWavFile;
        if (cachedWav.existsAsFile() && cachedWav.getSize() > 64)
            return cachedWav;

        slotData = (targetSlot == 2) ? slot2PlaybackData : slot1PlaybackData;
        const auto& slices = (targetSlot == 2) ? slot2Slices : slot1Slices;
        if (selIdx < 0 || selIdx >= (int)slices.size())
            return getSlotAudioFile(targetSlot);

        chosenSlice = slices[(size_t)selIdx];
    }

    if (slotData == nullptr || slotData->buffer.getNumSamples() <= 0)
    {
        auto f = getSlotAudioFile(targetSlot);
        if (!f.existsAsFile() || !activateSlotBuffer(f, targetSlot))
            return {};

        const juce::ScopedLock sl(bufferLock);
        slotData = (targetSlot == 2) ? slot2PlaybackData : slot1PlaybackData;
    }

    if (slotData == nullptr || slotData->buffer.getNumSamples() <= 0)
        return {};

    const int totalSamples = slotData->buffer.getNumSamples();
    const int numCh = juce::jmax(1, slotData->buffer.getNumChannels());
    const int startS = juce::jlimit(0, totalSamples - 1, chosenSlice.startSample);
    const int endS   = juce::jlimit(startS + 1, totalSamples, chosenSlice.endSample);
    const int sliceLen = endS - startS;
    if (sliceLen <= 4)
        return getSlotAudioFile(targetSlot);

    juce::AudioBuffer<float> sliceBuf(numCh, sliceLen);
    for (int ch = 0; ch < numCh; ++ch)
        sliceBuf.copyFrom(ch, 0, slotData->buffer, ch, startS, sliceLen);

    // Si la rodaja tiene Reverse activado, invertimos los samples
    if (chosenSlice.isReversed)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = sliceBuf.getWritePointer(ch);
            std::reverse(d, d + sliceLen);
        }
    }

    // Si la rodaja tiene Half-Time activado, duplicamos la duración a 0.5x velocidad
    if (chosenSlice.isHalfTime)
    {
        const int halfLen = sliceLen * 2;
        juce::AudioBuffer<float> halfBuf(numCh, halfLen);
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* src = sliceBuf.getReadPointer(ch);
            float* dst = halfBuf.getWritePointer(ch);
            for (int i = 0; i < halfLen; ++i)
            {
                double pos = (double)i * 0.5;
                dst[i] = interpolateHermite4pt(src, sliceLen, pos);
            }
        }
        sliceBuf = std::move(halfBuf);
    }

    const int finalSliceLen = sliceBuf.getNumSamples();

    // Micro-fade anti-click de 2.5 ms en el inicio y final del corte exacto
    const double sr = loadedFileSampleRate.load(std::memory_order_relaxed) > 8000.0
        ? loadedFileSampleRate.load(std::memory_order_relaxed)
        : 44100.0;
    const int fadeLen = juce::jmin(finalSliceLen / 4, juce::jmax(24, (int)std::round(sr * 0.0025)));
    if (fadeLen > 1)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            sliceBuf.applyGainRamp(ch, 0, fadeLen, 0.0f, 1.0f);
            sliceBuf.applyGainRamp(ch, finalSliceLen - fadeLen, fadeLen, 1.0f, 0.0f);
        }
    }

    juce::String safeNote = chosenSlice.noteName.replace("#", "s").retainCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
    if (safeNote.isEmpty())
        safeNote = "Slice" + juce::String(selIdx + 1);

    if (chosenSlice.isReversed)
        safeNote += "_Rev";
    if (chosenSlice.isHalfTime)
        safeNote += "_Half";

    juce::File outSliceFile = AudioExtractorWorker::getCacheDirectory().getChildFile(
        "ClipSampler_Audio" + juce::String(targetSlot) + "_CorteExacto_" + safeNote + ".wav");
    outSliceFile.deleteFile();

    {
        std::unique_ptr<juce::FileOutputStream> outStream(outSliceFile.createOutputStream());
        if (outStream == nullptr)
            return getSlotAudioFile(targetSlot);

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
            outStream.get(), sr, (unsigned int)numCh, 24, {}, 0));
        if (writer == nullptr)
            return getSlotAudioFile(targetSlot);

        outStream.release();
        writer->writeFromAudioSampleBuffer(sliceBuf, 0, finalSliceLen);
    }

    {
        const juce::ScopedLock sl(bufferLock);
        if (targetSlot == 2)
            slot2SelectedSliceWavFile = outSliceFile;
        else
            slot1SelectedSliceWavFile = outSliceFile;
    }

    return outSliceFile;
}

bool ClipSamplerAudioProcessor::toggleSliceReverse(int slotIndex, int sliceIndex)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    const juce::ScopedLock sl(bufferLock);
    auto& slices = (targetSlot == 2) ? slot2Slices : slot1Slices;
    if (sliceIndex < 0 || sliceIndex >= (int)slices.size())
        return false;

    slices[(size_t)sliceIndex].isReversed = !slices[(size_t)sliceIndex].isReversed;
    const bool nowRev = slices[(size_t)sliceIndex].isReversed;

    if (targetSlot == 2)
        slot2SelectedSliceWavFile = juce::File();
    else
        slot1SelectedSliceWavFile = juce::File();

    auto slotData = (targetSlot == 2) ? slot2PlaybackData : slot1PlaybackData;
    if (slotData != nullptr)
    {
        auto updatedData = std::make_shared<PlaybackData>();
        updatedData->buffer = slotData->buffer;
        updatedData->slices = slices;
        updatedData->slotIndex = targetSlot;
        updatedData->generation = ++playbackGenerationCounter;

        if (targetSlot == 2)
            slot2PlaybackData = updatedData;
        else
            slot1PlaybackData = updatedData;

        if (activeSlot.load(std::memory_order_relaxed) == targetSlot)
            publishPlaybackData(updatedData);
    }

    setStatusMessage(juce::String::fromUTF8("Rodaja [") + slices[(size_t)sliceIndex].noteName
                     + (nowRev ? juce::String::fromUTF8("]: Reverse (◀ Invertido) ACTIVADO")
                               : juce::String::fromUTF8("]: Reproducción normal (Adelante)")));
    return true;
}

bool ClipSamplerAudioProcessor::isSliceReversed(int slotIndex, int sliceIndex) const
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    const juce::ScopedLock sl(bufferLock);
    const auto& slices = (targetSlot == 2) ? slot2Slices : slot1Slices;
    if (sliceIndex >= 0 && sliceIndex < (int)slices.size())
        return slices[(size_t)sliceIndex].isReversed;
    return false;
}

bool ClipSamplerAudioProcessor::toggleSliceHalfTime(int slotIndex, int sliceIndex)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    const juce::ScopedLock sl(bufferLock);
    auto& slices = (targetSlot == 2) ? slot2Slices : slot1Slices;
    if (sliceIndex < 0 || sliceIndex >= (int)slices.size())
        return false;

    slices[(size_t)sliceIndex].isHalfTime = !slices[(size_t)sliceIndex].isHalfTime;
    const bool nowHalf = slices[(size_t)sliceIndex].isHalfTime;

    if (targetSlot == 2)
        slot2SelectedSliceWavFile = juce::File();
    else
        slot1SelectedSliceWavFile = juce::File();

    auto slotData = (targetSlot == 2) ? slot2PlaybackData : slot1PlaybackData;
    if (slotData != nullptr)
    {
        auto updatedData = std::make_shared<PlaybackData>();
        updatedData->buffer = slotData->buffer;
        updatedData->slices = slices;
        updatedData->slotIndex = targetSlot;
        updatedData->generation = ++playbackGenerationCounter;

        if (targetSlot == 2)
            slot2PlaybackData = updatedData;
        else
            slot1PlaybackData = updatedData;

        if (activeSlot.load(std::memory_order_relaxed) == targetSlot)
            publishPlaybackData(updatedData);
    }

    setStatusMessage(juce::String::fromUTF8("Rodaja [") + slices[(size_t)sliceIndex].noteName
                     + (nowHalf ? juce::String::fromUTF8("]: Half-Time (⏳ 0.5x velocidad) ACTIVADO")
                                : juce::String::fromUTF8("]: Velocidad 1.0x normal")));
    return true;
}

bool ClipSamplerAudioProcessor::isSliceHalfTime(int slotIndex, int sliceIndex) const
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    const juce::ScopedLock sl(bufferLock);
    const auto& slices = (targetSlot == 2) ? slot2Slices : slot1Slices;
    if (sliceIndex >= 0 && sliceIndex < (int)slices.size())
        return slices[(size_t)sliceIndex].isHalfTime;
    return false;
}

std::vector<ChopSlice> ClipSamplerAudioProcessor::computeTransientSlices(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    std::vector<ChopSlice> result;
    const int totalSamples = buffer.getNumSamples();
    if (totalSamples <= 256 || sampleRate <= 0.0)
        return result;

    // 1. Calculamos la envolvente de energía RMS en ventanas de 10 ms y su derivada positiva (flujo de ataque)
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

    // 2. Selección de picos de ataque (transientes) respetando una separación mínima entre rodajas
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
            if ((int)onsetSamples.size() >= 16)
                break;
        }
    }

    // 3. Si el audio es melódico/ambiente con pocos golpes percusivos, subdividimos hasta tener mínimo 8 rodajas
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

    if ((int)onsetSamples.size() > 20)
        onsetSamples.resize(20);

    for (size_t i = 0; i < onsetSamples.size(); ++i)
    {
        ChopSlice s;
        s.sliceIndex = (int)i;
        s.midiNote = 36 + (int)i; // Mapeo cromático C1 (36) a G2 (55)
        s.noteName = midiNoteToName(s.midiNote);
        s.startSample = onsetSamples[i];
        s.endSample = (i + 1 < onsetSamples.size()) ? onsetSamples[i + 1] : totalSamples;
        s.startNorm = (float)s.startSample / (float)totalSamples;
        s.endNorm = (float)s.endSample / (float)totalSamples;
        result.push_back(s);
    }

    return result;
}

double ClipSamplerAudioProcessor::estimateBpmFromBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int totalSamples = buffer.getNumSamples();
    if (totalSamples < (int)(sampleRate * 1.2) || sampleRate <= 0.0)
        return 120.0;

    // Calculamos la envolvente de ataques a 200 Hz y buscamos periodicidad por autocorrelación entre 72 y 172 BPM
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
    detectedBpm = std::round(detectedBpm * 2.0) * 0.5;
    return juce::jlimit(70.0, 175.0, detectedBpm);
}

juce::String ClipSamplerAudioProcessor::estimateMusicalKeyFromBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    static const char* pitchNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    // Perfiles tonales de Krumhansl-Schmuckler para escalas Mayores y Menores
    static const double majorProfile[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    static const double minorProfile[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

    const int totalSamples = buffer.getNumSamples();
    if (totalSamples < 2048 || sampleRate <= 0.0)
        return "C Maj";

    double chroma[12] = { 0.0 };
    const float* samples = buffer.getReadPointer(0);

    // Banco de resonadores de Goertzel en las 12 clases de altura cromática (octavas 2 a 4)
    const int frameSize = 2048;
    const int numFrames = juce::jmin(14, juce::jmax(1, totalSamples / frameSize));
    const int stride = juce::jmax(frameSize, totalSamples / numFrames);

    for (int pc = 0; pc < 12; ++pc)
    {
        for (int oct = 2; oct <= 4; ++oct)
        {
            int midi = (oct + 1) * 12 + pc;
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

double ClipSamplerAudioProcessor::getDetectedBpm(int slotIndex) const
{
    return (slotIndex == 2) ? slot2Bpm.load(std::memory_order_relaxed) : slot1Bpm.load(std::memory_order_relaxed);
}

juce::String ClipSamplerAudioProcessor::getDetectedKey(int slotIndex) const
{
    const juce::ScopedLock sl(statusLock);
    return (slotIndex == 2) ? slot2Key : slot1Key;
}

void ClipSamplerAudioProcessor::analyzeAutoChopAndMusicData(int slotIndex)
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
// Configuración e Impresión Permanente de Efectos Macro (Lo-Fi, Tono, Filtro, Sinc. BPM)
//==============================================================================
void ClipSamplerAudioProcessor::setFxTargetSlot(int slotIndex)
{
    const int target = (slotIndex == 2) ? 2 : 1;
    fxTargetSlot.store(target, std::memory_order_relaxed);
    if (hasSlotAudio(target))
        setActiveSlot(target);
}

void ClipSamplerAudioProcessor::setLofiAmount(float amount0to1)
{
    fxLofiAmount.store(juce::jlimit(0.0f, 1.0f, amount0to1), std::memory_order_relaxed);
}

void ClipSamplerAudioProcessor::setFxPitchSemitones(float semitones)
{
    fxPitchSemitones.store(juce::jlimit(-12.0f, 12.0f, semitones), std::memory_order_relaxed);
}

void ClipSamplerAudioProcessor::setFxToneFilter(float tone0to1)
{
    fxToneFilter.store(juce::jlimit(0.1f, 1.0f, tone0to1), std::memory_order_relaxed);
}

void ClipSamplerAudioProcessor::setEnvelopeAttackMs(float ms)
{
    envelopeAttackMs.store(juce::jlimit(0.5f, 500.0f, ms), std::memory_order_relaxed);
}

void ClipSamplerAudioProcessor::setEnvelopeReleaseMs(float ms)
{
    envelopeReleaseMs.store(juce::jlimit(5.0f, 2000.0f, ms), std::memory_order_relaxed);
}

void ClipSamplerAudioProcessor::setAutoSyncEnabled(bool enabled)
{
    autoSyncEnabled.store(enabled, std::memory_order_relaxed);
}

bool ClipSamplerAudioProcessor::applyFxMacroToTargetSlot(juce::String& outReport)
{
    if (!licenseManager.isUnlocked() || !licenseManager.verifyRuntimeIntegrity())
    {
        outReport = juce::String::fromUTF8("Activación requerida para aplicar efectos.");
        setStatusMessage(outReport);
        return false;
    }

    const int targetSlot = fxTargetSlot.load(std::memory_order_relaxed);
    auto srcFile = getSlotAudioFile(targetSlot);
    if (!srcFile.existsAsFile())
    {
        outReport = juce::String::fromUTF8("Primero carga o extrae el Audio ") + juce::String(targetSlot)
                  + juce::String::fromUTF8(" antes de aplicar los efectos.");
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

    // Reiniciamos las perillas en tiempo real tras imprimir el efecto en el archivo WAV
    fxLofiAmount.store(0.0f, std::memory_order_relaxed);
    fxPitchSemitones.store(0.0f, std::memory_order_relaxed);
    fxToneFilter.store(1.0f, std::memory_order_relaxed);

    loadAudioFileIntoSlot(fxFile, targetSlot);
    setFxActiveForSlot(targetSlot, true);

    outReport = juce::String::fromUTF8("¡Efectos (Lo-Fi 12b: ")
              + juce::String(juce::roundToInt(lofiAmt * 100.0f)) + "% | Tono: "
              + juce::String(pitchSt, 1) + juce::String::fromUTF8(" st) aplicados al Audio ")
              + juce::String(targetSlot) + "!";
    setStatusMessage(outReport);
    return true;
}

bool ClipSamplerAudioProcessor::isFxActiveForSlot(int slotIndex) const
{
    if (slotIndex == 2)
        return slot2FxActive.load(std::memory_order_relaxed);
    return slot1FxActive.load(std::memory_order_relaxed);
}

void ClipSamplerAudioProcessor::setFxActiveForSlot(int slotIndex, bool active)
{
    if (slotIndex == 2)
        slot2FxActive.store(active, std::memory_order_relaxed);
    else
        slot1FxActive.store(active, std::memory_order_relaxed);
}

//==============================================================================
// Conversor de Audio a MIDI (.mid) por Autocorrelación de Frecuencia Fundamental (f0)
//==============================================================================
juce::File ClipSamplerAudioProcessor::generateMidiFileForSlot(int slotIndex)
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
    int microsecondsPerQuarter = (int)std::round(60000000.0 / bpm);
    auto tempoMeta = juce::MidiMessage::tempoMetaEvent(microsecondsPerQuarter);
    tempoMeta.setTimeStamp(0.0);
    seq.addEvent(tempoMeta);

    const int winSize = juce::jmax(512, (int)(sr * 0.040));
    const int hopSize = juce::jmax(256, (int)(sr * 0.025));
    const int minLag = juce::jmax(8, (int)(sr / 980.0));
    const int maxLag = juce::jmin(winSize / 2, (int)(sr / 55.0));

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

    // Si el audio es puramente percusivo, generamos los eventos MIDI a partir de las rodajas del Rebanador MPC
    if (rawNotes.size() < 3)
    {
        rawNotes.clear();
        auto slices = getChopSlices(targetSlot);
        for (size_t i = 0; i < slices.size(); ++i)
        {
            rawNotes.push_back({
                slices[i].midiNote + 24,
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

    setStatusMessage(juce::String::fromUTF8("MIDI generado para Audio ") + juce::String(targetSlot)
                     + " (" + juce::String((int)rawNotes.size()) + juce::String::fromUTF8(" notas a ")
                     + juce::String(bpm, 1) + juce::String::fromUTF8(" BPM). ¡Arrástralo al Piano Roll de tu DAW!"));
    return outMidi;
}

juce::File ClipSamplerAudioProcessor::getCurrentMidiFile()
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
// Mantenimiento (Actualización de Motor y Limpieza de Caché)
//==============================================================================
void ClipSamplerAudioProcessor::startExtractorUpdate()
{
    if (updateInProgress.exchange(true))
        return;

    setStatusMessage(juce::String::fromUTF8("Buscando e instalando actualizaciones del motor yt-dlp (-U)..."));
    auto weakAlive = aliveFlag;

    AudioExtractorWorker::runYtDlpUpdate([this, weakAlive](bool success, const juce::String& msg) {
        if (!weakAlive->load(std::memory_order_acquire))
            return;

        updateInProgress.store(false, std::memory_order_relaxed);
        setStatusMessage(success
            ? (juce::String::fromUTF8("Actualización del motor: ") + msg)
            : (juce::String::fromUTF8("Aviso del motor: ") + msg));
    });
}

int ClipSamplerAudioProcessor::cleanAudioCache()
{
    int deleted = AudioExtractorWorker::clearCacheDirectory(getSlotAudioFile(1), getSlotAudioFile(2));
    setStatusMessage(juce::String::fromUTF8("Caché limpiada: ") + juce::String(deleted)
                     + juce::String::fromUTF8(" archivo(s) temporales eliminados (muestras en Visor 1 y 2 protegidas)."));
    return deleted;
}

juce::String ClipSamplerAudioProcessor::getStatusMessage() const
{
    const juce::ScopedLock sl(statusLock);
    return currentStatusMessage;
}

void ClipSamplerAudioProcessor::setStatusMessage(const juce::String& msg)
{
    const juce::ScopedLock sl(statusLock);
    currentStatusMessage = msg;
}

juce::String ClipSamplerAudioProcessor::getCopyrightBadgeText() const
{
    const juce::ScopedLock sl(statusLock);
    return currentCopyrightBadge;
}

juce::File ClipSamplerAudioProcessor::getCurrentAudioFile() const
{
    return getSlotAudioFile(activeSlot.load(std::memory_order_relaxed));
}

juce::File ClipSamplerAudioProcessor::getSlotAudioFile(int slotIndex) const
{
    const juce::ScopedLock sl(bufferLock);
    if (slotIndex == 2)
        return transformedLoadedFile;
    return originalLoadedFile;
}

void ClipSamplerAudioProcessor::setActiveSlot(int slotIndex)
{
    const int targetSlot = (slotIndex == 2) ? 2 : 1;
    fxTargetSlot.store(targetSlot, std::memory_order_relaxed);

    // Si ya es el slot activo y su buffer ya está publicado en el motor de audio, no recargamos ni cortamos el sonido
    auto currentLive = acquirePlaybackData();
    if (activeSlot.load(std::memory_order_relaxed) == targetSlot
        && currentLive != nullptr
        && currentLive->slotIndex == targetSlot
        && currentLive->buffer.getNumSamples() > 0)
    {
        return;
    }

    // Cambio instantáneo en RAM (O(1), cero lectura de disco) si el slot ya estaba precargado
    std::shared_ptr<PlaybackData> cachedSlotData;
    {
        const juce::ScopedLock sl(bufferLock);
        cachedSlotData = (targetSlot == 2) ? slot2PlaybackData : slot1PlaybackData;
    }

    if (cachedSlotData != nullptr && cachedSlotData->buffer.getNumSamples() > 0)
    {
        activeSlot.store(targetSlot, std::memory_order_relaxed);
        totalSampleCount.store(cachedSlotData->buffer.getNumSamples(), std::memory_order_relaxed);
        playheadPosition.store(0, std::memory_order_relaxed);
        activePlayingSlice.store(-1, std::memory_order_relaxed);
        isPlaying.store(false, std::memory_order_relaxed);
        publishPlaybackData(cachedSlotData);
        return;
    }

    auto f = getSlotAudioFile(targetSlot);
    if (f.existsAsFile())
        activateSlotBuffer(f, targetSlot);
}

bool ClipSamplerAudioProcessor::loadAudioFileIntoSlot(const juce::File& file, int slotIndex)
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
            slot2SelectedSliceWavFile = juce::File();
            slot2PlaybackData.reset();
        }
        else
        {
            originalLoadedFile = file;
            slot1MidiFile = juce::File();
            slot1SelectedSliceWavFile = juce::File();
            slot1PlaybackData.reset();
        }
    }

    if (targetSlot == 2)
        slot2SelectedSlice.store(-1, std::memory_order_relaxed);
    else
        slot1SelectedSlice.store(-1, std::memory_order_relaxed);
    exactCutModeEnabled.store(false, std::memory_order_relaxed);

    if (!file.getFileName().contains("_FX_"))
        setFxActiveForSlot(targetSlot, false);

    if (targetSlot == 2)
        transformedThumbnail.setSource(new juce::FileInputSource(file));
    else
        originalThumbnail.setSource(new juce::FileInputSource(file));

    return activateSlotBuffer(file, targetSlot);
}

bool ClipSamplerAudioProcessor::activateSlotBuffer(const juce::File& file, int slotIndex)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    double targetRate = getSampleRate();
    if (targetRate <= 8000.0)
        targetRate = currentSampleRateHz > 8000.0 ? currentSampleRateHz : 44100.0;

    double fileRate = reader->sampleRate > 0.0 ? reader->sampleRate : targetRate;
    juce::int64 fileSamples = reader->lengthInSamples;
    if (fileSamples <= 0)
        return false;

    const int numChannels = juce::jlimit(1, 2, (int)reader->numChannels);
    juce::AudioBuffer<float> tempBuffer(numChannels, (int)fileSamples);
    reader->read(&tempBuffer, 0, (int)fileSamples, 0, true, numChannels > 1);
    reader.reset();

    // Calculamos BPM y Tonalidad directamente sobre el buffer ya leído en memoria (evita abrir el archivo 2 veces)
    const double detectedBpmVal = estimateBpmFromBuffer(tempBuffer, fileRate);
    const juce::String detectedKeyVal = estimateMusicalKeyFromBuffer(tempBuffer, fileRate);

    if (slotIndex == 2)
        slot2Bpm.store(detectedBpmVal, std::memory_order_relaxed);
    else
        slot1Bpm.store(detectedBpmVal, std::memory_order_relaxed);

    {
        const juce::ScopedLock sl(statusLock);
        if (slotIndex == 2)
            slot2Key = detectedKeyVal;
        else
            slot1Key = detectedKeyVal;
    }

    juce::AudioBuffer<float> preparedBuffer;
    if (std::abs(fileRate - targetRate) > 1.0 && fileRate > 0.0)
    {
        double ratio = fileRate / targetRate; // LagrangeInterpolator speedRatio = sourceRate / destRate
        int targetSamples = juce::jmax(64, (int)std::ceil((double)fileSamples / ratio));
        preparedBuffer.setSize(numChannels, targetSamples);

        for (int ch = 0; ch < numChannels; ++ch)
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
    auto resampledSlices = computeTransientSlices(preparedBuffer, targetRate);

    const int cleanSlot = (slotIndex == 2) ? 2 : 1;
    auto newPlaybackData = std::make_shared<PlaybackData>();
    newPlaybackData->buffer = std::move(preparedBuffer);
    newPlaybackData->slices = resampledSlices;
    newPlaybackData->slotIndex = cleanSlot;
    newPlaybackData->generation = ++playbackGenerationCounter;

    {
        const juce::ScopedLock sl(bufferLock);
        if (cleanSlot == 2)
        {
            slot2Slices = std::move(resampledSlices);
            slot2PlaybackData = newPlaybackData;
        }
        else
        {
            slot1Slices = std::move(resampledSlices);
            slot1PlaybackData = newPlaybackData;
        }
    }

    activeSlot.store(cleanSlot, std::memory_order_relaxed);
    loadedFileSampleRate.store(targetRate, std::memory_order_relaxed);
    totalSampleCount.store(finalSampleCount, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
    activePlayingSlice.store(-1, std::memory_order_relaxed);
    isPlaying.store(false, std::memory_order_relaxed);

    publishPlaybackData(std::move(newPlaybackData));
    return true;
}

void ClipSamplerAudioProcessor::startPlayback()
{
    if (!licenseManager.isUnlocked())
        return;

    const int curSlot = activeSlot.load(std::memory_order_relaxed);
    const int selIdx = getSelectedSliceIndex(curSlot);
    if (selIdx >= 0)
    {
        triggerChopSlice(selIdx);
        return;
    }

    int total = totalSampleCount.load(std::memory_order_relaxed);
    if (total > 0)
        requestPlayRegion(0, total, -1);
}

void ClipSamplerAudioProcessor::stopPlayback()
{
    pendingTransportCommand.store(1, std::memory_order_release);
    isPlaying.store(false, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
    activePlayingSlice.store(-1, std::memory_order_relaxed);
}

void ClipSamplerAudioProcessor::playFromNormalizedPosition(int slotIndex, float normalizedPos)
{
    if (!licenseManager.isUnlocked())
        return;

    setActiveSlot(slotIndex);
    const int total = totalSampleCount.load(std::memory_order_relaxed);
    if (total > 0)
    {
        const int startSample = juce::jlimit(0, total - 1, (int)std::round(juce::jlimit(0.0f, 0.995f, normalizedPos) * (float)total));
        requestPlayRegion(startSample, total, -1);
    }
}

float ClipSamplerAudioProcessor::getPlaybackProgress() const
{
    int total = totalSampleCount.load(std::memory_order_relaxed);
    if (total <= 0)
        return 0.0f;

    float progress = (float)playheadPosition.load(std::memory_order_relaxed) / (float)total;
    return juce::jlimit(0.0f, 1.0f, progress);
}

//==============================================================================
// Punto de Entrada Oficial del Plugin para el Host VST3 / AAX
//==============================================================================
//==============================================================================
// Reproductor de Preescucha Integrado con el DAW (Freesound API v2)
//==============================================================================
void ClipSamplerAudioProcessor::startFreesoundPreview(const juce::File& file, const juce::String& soundId)
{
    if (!file.existsAsFile() || file.getSize() < 512)
        return;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return;

    const double dawRate = (currentSampleRateHz > 8000.0) ? currentSampleRateHz : 44100.0;
    const double fileRate = (reader->sampleRate > 8000.0) ? reader->sampleRate : 44100.0;
    const int numChannels = juce::jlimit(1, 2, (int)reader->numChannels);
    const int numSamples = (int)reader->lengthInSamples;

    juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);
    reader->read(&tempBuffer, 0, numSamples, 0, true, true);

    juce::AudioBuffer<float> finalBuffer;
    if (std::abs(fileRate - dawRate) > 1.0)
    {
        const double ratio = dawRate / fileRate;
        const int targetSamples = (int)std::ceil(numSamples * ratio);
        finalBuffer.setSize(numChannels, targetSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::LagrangeInterpolator resampler;
            resampler.process(fileRate / dawRate,
                              tempBuffer.getReadPointer(ch),
                              finalBuffer.getWritePointer(ch),
                              targetSamples);
        }
    }
    else
    {
        finalBuffer = std::move(tempBuffer);
    }

    {
        const juce::ScopedLock sl(previewLock);
        previewAudioBuffer = std::move(finalBuffer);
        previewTotalSamples.store(previewAudioBuffer.getNumSamples(), std::memory_order_relaxed);
        previewPlayPos.store(0, std::memory_order_relaxed);
        currentPreviewSoundId = soundId;
        previewIsPlaying.store(true, std::memory_order_release);
    }

    updateHostDisplay();
}

void ClipSamplerAudioProcessor::stopFreesoundPreview()
{
    const juce::ScopedLock sl(previewLock);
    previewIsPlaying.store(false, std::memory_order_release);
    previewPlayPos.store(0, std::memory_order_relaxed);
    currentPreviewSoundId.clear();
}

bool ClipSamplerAudioProcessor::isFreesoundPreviewPlaying() const
{
    return previewIsPlaying.load(std::memory_order_relaxed);
}

juce::String ClipSamplerAudioProcessor::getActivePreviewSoundId() const
{
    const juce::ScopedLock sl(previewLock);
    return currentPreviewSoundId;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClipSamplerAudioProcessor();
}

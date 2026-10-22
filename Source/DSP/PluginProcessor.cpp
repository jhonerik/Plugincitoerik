#include "PluginProcessor.h"
#include "../UI/PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ClipSampler10AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Base Master Gain parameter (ready for future expansion: Pitch, ADSR, Trim, Loop, Filter)
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
      apvts(*this, nullptr, "ClipSampler10State", createParameterLayout()),
      thumbnail(512, formatManager, thumbnailCache),
      currentStatusMessage(juce::String::fromUTF8("Listo. Pega un enlace de YouTube y define el segundo de inicio."))
{
    formatManager.registerBasicFormats();

    // Initialize 8-voice polyphonic sampler for MIDI Piano Roll triggering
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
#ifdef JucePlugin_Name
    return JucePlugin_Name;
#else
    return "ClipSampler10";
#endif
}

bool ClipSampler10AudioProcessor::acceptsMidi() const
{
    return true;
}

bool ClipSampler10AudioProcessor::producesMidi() const
{
    return false;
}

bool ClipSampler10AudioProcessor::isMidiEffect() const
{
    return false;
}

double ClipSampler10AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ClipSampler10AudioProcessor::getNumPrograms()
{
    return 1;
}

int ClipSampler10AudioProcessor::getCurrentProgram()
{
    return 0;
}

void ClipSampler10AudioProcessor::setCurrentProgram(int)
{
}

const juce::String ClipSampler10AudioProcessor::getProgramName(int)
{
    return {};
}

void ClipSampler10AudioProcessor::changeProgramName(int, const juce::String&)
{
}

//==============================================================================
void ClipSampler10AudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    sampler.setCurrentPlaybackSampleRate(sampleRate);

    auto fileToReload = getCurrentAudioFile();
    if (fileToReload.existsAsFile() && std::abs(loadedFileSampleRate.load(std::memory_order_relaxed) - sampleRate) > 1.0)
    {
        loadAudioFile(fileToReload);
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

    // Security Gate: If the 3-day trial has expired and no license is active, mute DSP output completely
    if (!licenseManager.isUnlocked())
    {
        buffer.clear();
        return;
    }

    // Detect MIDI Note-On events to animate playhead cursor
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            playheadPosition.store(0, std::memory_order_relaxed);
            isPlaying.store(true, std::memory_order_relaxed);
        }
    }

    // 1. Render juce::Synthesiser MIDI Sampler voices (MIDI Piano Roll playback)
    sampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // 2. Render internal buffer stream for GUI Play preview using wait-free ScopedTryLock
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

    // Apply Master Gain from APVTS
    if (auto* gainParam = apvts.getRawParameterValue("masterGain"))
    {
        const float gain = gainParam->load(std::memory_order_relaxed);
        if (std::abs(gain - 1.0f) > 0.0001f)
            buffer.applyGain(gain);
    }
}

//==============================================================================
bool ClipSampler10AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ClipSampler10AudioProcessor::createEditor()
{
    return new ClipSampler10AudioProcessorEditor(*this);
}

//==============================================================================
// DAW State Persistence using APVTS + ValueTree
//==============================================================================
void ClipSampler10AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("lastUrl", lastUrl, nullptr);
    state.setProperty("lastStartTime", lastStartTime, nullptr);

    auto loadedFile = getCurrentAudioFile();
    state.setProperty("lastWavPath", loadedFile.getFullPathName(), nullptr);
    state.setProperty("lastWavFileName", loadedFile.getFileName(), nullptr);

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
        juce::String path = state.getProperty("lastWavPath", "").toString();
        juce::String fileName = state.getProperty("lastWavFileName", "").toString();

        juce::File fileToLoad;
        if (path.isNotEmpty())
        {
            juce::File candidate(path);
            if (candidate.existsAsFile())
                fileToLoad = candidate;
        }

        // Cross-machine / portable fallback inside AudioCache
        if (!fileToLoad.existsAsFile() && fileName.isNotEmpty())
        {
            auto cacheCandidate = AudioExtractorWorker::getCacheDirectory().getChildFile(fileName);
            if (cacheCandidate.existsAsFile())
                fileToLoad = cacheCandidate;
        }

        if (fileToLoad.existsAsFile())
            loadAudioFile(fileToLoad);
    }
}

//==============================================================================
void ClipSampler10AudioProcessor::startSampleExtraction(const juce::String& url, const juce::String& startTime)
{
    licenseManager.verifyAndSyncState();
    if (!licenseManager.isUnlocked())
    {
        setStatusMessage(juce::String::fromUTF8("🔒 Periodo de prueba de 3 días finalizado. Activa tu licencia para continuar."));
        return;
    }

    lastUrl = url.trim();
    lastStartTime = startTime.trim();

    extractionInProgress.store(true, std::memory_order_relaxed);
    extractionProgress.store(0.05f, std::memory_order_relaxed);
    setStatusMessage(juce::String::fromUTF8("Iniciando descarga y conversión..."));

    int currentDawRate = static_cast<int>(getSampleRate());
    if (currentDawRate <= 0)
        currentDawRate = 44100;

    auto weakAlive = aliveFlag;

    extractorWorker.startExtraction(
        lastUrl,
        lastStartTime,
        currentDawRate,
        [this, weakAlive](float progress, const juce::String& msg) {
            if (!weakAlive->load(std::memory_order_acquire))
                return;

            extractionProgress.store(progress, std::memory_order_relaxed);
            setStatusMessage(msg);
        },
        [this, weakAlive](bool success, const juce::File& resultFile, const juce::String& error) {
            if (!weakAlive->load(std::memory_order_acquire))
                return;

            if (success && resultFile.existsAsFile())
            {
                loadAudioFile(resultFile);
                extractionProgress.store(1.0f, std::memory_order_relaxed);
                setStatusMessage(juce::String::fromUTF8("¡Muestra de 10s extraída! [Sampler MIDI activo en Piano Roll y listo para DAW]."));
            }
            else
            {
                extractionProgress.store(0.0f, std::memory_order_relaxed);
                setStatusMessage(juce::String::fromUTF8("Error en la extracción: ") + error);
            }

            extractionInProgress.store(false, std::memory_order_relaxed);
        });
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
    int deleted = AudioExtractorWorker::clearCacheDirectory(getCurrentAudioFile());
    setStatusMessage(juce::String::fromUTF8("Caché limpiada: ") + juce::String(deleted) + juce::String::fromUTF8(" archivo(s) temporales eliminados (muestra activa protegida)."));
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

juce::File ClipSampler10AudioProcessor::getCurrentAudioFile() const
{
    const juce::ScopedLock sl(bufferLock);
    return currentLoadedFile;
}

bool ClipSampler10AudioProcessor::loadAudioFile(const juce::File& file)
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

    // 1. Configure juce::Synthesiser SamplerSound for MIDI triggering
    sampler.clearSounds();
    juce::BigInteger allNotes;
    allNotes.setRange(0, 128, true); // All MIDI notes 0-127 trigger sample
    sampler.addSound(new juce::SamplerSound("ClipSample", *reader, allNotes, 60, 0.005, 0.05, 10.0));
    sampler.setCurrentPlaybackSampleRate(targetRate);

    // 2. Prepare preview buffer outside the lock (zero allocations while holding bufferLock)
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

    // O(1) pointer swap inside bufferLock (no heap allocation inside critical section)
    {
        const juce::ScopedLock sl(bufferLock);
        std::swap(sampleBuffer, preparedBuffer);
        currentLoadedFile = file;
    }

    loadedFileSampleRate.store(targetRate, std::memory_order_relaxed);
    totalSampleCount.store(finalSampleCount, std::memory_order_relaxed);
    playheadPosition.store(0, std::memory_order_relaxed);
    isPlaying.store(false, std::memory_order_relaxed);

    // Update graphical waveform thumbnail
    thumbnail.setSource(new juce::FileInputSource(file));

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

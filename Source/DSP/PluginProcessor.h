#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <vector>
#include "../Services/AudioExtractorWorker.h"
#include "../Security/LicenseManager.h"

/**
 * @struct ChopSlice
 * @brief Representa una rodaja rítmica individual (transiente) dentro del modo Rebanador MPC (Auto-Chop),
 *        asignada cromáticamente al teclado MIDI desde C1 (nota 36) hasta G2 (nota 55).
 */
struct ChopSlice
{
    int midiNote = 36;      ///< Número de nota MIDI asignada (36 = C1 .. 55 = G2).
    juce::String noteName;  ///< Nombre legible de la nota (ej. "C1", "C#1", "D1").
    int startSample = 0;    ///< Índice del sample donde inicia la rodaja.
    int endSample = 0;      ///< Índice del sample donde termina la rodaja.
    float startNorm = 0.0f; ///< Posición normalizada de inicio (0.0 a 1.0) para dibujo en el visor.
    float endNorm = 1.0f;   ///< Posición normalizada de fin (0.0 a 1.0) para dibujo en el visor.
};

/**
 * @class ClipSamplerAudioProcessor
 * @brief Motor principal DSP e Instrumento Virtual (VST3 / AAX) de ClipSampler v2.4.
 *
 * Coordina los subsistemas del plugin:
 * - Extracción asíncrona multi-plataforma (YouTube, TikTok, Instagram) en 3s, 10s o 30s.
 * - Doble ranura de audio (Audio 1 Original y Audio 2 Transformado por Derechos IA).
 * - Modo Rebanador MPC (Auto-Chop) por detección de transientes asignadas a las teclas C1..G2.
 * - Procesador de Efectos Macro en tiempo real y renderizable (Lo-Fi 12-Bit, Tono/Pitch, Filtro y Sincronización BPM).
 * - Detector automático de Tempo (BPM) y Tonalidad Musical (Key) mediante resonadores de Goertzel.
 * - Conversor de Audio a MIDI (.mid) para arrastrar y soltar directamente al Piano Roll del DAW.
 */
class ClipSamplerAudioProcessor : public juce::AudioProcessor
{
public:
    ClipSamplerAudioProcessor();
    ~ClipSamplerAudioProcessor() override;

    //==============================================================================
    // Ciclo de Vida de Audio de JUCE (Tiempo Real)
    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    /** Procesa en tiempo real los bloques de audio y eventos MIDI entrantes desde el DAW. */
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    // Creación de la Interfaz Gráfica (Editor)
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    // Metadatos del Plugin y Programas
    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    // Persistencia de Estado en la Sesión del DAW (APVTS + Propiedades XML/Binarias)
    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // API Multi-Plataforma (YouTube / TikTok / Instagram), Duración y Derechos IA
    //==============================================================================
    void setSelectedPlatform(SourcePlatform platform) { selectedPlatform.store(static_cast<int>(platform), std::memory_order_relaxed); }
    SourcePlatform getSelectedPlatform() const { return static_cast<SourcePlatform>(selectedPlatform.load(std::memory_order_relaxed)); }

    /** Inicia la extracción en segundo plano desde la plataforma seleccionada (3s, 10s o 30s). */
    void startSampleExtraction(const juce::String& url, const juce::String& startTime, int durationSeconds = 10);

    /** Consulta asíncronamente la duración total del video y sus metadatos de derechos. */
    void detectVideoDuration(const juce::String& url);

    /** Analiza el Audio 1 (Original) para detectar riesgo de huella acústica / Content ID. */
    bool analyzeOriginalCopyright(juce::String& outReport);

    /**
     * Genera el Audio 2 ("Derechos IA") a partir del Audio 1 desplazando ±3 semitonos
     * y aplicando dispersión de fase all-pass + micro-modulación estéreo para evadir Content ID.
     */
    bool applyDerechosTransformation(int semitoneShift, juce::String& outReport);

    /** Verifica matemáticamente la divergencia espectral entre el Audio 1 y el Audio 2. */
    bool verifyTransformedAudioCopyrightFree(juce::String& outReport);

    /** Devuelve true si la ranura indicada (1 o 2) tiene un archivo de audio cargado. */
    bool hasSlotAudio(int slotIndex) const;

    /**
     * Devuelve true únicamente cuando TANTO el Audio 1 como el Audio 2 están listos,
     * habilitando el botón "Nueva Extracción" para limpiar el plugin por completo.
     */
    bool hasBothAudiosReady() const;

    /** Limpia ambos visores, tajadas MPC, BPM/Tono, efectos y enlaces para iniciar una nueva extracción. */
    void resetForNewExtraction();

    /** Cambia la ranura activa para reproducción, MIDI y exportación (1 = Original, 2 = Modificado Derechos IA). */
    void setActiveSlot(int slotIndex);
    int getActiveSlot() const { return activeSlot.load(std::memory_order_relaxed); }

    //==============================================================================
    // Modo Rebanador MPC (Auto-Chop de Transientes C1 -> G2)
    //==============================================================================
    void setAutoChopEnabled(bool enabled);
    bool isAutoChopEnabled() const { return autoChopEnabled.load(std::memory_order_relaxed); }

    /** Ejecuta el detector de transientes, BPM y Tonalidad sobre la ranura indicada (1 o 2). */
    void analyzeAutoChopAndMusicData(int slotIndex);

    /** Devuelve el vector de rodajas MPC detectadas para la ranura solicitada. */
    std::vector<ChopSlice> getChopSlices(int slotIndex) const;

    /** Dispara la reproducción inmediata de una rodaja específica (0 = C1, 1 = C#1, etc.). */
    void triggerChopSlice(int sliceIndex);

    /** Devuelve el índice de la rodaja que está sonando actualmente, o -1 si ninguna. */
    int getActivePlayingSlice() const { return activePlayingSlice.load(std::memory_order_relaxed); }

    //==============================================================================
    // Macro de Efectos (Lo-Fi 12-Bit, Tono en Semitonos, Filtro y Sincronización BPM)
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

    /** Imprime de forma permanente los efectos actuales en el archivo WAV de la ranura seleccionada (1 o 2). */
    bool applyFxMacroToTargetSlot(juce::String& outReport);

    //==============================================================================
    // Detector Automático de Tempo (BPM) y Tonalidad Musical (Tono)
    //==============================================================================
    double getDetectedBpm(int slotIndex) const;
    juce::String getDetectedKey(int slotIndex) const;
    double getHostBpm() const { return currentHostBpm.load(std::memory_order_relaxed); }

    //==============================================================================
    // Conversor de Audio a MIDI (Arrastrar .MIDI al Piano Roll)
    //==============================================================================
    /** Detecta las notas fundamentales del audio y genera un archivo estándar .mid listo para arrastrar. */
    juce::File generateMidiFileForSlot(int slotIndex);
    juce::File getCurrentMidiFile();

    /** Actualiza el motor yt-dlp de forma asíncrona. */
    void startExtractorUpdate();

    /** Limpia la carpeta de caché preservando los audios activos en el Visor 1 y Visor 2. */
    int cleanAudioCache();

    /** Carga un archivo .wav en la ranura 1 o 2 y lo activa para reproducción. */
    bool loadAudioFileIntoSlot(const juce::File& file, int slotIndex);
    bool loadAudioFile(const juce::File& file) { return loadAudioFileIntoSlot(file, 1); }

    /** Inicia o detiene la reproducción del audio activo. */
    void startPlayback();
    void stopPlayback();
    bool isAudioPlaying() const { return isPlaying.load(std::memory_order_relaxed); }
    float getPlaybackProgress() const;

    /** Accesores de archivos y estado para la interfaz gráfica. */
    juce::File getCurrentAudioFile() const;
    juce::File getSlotAudioFile(int slotIndex) const;
    double getLoadedFileSampleRate() const { return loadedFileSampleRate.load(std::memory_order_relaxed); }

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

    juce::AudioThumbnail& getThumbnail(int slotIndex = 1) { return (slotIndex == 2) ? transformedThumbnail : originalThumbnail; }
    juce::AudioFormatManager& getFormatManager() { return formatManager; }

    LicenseManager& getLicenseManager() { return licenseManager; }
    const LicenseManager& getLicenseManager() const { return licenseManager; }

    juce::String getLastUrl() const { return lastUrl; }
    void setLastUrl(const juce::String& url) { lastUrl = url; }

    juce::String getLastStartTime() const { return lastStartTime; }
    void setLastStartTime(const juce::String& time) { lastStartTime = time; }

    /** Convierte un número de nota MIDI (0-127) a su notación musical estándar (ej. 36 -> "C1"). */
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

    // Estado del Rebanador MPC, BPM y Tonalidad por cada ranura
    std::atomic<bool> autoChopEnabled { true };
    std::vector<ChopSlice> slot1Slices;
    std::vector<ChopSlice> slot2Slices;
    std::atomic<double> slot1Bpm { 0.0 };
    std::atomic<double> slot2Bpm { 0.0 };
    juce::String slot1Key { "--" };
    juce::String slot2Key { "--" };
    std::atomic<double> currentHostBpm { 120.0 };

    // Estado de las Perillas de Efectos Macro
    std::atomic<int> fxTargetSlot { 1 };
    std::atomic<float> fxLofiAmount { 0.0f };       // 0.0 a 1.0 (Reducción de bits Lo-Fi 12-Bit)
    std::atomic<float> fxPitchSemitones { 0.0f };   // -12.0 a +12.0 semitonos
    std::atomic<float> fxToneFilter { 1.0f };       // 0.1 a 1.0 (Filtro paso-bajo cálido)
    std::atomic<bool> autoSyncEnabled { false };
    float lofiFilterStateL = 0.0f;
    float lofiFilterStateR = 0.0f;

    // Parámetros de sesión almacenados
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
    std::atomic<int> copyrightVerifyState { 0 };

    // Estado sincronizado entre hilos para los mensajes de la UI
    mutable juce::CriticalSection statusLock;
    juce::String currentStatusMessage;
    juce::String currentCopyrightBadge;
    std::atomic<bool> extractionInProgress { false };
    std::atomic<bool> updateInProgress { false };
    std::atomic<bool> metadataInProgress { false };
    std::atomic<float> extractionProgress { 0.0f };

    // Punteros de reproducción en tiempo real con interpolación fraccionaria
    std::atomic<bool> isPlaying { false };
    std::atomic<double> fractionalPlayhead { 0.0 };
    std::atomic<int> playheadPosition { 0 };
    std::atomic<int> playbackEndSample { 0 };
    std::atomic<int> totalSampleCount { 0 };
    std::atomic<int> activePlayingSlice { -1 };

    std::shared_ptr<std::atomic<bool>> aliveFlag = std::make_shared<std::atomic<bool>>(true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSamplerAudioProcessor)
};

// Alias de compatibilidad hacia atrás
using ClipSampler10AudioProcessor = ClipSamplerAudioProcessor;

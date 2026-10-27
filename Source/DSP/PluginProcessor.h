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

    //==============================================================================
    // Separador de Stems por IA (Vocales / Beat)
    //==============================================================================
    enum class StemType
    {
        Vocals = 0, ///< Aislar Vocales (Acapella / Voz limpia)
        Beat = 1    ///< Aislar Beat / Instrumental (Drums, Bass y Acompañamiento)
    };

    /**
     * Separa el audio activo en Vocales (Acapella) o Beat (Instrumental)
     * mediante separación espectral de formantes y cancelación de fase central,
     * cargando el resultado en la Ranura 2.
     */
    bool separateAndLoadStem(StemType type, juce::String& outReport);

    /** Devuelve el tipo de contenido cargado en la Ranura 2 ("DERECHOS ±3 ST IA", "VOCALES IA (ACAPELLA)" o "BEAT IA (INSTRUMENTAL)"). */
    juce::String getSlot2ContentTypeString() const;
    int getSlot2ContentType() const { return slot2ContentType.load(std::memory_order_relaxed); }

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

    /** Mueve con precisión milimétrica la línea divisoria entre rodajas adyacentes en la ranura indicada. */
    bool moveSliceDividerPosition(int slotIndex, int dividerIndex, float newNormalizedPos);

    /** Ejecuta el detector de transientes, BPM y Tonalidad sobre la ranura indicada (1 o 2). */
    void analyzeAutoChopAndMusicData(int slotIndex);

    /** Devuelve el vector de rodajas MPC detectadas para la ranura solicitada. */
    std::vector<ChopSlice> getChopSlices(int slotIndex) const;

    /** Dispara la reproducción inmediata de una rodaja específica (0 = C1, 1 = C#1, etc.). */
    void triggerChopSlice(int sliceIndex);

    /** Devuelve el índice de la rodaja que está sonando actualmente, o -1 si ninguna. */
    int getActivePlayingSlice() const { return activePlayingSlice.load(std::memory_order_relaxed); }

    //==============================================================================
    // Selección de Corte Exacto (1 Clic = Todo el Audio | Doble Clic = Nota / Trozo)
    //==============================================================================
    void setExactCutModeEnabled(bool enabled);
    bool isExactCutModeEnabled() const { return exactCutModeEnabled.load(std::memory_order_relaxed); }

    /** Selecciona todo el audio completo en la ranura indicada (1 clic). */
    void selectFullAudioInSlot(int slotIndex);

    /** Selecciona la nota/trozo exacto (C1, C#1, D1...) en la posición normalizada donde se hizo doble clic. */
    bool selectSliceAtNormalizedPosition(int slotIndex, float normalizedPos, ChopSlice& outSlice);

    /** Selecciona directamente una nota/trozo por índice (0 = C1, 1 = C#1...) y genera su archivo .WAV recortado. */
    bool selectSliceByIndex(int slotIndex, int sliceIndex, ChopSlice& outSlice);

    /** Devuelve el índice de la nota seleccionada en la ranura (-1 si está seleccionado todo el audio). */
    int getSelectedSliceIndex(int slotIndex) const;

    /** Devuelve el nombre de la nota seleccionada (ej. "C1", "D#1") o cadena vacía si está seleccionado todo el audio. */
    juce::String getSelectedSliceName(int slotIndex) const;

    /** Genera y devuelve un archivo .WAV exclusivo con el trozo exacto seleccionado listo para arrastrar al DAW. */
    juce::File getOrCreateSelectedSliceWavFile(int slotIndex);

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

    //==============================================================================
    // Envolvente Básica (Ataque y Caída / Attack & Release Anti-Chasquidos)
    //==============================================================================
    void setEnvelopeAttackMs(float ms);
    float getEnvelopeAttackMs() const { return envelopeAttackMs.load(std::memory_order_relaxed); }

    void setEnvelopeReleaseMs(float ms);
    float getEnvelopeReleaseMs() const { return envelopeReleaseMs.load(std::memory_order_relaxed); }

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
    void playFromNormalizedPosition(int slotIndex, float normalizedPos);
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

    mutable juce::CriticalSection bufferLock;   // Protege SOLO rutas de archivos y vectores de rodajas para la UI (el hilo de audio NUNCA lo toma)
    juce::File originalLoadedFile;
    juce::File transformedLoadedFile;
    juce::File slot1MidiFile;
    juce::File slot2MidiFile;
    std::atomic<int> activeSlot { 1 };
    std::atomic<double> loadedFileSampleRate { 44100.0 };

    // Estado del Rebanador MPC, Corte Exacto, BPM y Tonalidad por cada ranura
    std::atomic<bool> autoChopEnabled { true };
    std::atomic<bool> exactCutModeEnabled { false };
    std::atomic<int> slot1SelectedSlice { -1 }; // -1 = Todo el audio, >= 0 = Índice de rodaja (C1, C#1, D1...)
    std::atomic<int> slot2SelectedSlice { -1 };
    juce::File slot1SelectedSliceWavFile;
    juce::File slot2SelectedSliceWavFile;
    std::vector<ChopSlice> slot1Slices;
    std::vector<ChopSlice> slot2Slices;
    std::atomic<double> slot1Bpm { 0.0 };
    std::atomic<double> slot2Bpm { 0.0 };
    juce::String slot1Key { "--" };
    juce::String slot2Key { "--" };
    std::atomic<double> currentHostBpm { 120.0 };

    // Estado de las Perillas de Efectos Macro y Envolvente (Attack/Release)
    std::atomic<int> fxTargetSlot { 1 };
    std::atomic<float> fxLofiAmount { 0.0f };       // 0.0 a 1.0 (Reducción de bits Lo-Fi 12-Bit)
    std::atomic<float> fxPitchSemitones { 0.0f };   // -12.0 a +12.0 semitonos
    std::atomic<float> fxToneFilter { 1.0f };       // 0.1 a 1.0 (Filtro paso-bajo cálido)
    std::atomic<float> envelopeAttackMs { 2.0f };   // 0.5 ms a 500.0 ms (Ataque anti-chasquidos)
    std::atomic<float> envelopeReleaseMs { 50.0f }; // 5.0 ms a 2000.0 ms (Caída suave al soltar pad/nota)
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
    std::atomic<int> slot2ContentType { 0 }; // 0 = Derechos IA, 1 = Vocales IA, 2 = Beat IA

    // Estado sincronizado entre hilos para los mensajes de la UI
    mutable juce::CriticalSection statusLock;
    juce::String currentStatusMessage;
    juce::String currentCopyrightBadge;
    std::atomic<bool> extractionInProgress { false };
    std::atomic<bool> updateInProgress { false };
    std::atomic<bool> metadataInProgress { false };
    std::atomic<float> extractionProgress { 0.0f };

    // Estado de reproducción publicado por el hilo de audio para que la UI lo lea (solo lectura desde la UI)
    std::atomic<bool> isPlaying { false };
    std::atomic<int> playheadPosition { 0 };
    std::atomic<int> totalSampleCount { 0 };
    std::atomic<int> activePlayingSlice { -1 };

    //==============================================================================
    // Motor de Reproducción v2.5 (Lock-Free + Interpolación Hermite + Anti-Click)
    //==============================================================================
    /**
     * Paquete inmutable de audio listo para reproducir. El hilo de mensajes crea uno nuevo
     * cada vez que cambia la muestra activa y lo publica con un simple intercambio de puntero,
     * de modo que el hilo de audio jamás espera a la interfaz ni lee un buffer a medio escribir.
     */
    struct PlaybackData
    {
        juce::AudioBuffer<float> buffer;   ///< Audio PCM ya remuestreado al Sample Rate del DAW.
        std::vector<ChopSlice> slices;     ///< Rodajas MPC que corresponden exactamente a este buffer.
        int slotIndex = 1;                 ///< Ranura de origen (1 = Original, 2 = Derechos IA).
        juce::uint32 generation = 0;       ///< Contador único para detectar cambios de buffer en el hilo de audio.
    };

    /** Voz de reproducción polifónica con envolvente anti-click (fade-in, fade-out y crossfade). */
    struct PlaybackVoice
    {
        bool active = false;
        bool releasing = false;       ///< true mientras hace fade-out (detenida, NoteOff o reemplazada por otra rodaja).
        double position = 0.0;        ///< Posición fraccionaria de lectura dentro del buffer.
        double notePitchRatio = 1.0;  ///< Ratio de tono de la nota MIDI (1.0 en modo MPC/Play; 2^((nota-60)/12) en modo Cromático).
        int endSample = 0;            ///< Último sample (exclusivo) de la región a reproducir.
        int sliceIndex = -1;          ///< Rodaja MPC que suena (-1 = muestra completa).
        int midiNote = -1;            ///< Nota MIDI asociada en modo cromático (-1 = disparo one-shot).
        float gain = 0.0f;            ///< Ganancia actual de la envolvente.
        float targetGain = 1.0f;      ///< Ganancia final tras el fade-in (velocity MIDI).
        float gainStep = 0.0f;        ///< Incremento de ganancia por sample (positivo = fade-in, negativo = fade-out).
        juce::uint32 order = 0;       ///< Orden de disparo (la voz más reciente define el cabezal visual).
    };

    std::shared_ptr<PlaybackData> acquirePlaybackData() const;
    void publishPlaybackData(std::shared_ptr<PlaybackData> newData);
    void requestPlayRegion(int startSample, int endSample, int sliceIndex);
    void startVoice(const PlaybackData& data, int startSample, int endSample, int sliceIndex, float velocityGain, double notePitchRatio = 1.0, int midiNote = -1, bool chokeExisting = true);
    void releaseVoiceForMidiNote(int midiNote);
    void releaseAllVoices();
    void killAllVoices();
    void handlePendingTransportCommand(const PlaybackData& data);
    void triggerNoteFromMidi(const PlaybackData& data, int midiNote, float velocity, bool chopMode);
    void renderVoices(const PlaybackData& data, juce::AudioBuffer<float>& out, int startSample, int numSamples);
    void publishPlaybackStateToUi();
    void processMacroFx(juce::AudioBuffer<float>& buffer);

    std::shared_ptr<PlaybackData> livePlaybackData;                 // Puntero activo en el hilo de audio (protegido por playbackDataLock)
    std::shared_ptr<PlaybackData> slot1PlaybackData;                // Caché en RAM de Audio 1 para cambio instantáneo sin leer disco
    std::shared_ptr<PlaybackData> slot2PlaybackData;                // Caché en RAM de Audio 2 para cambio instantáneo sin leer disco
    mutable juce::SpinLock playbackDataLock;                        // Solo protege la COPIA del puntero (nanosegundos)
    std::vector<std::shared_ptr<PlaybackData>> retiredPlaybackData; // Buffers antiguos liberados fuera del hilo de audio
    juce::CriticalSection retiredLock;
    std::atomic<juce::uint32> playbackGenerationCounter { 0 };

    static constexpr int kNumPlaybackVoices = 8;                    // 8 voces polifónicas con crossfade suave sin cortes
    PlaybackVoice voices[kNumPlaybackVoices];
    juce::uint32 voiceOrderCounter = 0;
    juce::uint32 lastRenderedGeneration = 0;

    // Comandos de transporte enviados por la UI al hilo de audio (sin locks)
    std::atomic<int> pendingTransportCommand { 0 };               // 0 = ninguno, 1 = detener, 2 = reproducir
    std::atomic<int> pendingStartSample { 0 };
    std::atomic<int> pendingEndSample { 0 };
    std::atomic<int> pendingSliceIndex { -1 };

    // Suavizado de parámetros para eliminar ruido de "cremallera" (zipper noise)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedPlaybackRate { 1.0f };
    juce::SmoothedValue<float> smoothedMasterGain { 1.0f };
    juce::SmoothedValue<float> smoothedToneAlpha { 1.0f };
    double currentSampleRateHz = 44100.0;
    int fadeInSamples = 64;       // ~1.5 ms: conserva el ataque de las transientes sin producir clicks
    int fadeOutSamples = 288;     // ~6 ms: crossfade al detener o re-disparar
    int endFadeSamples = 144;     // ~3 ms: suaviza el final de cada región / rodaja

    // Estado persistente del Lo-Fi entre bloques (evita discontinuidades periódicas)
    float lofiHeldSample[2] { 0.0f, 0.0f };
    int lofiHoldCounter[2] { 0, 0 };

    // Caché de la verificación de licencia en el hilo de audio (se re-evalúa ~4 veces por segundo)
    bool cachedAudioUnlocked = false;
    int licenseRecheckCountdown = 0;

    std::shared_ptr<std::atomic<bool>> aliveFlag = std::make_shared<std::atomic<bool>>(true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSamplerAudioProcessor)
};

// Alias de compatibilidad hacia atrás
using ClipSampler10AudioProcessor = ClipSamplerAudioProcessor;

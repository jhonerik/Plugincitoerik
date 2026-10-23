#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include <atomic>

/**
 * @enum SourcePlatform
 * @brief Plataformas digitales soportadas por el motor de extracción de ClipSampler.
 * Permite al algoritmo aplicar cabeceras HTTP, normalización de enlaces y modos de recorte específicos.
 */
enum class SourcePlatform
{
    YouTube = 0,   ///< Videos largos, YouTube Shorts y YouTube Music (soporta descarga por secciones DASH).
    TikTok = 1,    ///< Videos de TikTok y enlaces cortos vm.tiktok.com / vt.tiktok.com.
    Instagram = 2  ///< Instagram Reels, Posts (/p/) e IGTV (/tv/).
};

/**
 * @class AudioExtractorWorker
 * @brief Hilo de trabajo en segundo plano encargado de la extracción, lectura de metadatos
 *        y normalización de audio multi-plataforma (YouTube, TikTok e Instagram).
 *
 * Responsabilidades principales:
 * 1. Consultar de forma asíncrona la duración total del video, título y metadatos de derechos de autor.
 * 2. Normalizar enlaces según la plataforma e inyectar cabeceras anti-bloqueo (User-Agent y Referer).
 * 3. Descargar fragmentos de 3, 10 o 30 segundos y convertirlos a formato WAV PCM de 24 bits
 *    sincronizado con la frecuencia de muestreo (Sample Rate) del DAW anfitrión.
 * 4. Aplicar rampas de entrada/salida (anti-click fade-in/out) y normalización a -1.0 dBFS.
 */
class AudioExtractorWorker : public juce::Thread
{
public:
    /** Callback para reportar el progreso (0.0 a 1.0) y el mensaje de estado a la interfaz gráfica. */
    using ProgressCallback = std::function<void(float progress0to1, const juce::String& statusMessage)>;

    /** Callback invocado al finalizar la extracción con el archivo .wav resultante o mensaje de error. */
    using CompletionCallback = std::function<void(bool success, const juce::File& resultFile, const juce::String& errorMessage)>;

    /**
     * @struct VideoMetadata
     * @brief Estructura que almacena los metadatos detectados de un enlace multimedia.
     */
    struct VideoMetadata
    {
        bool success = false;                               ///< Indica si la consulta fue exitosa.
        SourcePlatform platform = SourcePlatform::YouTube;  ///< Plataforma a la que pertenece el video.
        int durationSeconds = 0;                            ///< Duración total del video en segundos.
        juce::String title;                                 ///< Título del video o publicación.
        juce::String licenseInfo;                           ///< Tipo de licencia reportada (ej. Estándar o Creative Commons).
        juce::String trackInfo;                             ///< Nombre de la pista musical o creador detectado.
        bool hasCopyrightMetadata = false;                  ///< True si los metadatos indican contenido comercial protegido.
        juce::String errorMessage;                          ///< Descripción del error en caso de fallo.
    };

    AudioExtractorWorker();
    ~AudioExtractorWorker() override;

    /** Detecta automáticamente si una URL pertenece a YouTube, TikTok o Instagram. */
    static SourcePlatform detectPlatformFromUrl(const juce::String& url);

    /** Devuelve el nombre legible de la plataforma ("YouTube", "TikTok" o "Instagram"). */
    static juce::String getPlatformName(SourcePlatform platform);

    /** Normaliza los enlaces de cada plataforma (ej. convierte /reels/ a /reel/ y limpia parámetros basura). */
    static juce::String normalizeUrlForPlatform(const juce::String& rawUrl, SourcePlatform platform);

    /** Verifica si la URL ingresada es válida y coincide con la plataforma seleccionada. */
    static bool isValidPlatformUrl(const juce::String& url, SourcePlatform platform, juce::String& outReason);

    /** Inicia un trabajo de extracción en un hilo secundario para la duración elegida (3s, 10s o 30s). */
    void startExtraction(const juce::String& mediaUrl,
                         SourcePlatform platform,
                         const juce::String& startTimeString,
                         int durationSeconds,
                         int targetSampleRate,
                         ProgressCallback progressCb,
                         CompletionCallback completionCb);

    /** Cancela cualquier extracción activa y termina de forma segura los subprocesos hijos. */
    void cancelExtraction();

    /** Indica si el hilo de extracción está procesando una descarga actualmente. */
    bool isExtracting() const { return isThreadRunning(); }

    /** Consulta asíncronamente la duración total y metadatos de derechos para YouTube, TikTok o Instagram. */
    static void fetchVideoMetadata(const juce::String& mediaUrl,
                                   SourcePlatform platform,
                                   std::function<void(const VideoMetadata&)> onComplete);

    /** Convierte una cadena de tiempo ("MM:SS", "HH:MM:SS" o segundos) a segundos enteros. */
    static int parseTimeToSeconds(const juce::String& timeString);

    /** Formatea una cantidad de segundos al estándar "HH:MM:SS". */
    static juce::String formatSecondsToTime(int totalSeconds);

    /** Formatea una cantidad de segundos al formato compacto "MM:SS" (o "HH:MM:SS" si supera 1 hora). */
    static juce::String formatSecondsToMMSS(int totalSeconds);

    /** Obtiene y crea el directorio de caché de audio del plugin (AppData/Roaming/ClipSampler/AudioCache). */
    static juce::File getCacheDirectory();

    /** Limpia los archivos temporales en caché protegiendo los archivos actualmente cargados en el Visor 1 y 2. */
    static int clearCacheDirectory(const juce::File& activeFile1 = {}, const juce::File& activeFile2 = {});

    /** Ejecuta la actualización asíncrona de yt-dlp (-U) con límite de tiempo de seguridad. */
    static void runYtDlpUpdate(std::function<void(bool success, const juce::String& outputMsg)> onComplete);

    /** Busca los binarios auxiliares (yt-dlp, ffmpeg) en las carpetas de ClipSampler y en el PATH del sistema. */
    static juce::File findExecutable(const juce::String& executableName);

private:
    /** Bucle principal del hilo secundario que ejecuta la descarga y conversión a WAV. */
    void run() override;

    /** Valida el archivo WAV descargado, aplica el recorte exacto al sample, micro-fades y normalización. */
    bool validateAndEnforceDuration(const juce::File& audioFile, int targetRate, int startOffsetSec, int maxDurationSec);

    /** Termina el proceso hijo activo si el usuario cancela o cierra el plugin. */
    void killActiveProcessTree();

    /** Añade los argumentos de red y cabeceras HTTP específicos para YouTube, TikTok o Instagram. */
    static void appendPlatformSpecificArgs(juce::StringArray& args, SourcePlatform platform);

    // Parámetros de la tarea de extracción actual
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

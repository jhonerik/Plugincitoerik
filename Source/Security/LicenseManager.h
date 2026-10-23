#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>

/**
 * @class LicenseManager
 * @brief Motor de Seguridad Anti-Piratería, Huella de Hardware, Prueba de 3 Días y Activación de Licencias.
 *
 * Arquitectura de defensa en profundidad:
 * 1. Ofuscación XOR en Tiempo de Compilación + Limpieza de Stack:
 *    Ninguna clave secreta de firma ni sal criptográfica aparece en texto plano dentro del binario .vst3.
 * 2. Huella de Hardware Multi-Factor (Machine ID):
 *    Combina el MachineGuid de Windows, número de serie del volumen C:\, fabricante/modelo/núcleos de CPU
 *    y nombre del equipo para generar un identificador único determinista ("MCH-XXXX-XXXX-XXXX").
 * 3. Bóveda Cifrada de 5 Anclajes (Protección de Prueba de 3 Días):
 *    Sincroniza la fecha de primera instalación, última ejecución, bandera anti-retroceso de reloj
 *    y clave de licencia en 5 ubicaciones independientes (ProgramData/ClipSampler, AppData/Roaming/ClipSampler,
 *    caché camuflado en LocalAppData y 2 ramas del Registro de Windows).
 * 4. Canario Criptográfico de 64 bits en RAM y Protección Anti-Debugger:
 *    Sustituye las validaciones booleanas simples por un token matemático de 64 bits verificado en DSP y UI.
 * 5. Heartbeat Asíncrono de Revocación Online (/api/verify):
 *    Comprueba silenciosamente contra el servidor de licencias si una clave ha sido revocada.
 */
class LicenseManager
{
public:
    /** Duración estricta del periodo de prueba gratuito: 3 días (72 horas = 259,200 segundos). */
    static constexpr juce::int64 TRIAL_DURATION_SECONDS = 3LL * 24LL * 3600LL;

    /** Resultado devuelto tras intentar activar una licencia. */
    struct ActivationResult
    {
        bool success = false;
        juce::String message;
    };

    LicenseManager();
    ~LicenseManager() = default;

    /** Actualiza el temporizador de prueba, verifica alteraciones de reloj/debugger y sincroniza los 5 anclajes. */
    void verifyAndSyncState();

    /** Devuelve true si existe una licencia oficial activa vinculada a este equipo y el canario RAM es válido. */
    bool isLicensed() const;

    /** Devuelve true si la prueba de 3 días sigue vigente, sin alteración de reloj y con canario RAM válido. */
    bool isTrialActive() const;

    /** Devuelve true si el plugin está autorizado para procesar audio, extraer muestras y exportar. */
    bool isUnlocked() const;

    /** Verificación secundaria independiente de integridad en tiempo de ejecución para proteger el motor DSP. */
    bool verifyRuntimeIntegrity() const;

    /** Devuelve true si se detectó que el usuario atrasó o congeló el reloj del sistema. */
    bool isClockTampered() const { return clockTamperedFlag.load(std::memory_order_relaxed); }

    /** Devuelve los segundos restantes del periodo de prueba de 3 días (0 si ya expiró). */
    juce::int64 getRemainingTrialSeconds() const { return remainingTrialSeconds.load(std::memory_order_relaxed); }

    /** Devuelve el ID de Máquina único de esta computadora (formato "MCH-XXXX-XXXX-XXXX"). */
    juce::String getMachineId() const { return machineId; }

    /** Devuelve el texto resumido para el botón de estado en la cabecera del plugin. */
    juce::String getHeaderStatusBadgeText() const;

    /** Devuelve la clave de licencia activada actualmente (o cadena vacía si está en modo prueba). */
    juce::String getActivatedLicenseKey() const;

    /**
     * Intenta activar el plugin con el código de licencia ingresado.
     * Soporta tanto claves criptográficas vinculadas al Machine ID como claves dinámicas CS10-AUTO-... del servidor.
     */
    ActivationResult activateLicense(const juce::String& licenseKeyInput,
                                     const juce::String& serverEndpointUrl = "http://127.0.0.1:8765/api/activate");

    /** Consulta silenciosa en segundo plano para verificar que la licencia no haya sido revocada en la base de datos. */
    void verifyOnlineRevocationHeartbeat(const juce::String& verifyEndpointUrl = "http://127.0.0.1:8765/api/verify");

    /** Generador determinista de firma criptográfica SHA-256 de 3 rondas. */
    static juce::String computeLicenseSignature(const juce::String& normalizedMachineId,
                                                const juce::String& tierCode,
                                                const juce::String& serialBlock);

    /** Verifica matemáticamente si un código de licencia corresponde al Hardware ID de este equipo. */
    static bool verifyCryptographicKeyForMachine(const juce::String& cleanKey,
                                                 const juce::String& targetMachineId,
                                                 juce::String& outErrorMsg);

    /** Detecta si hay un depurador (debugger) conectado al proceso del DAW/plugin. */
    static bool isDebuggerAttached();

private:
    /** Estructura interna que representa los datos guardados en la bóveda cifrada. */
    struct VaultData
    {
        juce::int64 firstRunUtc = 0;      ///< Timestamp UTC de la primera vez que se abrió el plugin.
        juce::int64 lastSeenUtc = 0;      ///< Timestamp UTC más reciente observado.
        bool clockTampered = false;       ///< Indica si se detectó retroceso en el reloj del sistema.
        juce::String activatedKey;        ///< Clave de licencia activada.
        juce::String boundMachineId;      ///< ID de máquina vinculado a la licencia.
    };

    static juce::String buildHardwareMachineId();
    juce::uint64 computeExpectedCanary(bool unlockedState) const;
    juce::String encryptVaultPayload(const VaultData& data) const;
    bool decryptVaultPayload(const juce::String& cipherHex, VaultData& outData) const;

    void loadAndMergeAnchors(VaultData& mergedData);
    void saveToAllAnchors(const VaultData& data) const;

    static juce::File getPrimaryVaultFile();
    static juce::File getSecondaryVaultFile();
    juce::File getStealthTertiaryVaultFile() const;
    static juce::String getRegistryVaultPathPrimary();
    static juce::String getRegistryVaultPathSecondary();

    juce::String machineId;
    juce::uint64 machineCanarySeed = 0;
    mutable juce::CriticalSection stateLock;
    juce::String currentActivatedKey;

    std::atomic<bool> licensedFlag { false };
    std::atomic<bool> trialActiveFlag { true };
    std::atomic<bool> clockTamperedFlag { false };
    std::atomic<juce::int64> remainingTrialSeconds { TRIAL_DURATION_SECONDS };
    std::atomic<juce::uint64> runtimeCanaryToken { 0 };
    std::atomic<bool> heartbeatInProgress { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LicenseManager)
};

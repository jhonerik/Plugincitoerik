#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>

/**
 * LicenseManager (Hardened Anti-Piracy & Security Engine v2.3.1)
 * 
 * Defensive Security Architecture:
 * 1. Compile-Time XOR Secret Obfuscation + Stack Zeroing:
 *    Zero plaintext signing keys or vault salts in the compiled .vst3 binary (.rdata).
 * 2. Multi-Factor Hardware Fingerprinting (Machine ID):
 *    Combines Windows Cryptography MachineGuid, System Volume Serial Number, Computer BIOS/Host Name,
 *    and CPU architecture signature into a deterministic Hardware ID ("MCH-XXXX-XXXX-XXXX").
 * 3. Quad-Anchor + Stealth Registry Encrypted Vault (3-Day Trial Protection):
 *    Persists first-install timestamp, last-seen timestamp, file-system monotonicity timestamps,
 *    clock-tamper flag, and activated license across 5 independent locations (ProgramData, AppData,
 *    LocalAppData Stealth Cache, and 2 Registry hives). Deleting any subset automatically self-heals
 *    from the surviving anchors.
 * 4. Runtime Cryptographic Canary & Anti-Debug Protection:
 *    Replaces single-boolean checks with a 64-bit hardware-derived integrity canary verified across
 *    DSP, Extraction, Derechos AI, FX Macro, and MIDI export paths, plus Win32 anti-debugger checks.
 * 5. Online Revocation & Anti-Cloning Heartbeat:
 *    Asynchronously verifies active licenses against the License Server DB (/api/verify) to enforce
 *    remote license revocation and single-machine binding.
 */
class LicenseManager
{
public:
    /** Strictly enforced 3-day trial duration (72 hours = 259,200 seconds). */
    static constexpr juce::int64 TRIAL_DURATION_SECONDS = 3LL * 24LL * 3600LL;

    struct ActivationResult
    {
        bool success = false;
        juce::String message;
    };

    LicenseManager();
    ~LicenseManager() = default;

    /** Refreshes trial timer, checks for clock rollback/debugger, and syncs all stealth anchors. */
    void verifyAndSyncState();

    /** Returns true if a valid full license is activated on this machine and passes canary verification. */
    bool isLicensed() const;

    /** Returns true if the 3-day trial is within the 72-hour window, not tampered, and passes canary check. */
    bool isTrialActive() const;

    /** Returns true if the plugin is authorized to process audio, extract, and export. */
    bool isUnlocked() const;

    /** Secondary independent integrity check used across DSP and service entry points to defeat 1-byte JMP patches. */
    bool verifyRuntimeIntegrity() const;

    /** Returns true if the user rolled back the system clock or froze time. */
    bool isClockTampered() const { return clockTamperedFlag.load(std::memory_order_relaxed); }

    /** Returns remaining trial seconds (0 if expired). */
    juce::int64 getRemainingTrialSeconds() const { return remainingTrialSeconds.load(std::memory_order_relaxed); }

    /** Returns this computer's unique Hardware Machine ID (e.g. "MCH-XXXX-XXXX-XXXX"). */
    juce::String getMachineId() const { return machineId; }

    /** Returns human-readable badge text for the header (e.g. "PRUEBA: 2d 18h restantes"). */
    juce::String getHeaderStatusBadgeText() const;

    /** Returns the activated license key if licensed. */
    juce::String getActivatedLicenseKey() const;

    /**
     * Attempts to activate the plugin with the provided license key.
     * Supports both Cryptographic Machine-Bound keys and Online Server DB keys.
     */
    ActivationResult activateLicense(const juce::String& licenseKeyInput,
                                     const juce::String& serverEndpointUrl = "http://127.0.0.1:8765/api/activate");

    /** Asynchronous heartbeat to verify that an activated license has not been revoked on the server. */
    void verifyOnlineRevocationHeartbeat(const juce::String& verifyEndpointUrl = "http://127.0.0.1:8765/api/verify");

    /** Deterministic 3-round cryptographic signature generator used by the verification engine. */
    static juce::String computeLicenseSignature(const juce::String& normalizedMachineId,
                                                const juce::String& tierCode,
                                                const juce::String& serialBlock);

    /** Verifies whether a license key matches this machine's Hardware ID signature. */
    static bool verifyCryptographicKeyForMachine(const juce::String& cleanKey,
                                                 const juce::String& targetMachineId,
                                                 juce::String& outErrorMsg);

    /** Detects if a user-mode or remote debugger is attached to the process. */
    static bool isDebuggerAttached();

private:
    struct VaultData
    {
        juce::int64 firstRunUtc = 0;
        juce::int64 lastSeenUtc = 0;
        bool clockTampered = false;
        juce::String activatedKey;
        juce::String boundMachineId;
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

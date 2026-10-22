#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>

/**
 * LicenseManager
 * 
 * Commercial Anti-Piracy & 3-Day Trial Security Engine for ClipSampler10.
 * 
 * Security Architecture:
 * 1. Hardware Fingerprinting (Machine ID):
 *    Combines Windows Cryptography MachineGuid, System Volume Serial Number, and CPU signature
 *    into a deterministic, tamper-resistant hardware identifier (e.g. "MCH-8F3A-92C1-4B0E").
 * 
 * 2. Dual-Anchor Encrypted Vault (3-Day Trial Protection):
 *    Persists first-install timestamp, last-seen timestamp, clock-tamper flag, and activated license
 *    simultaneously in:
 *      - Anchor A: Windows Registry (HKCU\Software\Classes\CLSID\{...})
 *      - Anchor B: Hidden encrypted vault files in ProgramData & AppData
 *    Deleting one anchor automatically restores from the other. Rolling back the system clock
 *    immediately invalidates the trial.
 * 
 * 3. Hybrid License Activation (Hardware-Bound Cryptographic Signature + Online DB Verification):
 *    - Validates cryptographically signed license keys tied to the client's Machine ID (offline/online).
 *    - Supports online activation & single-PC binding against the License Server / Cloud DB API.
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

    /** Refreshes trial timer, checks for clock rollback, and syncs dual anchors. */
    void verifyAndSyncState();

    /** Returns true if a valid full license is activated on this machine. */
    bool isLicensed() const { return licensedFlag.load(std::memory_order_relaxed); }

    /** Returns true if the 3-day trial is still within the 72-hour window and not tampered. */
    bool isTrialActive() const { return trialActiveFlag.load(std::memory_order_relaxed); }

    /** Returns true if the plugin is allowed to process audio and extract samples. */
    bool isUnlocked() const { return isLicensed() || isTrialActive(); }

    /** Returns true if the user rolled back the system clock. */
    bool isClockTampered() const { return clockTamperedFlag.load(std::memory_order_relaxed); }

    /** Returns remaining trial seconds (0 if expired). */
    juce::int64 getRemainingTrialSeconds() const { return remainingTrialSeconds.load(std::memory_order_relaxed); }

    /** Returns this computer's unique Hardware Machine ID (e.g. "MCH-XXXX-XXXX-XXXX"). */
    juce::String getMachineId() const { return machineId; }

    /** Returns human-readable badge text for the header (e.g. "PRUEBA: 2d 18h restantes"). */
    juce::String getHeaderStatusBadgeText() const;

    /** Returns the masked or full activated license key if licensed. */
    juce::String getActivatedLicenseKey() const;

    /**
     * Attempts to activate the plugin with the provided license key.
     * Supports both Cryptographic Machine-Bound keys and Online Server DB keys.
     */
    ActivationResult activateLicense(const juce::String& licenseKeyInput,
                                     const juce::String& serverEndpointUrl = "http://127.0.0.1:8765/api/activate");

    /** Deterministic cryptographic signature generator used by the verification engine. */
    static juce::String computeLicenseSignature(const juce::String& normalizedMachineId,
                                                const juce::String& tierCode,
                                                const juce::String& serialBlock);

    /** Verifies whether a license key matches this machine's Hardware ID signature. */
    static bool verifyCryptographicKeyForMachine(const juce::String& cleanKey,
                                                 const juce::String& targetMachineId,
                                                 juce::String& outErrorMsg);

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
    juce::String encryptVaultPayload(const VaultData& data) const;
    bool decryptVaultPayload(const juce::String& cipherHex, VaultData& outData) const;

    void loadAndMergeAnchors(VaultData& mergedData);
    void saveToAllAnchors(const VaultData& data) const;

    static juce::File getPrimaryVaultFile();
    static juce::File getSecondaryVaultFile();
    static juce::String getRegistryVaultPath();

    juce::String machineId;
    mutable juce::CriticalSection stateLock;
    juce::String currentActivatedKey;

    std::atomic<bool> licensedFlag { false };
    std::atomic<bool> trialActiveFlag { true };
    std::atomic<bool> clockTamperedFlag { false };
    std::atomic<juce::int64> remainingTrialSeconds { TRIAL_DURATION_SECONDS };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LicenseManager)
};

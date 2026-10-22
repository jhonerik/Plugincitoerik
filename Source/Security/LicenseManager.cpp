#include "LicenseManager.h"

#if JUCE_WINDOWS
 #define NOMINMAX
 #include <windows.h>
#endif

namespace
{
    // Master cryptographic signing secret shared ONLY with the developer's License Generator
    constexpr const char* kMasterSigningSecret = "CS10_MASTER_SIG_2026_AUDIOINNOVATIONS_9F8E7D6C5B4A392817263544";
    constexpr const char* kVaultEncryptionSalt = "CS10_VAULT_AES_SALT_7741_anti_reset_v2";
}

LicenseManager::LicenseManager()
    : machineId(buildHardwareMachineId())
{
    verifyAndSyncState();
}

juce::String LicenseManager::buildHardwareMachineId()
{
    juce::String rawComponents = "CS10_HW_";

#if JUCE_WINDOWS
    // 1. Read Windows MachineGuid from HKLM\SOFTWARE\Microsoft\Cryptography
    juce::String machineGuid = juce::WindowsRegistry::getValue(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\MachineGuid",
        {},
        juce::WindowsRegistry::WoW64_64bit);

    // 2. Read C:\ Volume Serial Number
    DWORD volumeSerial = 0;
    GetVolumeInformationW(L"C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0);

    rawComponents << machineGuid << "_" << juce::String::toHexString((juce::int64)volumeSerial);
#endif

    // 3. Combine with CPU vendor, model, and core count
    rawComponents << "_" << juce::SystemStats::getCpuVendor()
                  << "_" << juce::SystemStats::getCpuModel()
                  << "_" << juce::String(juce::SystemStats::getNumCpus());

    juce::SHA256 sha(rawComponents.toUTF8());
    juce::String hex = sha.toHexString().toUpperCase();

    // Format as MCH-XXXX-XXXX-XXXX (12 hex chars = 48-bit hardware fingerprint)
    return "MCH-" + hex.substring(0, 4) + "-" + hex.substring(4, 8) + "-" + hex.substring(8, 12);
}

juce::String LicenseManager::computeLicenseSignature(const juce::String& normalizedMachineId,
                                                     const juce::String& tierCode,
                                                     const juce::String& serialBlock)
{
    juce::String cleanMch = normalizedMachineId.trim().toUpperCase();
    juce::String cleanTier = tierCode.trim().toUpperCase();
    juce::String cleanSerial = serialBlock.trim().toUpperCase();

    juce::String payload = cleanMch + "|" + cleanTier + "|" + cleanSerial + "|" + kMasterSigningSecret;
    juce::SHA256 firstPass(payload.toUTF8());
    juce::String secondPayload = firstPass.toHexString().toUpperCase() + "|" + kMasterSigningSecret;
    juce::SHA256 secondPass(secondPayload.toUTF8());

    return secondPass.toHexString().toUpperCase().substring(0, 8);
}

bool LicenseManager::verifyCryptographicKeyForMachine(const juce::String& cleanKey,
                                                      const juce::String& targetMachineId,
                                                      juce::String& outErrorMsg)
{
    // Expected format:
    // CS10-<TIER/HW_HASH4>-<SERIAL4>-<SIG4_A>-<SIG4_B>
    // Example: CS10-8F3A-K92M-7C1D-E40B
    auto tokens = juce::StringArray::fromTokens(cleanKey.trim().toUpperCase(), "-", "");
    if (tokens.size() != 5 || tokens[0] != "CS10")
    {
        outErrorMsg = juce::String::fromUTF8("Formato inválido. Usa el formato: CS10-XXXX-XXXX-XXXX-XXXX");
        return false;
    }

    const juce::String block1 = tokens[1]; // 4-char hardware tag or "UNIV" (after online bind)
    const juce::String serialBlock = tokens[2]; // 4-char unique license serial
    const juce::String providedSig = tokens[3] + tokens[4]; // 8-char cryptographic signature

    if (block1.length() != 4 || serialBlock.length() != 4 || providedSig.length() != 8)
    {
        outErrorMsg = juce::String::fromUTF8("Longitud de bloques de licencia incorrecta.");
        return false;
    }

    // Extract the 4-char hardware tag from targetMachineId ("MCH-ABCD-1234-5678" -> "ABCD")
    auto mchTokens = juce::StringArray::fromTokens(targetMachineId.toUpperCase(), "-", "");
    juce::String expectedHwTag = (mchTokens.size() >= 2) ? mchTokens[1] : "0000";

    // Check if the key is bound to a different machine ID
    if (block1 != expectedHwTag)
    {
        outErrorMsg = juce::String::fromUTF8("Esta licencia pertenece a otra computadora y no puede usarse en este equipo (ID: ")
                    + targetMachineId + ").";
        return false;
    }

    const juce::String expectedSig = computeLicenseSignature(targetMachineId, block1, serialBlock);
    if (providedSig != expectedSig)
    {
        outErrorMsg = juce::String::fromUTF8("Código de licencia inválido o firma criptográfica corrupta.");
        return false;
    }

    return true;
}

juce::File LicenseManager::getPrimaryVaultFile()
{
    auto commonDir = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
                         .getChildFile("ClipSampler10");
    commonDir.createDirectory();
    return commonDir.getChildFile(".cs10_sec_vault.dat");
}

juce::File LicenseManager::getSecondaryVaultFile()
{
    auto userDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    userDir = userDir.getChildFile("Application Support");
#endif
    auto dir = userDir.getChildFile("ClipSampler10");
    dir.createDirectory();
    return dir.getChildFile(".cs10_sec_vault.dat");
}

juce::String LicenseManager::getRegistryVaultPath()
{
    return "HKEY_CURRENT_USER\\Software\\Classes\\CLSID\\{C78912A3-B245-429C-8A91-58FF807B4391}\\VaultState";
}

juce::String LicenseManager::encryptVaultPayload(const VaultData& data) const
{
    juce::ValueTree vt("Vault");
    vt.setProperty("f", data.firstRunUtc, nullptr);
    vt.setProperty("l", data.lastSeenUtc, nullptr);
    vt.setProperty("t", data.clockTampered ? 1 : 0, nullptr);
    vt.setProperty("k", data.activatedKey, nullptr);
    vt.setProperty("m", data.boundMachineId, nullptr);

    juce::String xmlStr = vt.toXmlString(juce::XmlElement::TextFormat().singleLine());

    // Compute integrity MAC over plaintext + machineId + salt
    juce::String macInput = xmlStr + "|" + machineId + "|" + kVaultEncryptionSalt;
    juce::String macHex = juce::SHA256(macInput.toUTF8()).toHexString().substring(0, 16);

    juce::String combined = macHex + "|" + xmlStr;
    juce::MemoryBlock mb(combined.toRawUTF8(), combined.getNumBytesAsUTF8());

    // Derive key stream from machineId + salt
    juce::SHA256 keyHash((machineId + kVaultEncryptionSalt).toUTF8());
    auto keyBytes = keyHash.getRawData();
    const uint8_t* keyPtr = static_cast<const uint8_t*>(keyBytes.getData());
    const size_t keyLen = keyBytes.getSize();

    uint8_t* dataPtr = static_cast<uint8_t*>(mb.getData());
    for (size_t i = 0; i < mb.getSize(); ++i)
        dataPtr[i] ^= keyPtr[i % keyLen] ^ static_cast<uint8_t>((i * 31) & 0xFF);

    return juce::String::toHexString(mb.getData(), (int)mb.getSize(), 0);
}

bool LicenseManager::decryptVaultPayload(const juce::String& cipherHex, VaultData& outData) const
{
    if (cipherHex.trim().isEmpty())
        return false;

    juce::MemoryBlock mb;
    mb.loadFromHexString(cipherHex.trim());
    if (mb.getSize() < 18)
        return false;

    juce::SHA256 keyHash((machineId + kVaultEncryptionSalt).toUTF8());
    auto keyBytes = keyHash.getRawData();
    const uint8_t* keyPtr = static_cast<const uint8_t*>(keyBytes.getData());
    const size_t keyLen = keyBytes.getSize();

    uint8_t* dataPtr = static_cast<uint8_t*>(mb.getData());
    for (size_t i = 0; i < mb.getSize(); ++i)
        dataPtr[i] ^= keyPtr[i % keyLen] ^ static_cast<uint8_t>((i * 31) & 0xFF);

    juce::String combined = juce::String::fromUTF8(static_cast<const char*>(mb.getData()), (int)mb.getSize());
    int sepIdx = combined.indexOfChar('|');
    if (sepIdx != 16)
        return false;

    juce::String storedMac = combined.substring(0, 16);
    juce::String xmlStr = combined.substring(17);

    juce::String expectedMac = juce::SHA256((xmlStr + "|" + machineId + "|" + kVaultEncryptionSalt).toUTF8())
                                   .toHexString()
                                   .substring(0, 16);
    if (storedMac != expectedMac)
        return false;

    auto vt = juce::ValueTree::fromXml(xmlStr);
    if (!vt.isValid() || !vt.hasType("Vault"))
        return false;

    outData.firstRunUtc = static_cast<juce::int64>(vt.getProperty("f", 0));
    outData.lastSeenUtc = static_cast<juce::int64>(vt.getProperty("l", 0));
    outData.clockTampered = static_cast<int>(vt.getProperty("t", 0)) != 0;
    outData.activatedKey = vt.getProperty("k", "").toString();
    outData.boundMachineId = vt.getProperty("m", "").toString();
    return outData.firstRunUtc > 0;
}

void LicenseManager::loadAndMergeAnchors(VaultData& mergedData)
{
    std::vector<VaultData> foundAnchors;

#if JUCE_WINDOWS
    juce::String regHex = juce::WindowsRegistry::getValue(getRegistryVaultPath(), {});
    VaultData regData;
    if (decryptVaultPayload(regHex, regData))
        foundAnchors.push_back(regData);
#endif

    auto loadFileAnchor = [&](const juce::File& f) {
        if (f.existsAsFile())
        {
            VaultData fileData;
            if (decryptVaultPayload(f.loadFileAsString(), fileData))
                foundAnchors.push_back(fileData);
        }
    };

    loadFileAnchor(getPrimaryVaultFile());
    loadFileAnchor(getSecondaryVaultFile());

    const juce::int64 nowSec = juce::Time::getCurrentTime().toMilliseconds() / 1000LL;

    if (foundAnchors.empty())
    {
        // First time ever running ClipSampler10 on this machine: start 3-day trial
        mergedData.firstRunUtc = nowSec;
        mergedData.lastSeenUtc = nowSec;
        mergedData.clockTampered = false;
        mergedData.activatedKey = {};
        mergedData.boundMachineId = machineId;
        return;
    }

    // Anti-reset merge: take the OLDEST firstRunUtc and NEWEST lastSeenUtc across all anchors
    mergedData = foundAnchors[0];
    for (const auto& anchor : foundAnchors)
    {
        if (anchor.firstRunUtc > 0 && anchor.firstRunUtc < mergedData.firstRunUtc)
            mergedData.firstRunUtc = anchor.firstRunUtc;

        if (anchor.lastSeenUtc > mergedData.lastSeenUtc)
            mergedData.lastSeenUtc = anchor.lastSeenUtc;

        if (anchor.clockTampered)
            mergedData.clockTampered = true;

        if (mergedData.activatedKey.isEmpty() && anchor.activatedKey.isNotEmpty())
        {
            mergedData.activatedKey = anchor.activatedKey;
            mergedData.boundMachineId = anchor.boundMachineId;
        }
    }
}

void LicenseManager::saveToAllAnchors(const VaultData& data) const
{
    const juce::String encryptedHex = encryptVaultPayload(data);

#if JUCE_WINDOWS
    juce::WindowsRegistry::setValue(getRegistryVaultPath(), encryptedHex);
#endif

    getPrimaryVaultFile().replaceWithText(encryptedHex);
    getSecondaryVaultFile().replaceWithText(encryptedHex);
}

void LicenseManager::verifyAndSyncState()
{
    VaultData data;
    loadAndMergeAnchors(data);

    const juce::int64 nowSec = juce::Time::getCurrentTime().toMilliseconds() / 1000LL;

    // Detect system clock rollback (allow 15-minute tolerance for minor NTP drift)
    if (nowSec + 900 < data.lastSeenUtc)
    {
        data.clockTampered = true;
    }
    else if (nowSec > data.lastSeenUtc)
    {
        data.lastSeenUtc = nowSec;
    }

    // Verify if stored license key is valid for this machine
    bool validLicense = false;
    if (data.activatedKey.isNotEmpty() && data.boundMachineId == machineId)
    {
        juce::String dummyErr;
        validLicense = verifyCryptographicKeyForMachine(data.activatedKey, machineId, dummyErr);
    }

    // Calculate 3-day trial remaining time
    const juce::int64 elapsedSec = juce::jmax<juce::int64>(0, nowSec - data.firstRunUtc);
    const juce::int64 remSec = data.clockTampered
        ? 0
        : juce::jmax<juce::int64>(0, TRIAL_DURATION_SECONDS - elapsedSec);

    {
        const juce::ScopedLock sl(stateLock);
        currentActivatedKey = validLicense ? data.activatedKey : juce::String();
    }

    licensedFlag.store(validLicense, std::memory_order_relaxed);
    clockTamperedFlag.store(data.clockTampered, std::memory_order_relaxed);
    remainingTrialSeconds.store(remSec, std::memory_order_relaxed);
    trialActiveFlag.store(!data.clockTampered && remSec > 0, std::memory_order_relaxed);

    // Sync back to all anchors so deleting one file never resets the trial
    saveToAllAnchors(data);
}

LicenseManager::ActivationResult LicenseManager::activateLicense(const juce::String& licenseKeyInput,
                                                                 const juce::String& serverEndpointUrl)
{
    juce::String cleanKey = licenseKeyInput.trim().toUpperCase();
    if (cleanKey.isEmpty())
    {
        return { false, juce::String::fromUTF8("Por favor ingresa un código de licencia.") };
    }

    juce::String finalBoundKey = cleanKey;

    // 1. If the key is a Universal/Web Auto-Bind key (CS10-AUTO-XXXX-XXXX-XXXX),
    //    contact the License Server DB to bind it to this Machine ID on first use
    auto tokens = juce::StringArray::fromTokens(cleanKey, "-", "");
    if (tokens.size() == 5 && tokens[0] == "CS10" && tokens[1] == "AUTO")
    {
        juce::URL url(serverEndpointUrl);
        juce::DynamicObject::Ptr bodyObj = new juce::DynamicObject();
        bodyObj->setProperty("license_key", cleanKey);
        bodyObj->setProperty("machine_id", machineId);
        juce::String jsonBody = juce::JSON::toString(juce::var(bodyObj.get()), true);

        url = url.withPOSTData(jsonBody);
        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                           .withExtraHeaders("Content-Type: application/json\r\n")
                           .withConnectionTimeoutMs(6000);

        std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);
        if (stream == nullptr)
        {
            return { false, juce::String::fromUTF8("No se pudo conectar al servidor de licencias para vincular esta clave AUTO. Usa una clave vinculada a tu ID de Máquina o inicia el servidor de licencias.") };
        }

        juce::String responseStr = stream->readEntireStreamAsString();
        juce::var parsed = juce::JSON::parse(responseStr);
        if (auto* obj = parsed.getDynamicObject())
        {
            bool ok = static_cast<bool>(obj->getProperty("success"));
            juce::String msg = obj->getProperty("message").toString();
            juce::String boundKey = obj->getProperty("bound_license_key").toString();

            if (!ok || boundKey.isEmpty())
            {
                return { false, msg.isNotEmpty() ? msg : juce::String::fromUTF8("El servidor rechazó la activación de la licencia.") };
            }

            finalBoundKey = boundKey.trim().toUpperCase();
        }
        else
        {
            return { false, juce::String::fromUTF8("Respuesta inválida del servidor de licencias.") };
        }
    }
    else
    {
        // Also notify online server asynchronously if reachable so the Admin DB records the activation timestamp & Machine ID
        juce::String mchCopy = machineId;
        juce::String keyCopy = cleanKey;
        juce::String endpointCopy = serverEndpointUrl;
        juce::Thread::launch([endpointCopy, keyCopy, mchCopy] {
            juce::URL url(endpointCopy);
            juce::DynamicObject::Ptr bodyObj = new juce::DynamicObject();
            bodyObj->setProperty("license_key", keyCopy);
            bodyObj->setProperty("machine_id", mchCopy);
            url = url.withPOSTData(juce::JSON::toString(juce::var(bodyObj.get()), true));
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                               .withExtraHeaders("Content-Type: application/json\r\n")
                               .withConnectionTimeoutMs(2500);
            std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);
            if (stream != nullptr)
                stream->readEntireStreamAsString();
        });
    }

    // 2. Cryptographically verify the key against this computer's Hardware Machine ID
    juce::String errorMsg;
    if (!verifyCryptographicKeyForMachine(finalBoundKey, machineId, errorMsg))
    {
        return { false, errorMsg };
    }

    // 3. Persist activated license into all encrypted anchors
    VaultData data;
    loadAndMergeAnchors(data);
    data.activatedKey = finalBoundKey;
    data.boundMachineId = machineId;
    saveToAllAnchors(data);

    verifyAndSyncState();
    return { true, juce::String::fromUTF8("¡Licencia verificada y activada permanentemente en este equipo!") };
}

juce::String LicenseManager::getHeaderStatusBadgeText() const
{
    if (isLicensed())
        return juce::String::fromUTF8("✓ LICENCIA ACTIVA [PRO]");

    if (isClockTampered())
        return juce::String::fromUTF8("🔒 RELOJ ALTERADO - ACTIVAR");

    const juce::int64 rem = getRemainingTrialSeconds();
    if (rem <= 0)
        return juce::String::fromUTF8("🔒 PRUEBA VENCIDA - ACTIVAR");

    const juce::int64 days = rem / 86400;
    const juce::int64 hours = (rem % 86400) / 3600;
    const juce::int64 mins = (rem % 3600) / 60;

    if (days > 0)
        return juce::String::fromUTF8("⏳ PRUEBA (3 DÍAS): ") + juce::String(days) + "d " + juce::String(hours) + "h [Activar]";

    return juce::String::fromUTF8("⏳ PRUEBA: ") + juce::String(hours) + "h " + juce::String(mins) + "m [Activar]";
}

juce::String LicenseManager::getActivatedLicenseKey() const
{
    const juce::ScopedLock sl(stateLock);
    return currentActivatedKey;
}

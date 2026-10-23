#include "LicenseManager.h"

#if JUCE_WINDOWS
 #define NOMINMAX
 #include <windows.h>
#endif

// ==============================================================================
// Utilidades Internas de Ofuscación Criptográfica y Atributos de Sistema
// ==============================================================================
namespace
{
    // Macro XOR en tiempo de compilación: impide que las claves secretas aparezcan en texto plano en el binario .vst3
    #define CS_OBF_BYTE(ch, idx) static_cast<uint8_t>(static_cast<uint8_t>(ch) ^ static_cast<uint8_t>((((idx) * 37) + 0x6B) & 0xFF))

    // Representación ofuscada del secreto maestro de firma (62 bytes)
    static constexpr uint8_t kObfMasterSecret[62] = {
        CS_OBF_BYTE('C', 0),  CS_OBF_BYTE('S', 1),  CS_OBF_BYTE('1', 2),  CS_OBF_BYTE('0', 3),
        CS_OBF_BYTE('_', 4),  CS_OBF_BYTE('M', 5),  CS_OBF_BYTE('A', 6),  CS_OBF_BYTE('S', 7),
        CS_OBF_BYTE('T', 8),  CS_OBF_BYTE('E', 9),  CS_OBF_BYTE('R', 10), CS_OBF_BYTE('_', 11),
        CS_OBF_BYTE('S', 12), CS_OBF_BYTE('I', 13), CS_OBF_BYTE('G', 14), CS_OBF_BYTE('_', 15),
        CS_OBF_BYTE('2', 16), CS_OBF_BYTE('0', 17), CS_OBF_BYTE('2', 18), CS_OBF_BYTE('6', 19),
        CS_OBF_BYTE('_', 20), CS_OBF_BYTE('A', 21), CS_OBF_BYTE('U', 22), CS_OBF_BYTE('D', 23),
        CS_OBF_BYTE('I', 24), CS_OBF_BYTE('O', 25), CS_OBF_BYTE('I', 26), CS_OBF_BYTE('N', 27),
        CS_OBF_BYTE('N', 28), CS_OBF_BYTE('O', 29), CS_OBF_BYTE('V', 30), CS_OBF_BYTE('A', 31),
        CS_OBF_BYTE('T', 32), CS_OBF_BYTE('I', 33), CS_OBF_BYTE('O', 34), CS_OBF_BYTE('N', 35),
        CS_OBF_BYTE('S', 36), CS_OBF_BYTE('_', 37), CS_OBF_BYTE('9', 38), CS_OBF_BYTE('F', 39),
        CS_OBF_BYTE('8', 40), CS_OBF_BYTE('E', 41), CS_OBF_BYTE('7', 42), CS_OBF_BYTE('D', 43),
        CS_OBF_BYTE('6', 44), CS_OBF_BYTE('C', 45), CS_OBF_BYTE('5', 46), CS_OBF_BYTE('B', 47),
        CS_OBF_BYTE('4', 48), CS_OBF_BYTE('A', 49), CS_OBF_BYTE('3', 50), CS_OBF_BYTE('9', 51),
        CS_OBF_BYTE('2', 52), CS_OBF_BYTE('8', 53), CS_OBF_BYTE('1', 54), CS_OBF_BYTE('7', 55),
        CS_OBF_BYTE('2', 56), CS_OBF_BYTE('6', 57), CS_OBF_BYTE('3', 58), CS_OBF_BYTE('5', 59),
        CS_OBF_BYTE('4', 60), CS_OBF_BYTE('4', 61)
    };

    // Representación ofuscada de la sal de cifrado de la bóveda local (38 bytes)
    static constexpr uint8_t kObfVaultSalt[38] = {
        CS_OBF_BYTE('C', 0),  CS_OBF_BYTE('S', 1),  CS_OBF_BYTE('1', 2),  CS_OBF_BYTE('0', 3),
        CS_OBF_BYTE('_', 4),  CS_OBF_BYTE('V', 5),  CS_OBF_BYTE('A', 6),  CS_OBF_BYTE('U', 7),
        CS_OBF_BYTE('L', 8),  CS_OBF_BYTE('T', 9),  CS_OBF_BYTE('_', 10), CS_OBF_BYTE('A', 11),
        CS_OBF_BYTE('E', 12), CS_OBF_BYTE('S', 13), CS_OBF_BYTE('_', 14), CS_OBF_BYTE('S', 15),
        CS_OBF_BYTE('A', 16), CS_OBF_BYTE('L', 17), CS_OBF_BYTE('T', 18), CS_OBF_BYTE('_', 19),
        CS_OBF_BYTE('7', 20), CS_OBF_BYTE('7', 21), CS_OBF_BYTE('4', 22), CS_OBF_BYTE('1', 23),
        CS_OBF_BYTE('_', 24), CS_OBF_BYTE('a', 25), CS_OBF_BYTE('n', 26), CS_OBF_BYTE('t', 27),
        CS_OBF_BYTE('i', 28), CS_OBF_BYTE('_', 29), CS_OBF_BYTE('r', 30), CS_OBF_BYTE('e', 31),
        CS_OBF_BYTE('s', 32), CS_OBF_BYTE('e', 33), CS_OBF_BYTE('t', 34), CS_OBF_BYTE('_', 35),
        CS_OBF_BYTE('v', 36), CS_OBF_BYTE('2', 37)
    };

    /** Reconstruye temporalmente en el stack una cadena ofuscada y limpia el buffer inmediatamente. */
    juce::String decodeObfuscatedBytes(const uint8_t* data, size_t len)
    {
        char stackBuf[128] = { 0 };
        const size_t safeLen = juce::jmin(len, sizeof(stackBuf) - 1);
        for (size_t i = 0; i < safeLen; ++i)
            stackBuf[i] = static_cast<char>(data[i] ^ static_cast<uint8_t>((((i) * 37) + 0x6B) & 0xFF));
        stackBuf[safeLen] = '\0';
        juce::String result(stackBuf);
        juce::zeromem(stackBuf, sizeof(stackBuf));
        return result;
    }

    /** Marca un archivo de bóveda como oculto en Windows para protegerlo frente a borrados manuales. */
    void markFileHiddenOnWindows(const juce::File& f)
    {
    #if JUCE_WINDOWS
        if (f.existsAsFile())
        {
            DWORD attrs = GetFileAttributesW(f.getFullPathName().toWideCharPointer());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_HIDDEN) == 0)
                SetFileAttributesW(f.getFullPathName().toWideCharPointer(), attrs | FILE_ATTRIBUTE_HIDDEN);
        }
    #else
        juce::ignoreUnused(f);
    #endif
    }

    /** Migra de forma transparente la carpeta antigua ClipSampler10 a ClipSampler si existía en el sistema. */
    void migrateLegacyFolderIfNeeded(const juce::File& parentDir)
    {
        auto oldDir = parentDir.getChildFile("ClipSampler10");
        auto newDir = parentDir.getChildFile("ClipSampler");
        if (oldDir.isDirectory())
        {
            newDir.createDirectory();
            auto oldVault = oldDir.getChildFile(".cs10_sec_vault.dat");
            auto newVault = newDir.getChildFile(".cs10_sec_vault.dat");
            if (oldVault.existsAsFile() && !newVault.existsAsFile())
                oldVault.copyFileTo(newVault);
        }
    }
}

// ==============================================================================
// Constructor e Inicialización de Semilla Criptográfica
// ==============================================================================
LicenseManager::LicenseManager()
    : machineId(buildHardwareMachineId())
{
    // Derivamos una semilla de 64 bits única a partir del Hardware ID de este equipo
    juce::SHA256 seedHash((machineId + "_CANARY_V24").toUTF8());
    const auto* raw = static_cast<const uint8_t*>(seedHash.getRawData().getData());
    machineCanarySeed = 0;
    for (int i = 0; i < 8; ++i)
        machineCanarySeed = (machineCanarySeed << 8) | static_cast<juce::uint64>(raw[i]);

    verifyAndSyncState();
}

// ==============================================================================
// Verificaciones Anti-Debugger e Integridad de Canario en RAM
// ==============================================================================
bool LicenseManager::isDebuggerAttached()
{
#if JUCE_WINDOWS
    if (IsDebuggerPresent() != FALSE)
        return true;

    BOOL remoteDbg = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDbg) != FALSE && remoteDbg != FALSE)
        return true;
#endif
    return false;
}

juce::uint64 LicenseManager::computeExpectedCanary(bool unlockedState) const
{
    if (!unlockedState)
        return machineCanarySeed ^ 0xDEAD10CC55AA0011ULL;
    return (machineCanarySeed ^ 0xA55A9966F00DCAFEULL) + 0x1337C0DEULL;
}

bool LicenseManager::isLicensed() const
{
    if (!licensedFlag.load(std::memory_order_relaxed))
        return false;
    return runtimeCanaryToken.load(std::memory_order_relaxed) == computeExpectedCanary(true);
}

bool LicenseManager::isTrialActive() const
{
    if (!trialActiveFlag.load(std::memory_order_relaxed) || clockTamperedFlag.load(std::memory_order_relaxed))
        return false;
    if (remainingTrialSeconds.load(std::memory_order_relaxed) <= 0)
        return false;
    return runtimeCanaryToken.load(std::memory_order_relaxed) == computeExpectedCanary(true);
}

bool LicenseManager::isUnlocked() const
{
    return (isLicensed() || isTrialActive()) && !isDebuggerAttached();
}

bool LicenseManager::verifyRuntimeIntegrity() const
{
    if (isDebuggerAttached())
        return false;

    const juce::uint64 currentCanary = runtimeCanaryToken.load(std::memory_order_relaxed);
    const juce::uint64 validCanary = computeExpectedCanary(true);
    if (currentCanary != validCanary)
        return false;

    return licensedFlag.load(std::memory_order_relaxed)
        || (trialActiveFlag.load(std::memory_order_relaxed)
            && !clockTamperedFlag.load(std::memory_order_relaxed)
            && remainingTrialSeconds.load(std::memory_order_relaxed) > 0);
}

// ==============================================================================
// Generación de Huella de Hardware (Machine ID) y Firma Criptográfica de 3 Rondas
// ==============================================================================
juce::String LicenseManager::buildHardwareMachineId()
{
    juce::String rawComponents = "CS10_HW_";

#if JUCE_WINDOWS
    // 1. Leemos el MachineGuid oficial de Windows desde el registro de 64 bits
    juce::String machineGuid = juce::WindowsRegistry::getValue(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\MachineGuid",
        {},
        juce::WindowsRegistry::WoW64_64bit);

    // 2. Leemos el número de serie del volumen del disco principal C:\ 
    DWORD volumeSerial = 0;
    GetVolumeInformationW(L"C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0);

    rawComponents << machineGuid << "_" << juce::String::toHexString((juce::int64)volumeSerial);
#endif

    // 3. Combinamos con fabricante/modelo/núcleos del procesador y nombre del equipo
    rawComponents << "_" << juce::SystemStats::getCpuVendor()
                  << "_" << juce::SystemStats::getCpuModel()
                  << "_" << juce::String(juce::SystemStats::getNumCpus())
                  << "_" << juce::SystemStats::getComputerName();

    juce::SHA256 sha(rawComponents.toUTF8());
    juce::String hex = sha.toHexString().toUpperCase();

    // Formato estándar: MCH-XXXX-XXXX-XXXX
    return "MCH-" + hex.substring(0, 4) + "-" + hex.substring(4, 8) + "-" + hex.substring(8, 12);
}

juce::String LicenseManager::computeLicenseSignature(const juce::String& normalizedMachineId,
                                                     const juce::String& tierCode,
                                                     const juce::String& serialBlock)
{
    juce::String cleanMch = normalizedMachineId.trim().toUpperCase();
    juce::String cleanTier = tierCode.trim().toUpperCase();
    juce::String cleanSerial = serialBlock.trim().toUpperCase();

    juce::String masterSecret = decodeObfuscatedBytes(kObfMasterSecret, sizeof(kObfMasterSecret));

    // Ronda 1
    juce::String payload1 = cleanMch + "|" + cleanTier + "|" + cleanSerial + "|" + masterSecret;
    juce::String pass1 = juce::SHA256(payload1.toUTF8()).toHexString().toUpperCase();

    // Ronda 2
    juce::String payload2 = pass1 + "|" + masterSecret + "|" + cleanMch;
    juce::String pass2 = juce::SHA256(payload2.toUTF8()).toHexString().toUpperCase();

    // Ronda 3
    juce::String payload3 = pass2 + "|V24_HARDENED|" + cleanSerial + "|" + masterSecret;
    juce::String pass3 = juce::SHA256(payload3.toUTF8()).toHexString().toUpperCase();

    return pass3.substring(0, 8);
}

bool LicenseManager::verifyCryptographicKeyForMachine(const juce::String& cleanKey,
                                                      const juce::String& targetMachineId,
                                                      juce::String& outErrorMsg)
{
    if (isDebuggerAttached())
    {
        outErrorMsg = juce::String::fromUTF8("Error de seguridad: cierra cualquier depurador activo.");
        return false;
    }

    auto tokens = juce::StringArray::fromTokens(cleanKey.trim().toUpperCase(), "-", "");
    if (tokens.size() != 5 || tokens[0] != "CS10")
    {
        outErrorMsg = juce::String::fromUTF8("Formato inválido. Usa el formato: CS10-XXXX-XXXX-XXXX-XXXX");
        return false;
    }

    const juce::String block1 = tokens[1];
    const juce::String serialBlock = tokens[2];
    const juce::String providedSig = tokens[3] + tokens[4];

    if (block1.length() != 4 || serialBlock.length() != 4 || providedSig.length() != 8)
    {
        outErrorMsg = juce::String::fromUTF8("Longitud de bloques de licencia incorrecta.");
        return false;
    }

    auto mchTokens = juce::StringArray::fromTokens(targetMachineId.toUpperCase(), "-", "");
    juce::String expectedHwTag = (mchTokens.size() >= 2) ? mchTokens[1] : "0000";

    if (block1 != expectedHwTag)
    {
        outErrorMsg = juce::String::fromUTF8("Esta licencia pertenece a otra computadora y no puede usarse en este equipo (ID: ")
                    + targetMachineId + ").";
        return false;
    }

    const juce::String expectedSig = computeLicenseSignature(targetMachineId, block1, serialBlock);

    // Comparación en tiempo constante para evitar ataques de temporización (timing attacks)
    int diff = 0;
    for (int i = 0; i < 8; ++i)
        diff |= (int)(providedSig[i] ^ expectedSig[i]);

    if (diff != 0)
    {
        outErrorMsg = juce::String::fromUTF8("Código de licencia inválido o firma criptográfica no autorizada.");
        return false;
    }

    return true;
}

// ==============================================================================
// Rutas de los 5 Anclajes de la Bóveda Cifrada (Carpeta Oficial: ClipSampler)
// ==============================================================================
juce::File LicenseManager::getPrimaryVaultFile()
{
    auto baseCommon = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory);
    migrateLegacyFolderIfNeeded(baseCommon);
    auto commonDir = baseCommon.getChildFile("ClipSampler");
    commonDir.createDirectory();
    return commonDir.getChildFile(".cs10_sec_vault.dat");
}

juce::File LicenseManager::getSecondaryVaultFile()
{
    auto userDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    userDir = userDir.getChildFile("Application Support");
#endif
    migrateLegacyFolderIfNeeded(userDir);
    auto dir = userDir.getChildFile("ClipSampler");
    dir.createDirectory();
    return dir.getChildFile(".cs10_sec_vault.dat");
}

juce::File LicenseManager::getStealthTertiaryVaultFile() const
{
    // Anclaje camuflado en caché del sistema para impedir el reseteo de los 3 días de prueba
    auto localAppDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getParentDirectory()
                           .getChildFile("Local")
                           .getChildFile("Microsoft")
                           .getChildFile("CLR_v4.0_Cache");
    if (!localAppDir.exists())
        localAppDir.createDirectory();

    juce::String hwTag = machineId.substring(4, 8).toLowerCase();
    return localAppDir.getChildFile(".sys_drv_" + hwTag + ".bin");
}

juce::String LicenseManager::getRegistryVaultPathPrimary()
{
    return "HKEY_CURRENT_USER\\Software\\Classes\\CLSID\\{C78912A3-B245-429C-8A91-58FF807B4391}\\VaultState";
}

juce::String LicenseManager::getRegistryVaultPathSecondary()
{
    return "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SessionInfo\\CS_AudioVault";
}

// ==============================================================================
// Cifrado, Descifrado y Sincronización Cruzada de la Bóveda
// ==============================================================================
juce::String LicenseManager::encryptVaultPayload(const VaultData& data) const
{
    juce::String vaultSalt = decodeObfuscatedBytes(kObfVaultSalt, sizeof(kObfVaultSalt));

    juce::ValueTree vt("Vault");
    vt.setProperty("f", data.firstRunUtc, nullptr);
    vt.setProperty("l", data.lastSeenUtc, nullptr);
    vt.setProperty("t", data.clockTampered ? 1 : 0, nullptr);
    vt.setProperty("k", data.activatedKey, nullptr);
    vt.setProperty("m", data.boundMachineId, nullptr);

    juce::String xmlStr = vt.toXmlString(juce::XmlElement::TextFormat().singleLine());

    juce::String macInput = xmlStr + "|" + machineId + "|" + vaultSalt;
    juce::String macHex = juce::SHA256(macInput.toUTF8()).toHexString().substring(0, 16);

    juce::String combined = macHex + "|" + xmlStr;
    juce::MemoryBlock mb(combined.toRawUTF8(), combined.getNumBytesAsUTF8());

    juce::SHA256 keyHash((machineId + vaultSalt).toUTF8());
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

    juce::String vaultSalt = decodeObfuscatedBytes(kObfVaultSalt, sizeof(kObfVaultSalt));

    juce::SHA256 keyHash((machineId + vaultSalt).toUTF8());
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

    juce::String expectedMac = juce::SHA256((xmlStr + "|" + machineId + "|" + vaultSalt).toUTF8())
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
    juce::int64 latestFileModSec = 0;

#if JUCE_WINDOWS
    for (const auto& regPath : { getRegistryVaultPathPrimary(), getRegistryVaultPathSecondary() })
    {
        juce::String regHex = juce::WindowsRegistry::getValue(regPath, {});
        VaultData regData;
        if (decryptVaultPayload(regHex, regData))
            foundAnchors.push_back(regData);
    }
#endif

    auto loadFileAnchor = [&](const juce::File& f) {
        if (f.existsAsFile())
        {
            juce::int64 fileMod = f.getLastModificationTime().toMilliseconds() / 1000LL;
            if (fileMod > latestFileModSec)
                latestFileModSec = fileMod;

            VaultData fileData;
            if (decryptVaultPayload(f.loadFileAsString(), fileData))
                foundAnchors.push_back(fileData);
        }
    };

    loadFileAnchor(getPrimaryVaultFile());
    loadFileAnchor(getSecondaryVaultFile());
    loadFileAnchor(getStealthTertiaryVaultFile());

    const juce::int64 nowSec = juce::Time::getCurrentTime().toMilliseconds() / 1000LL;

    if (foundAnchors.empty())
    {
        mergedData.firstRunUtc = nowSec;
        mergedData.lastSeenUtc = nowSec;
        mergedData.clockTampered = false;
        mergedData.activatedKey = {};
        mergedData.boundMachineId = machineId;
        return;
    }

    // Fusión anti-reseteo: conserva la fecha de inicio MÁS ANTIGUA y la última vista MÁS RECIENTE
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

    // Monotonicidad de fechas del sistema de archivos (neutraliza herramientas tipo RunAsDate)
    if (latestFileModSec > mergedData.lastSeenUtc)
        mergedData.lastSeenUtc = latestFileModSec;
}

void LicenseManager::saveToAllAnchors(const VaultData& data) const
{
    const juce::String encryptedHex = encryptVaultPayload(data);

#if JUCE_WINDOWS
    juce::WindowsRegistry::setValue(getRegistryVaultPathPrimary(), encryptedHex);
    juce::WindowsRegistry::setValue(getRegistryVaultPathSecondary(), encryptedHex);
#endif

    auto f1 = getPrimaryVaultFile();
    auto f2 = getSecondaryVaultFile();
    auto f3 = getStealthTertiaryVaultFile();

    f1.replaceWithText(encryptedHex);
    f2.replaceWithText(encryptedHex);
    f3.replaceWithText(encryptedHex);

    markFileHiddenOnWindows(f1);
    markFileHiddenOnWindows(f2);
    markFileHiddenOnWindows(f3);
}

void LicenseManager::verifyAndSyncState()
{
    VaultData data;
    loadAndMergeAnchors(data);

    const juce::int64 nowSec = juce::Time::getCurrentTime().toMilliseconds() / 1000LL;

    // Detección de retroceso del reloj del sistema (tolerancia máxima de 5 minutos para ajustes NTP)
    if (nowSec + 300 < data.lastSeenUtc)
    {
        data.clockTampered = true;
    }
    else if (nowSec > data.lastSeenUtc)
    {
        data.lastSeenUtc = nowSec;
    }

    bool validLicense = false;
    if (data.activatedKey.isNotEmpty() && data.boundMachineId == machineId)
    {
        juce::String dummyErr;
        validLicense = verifyCryptographicKeyForMachine(data.activatedKey, machineId, dummyErr);
    }

    const juce::int64 elapsedSec = juce::jmax<juce::int64>(0, nowSec - data.firstRunUtc);
    const juce::int64 remSec = data.clockTampered
        ? 0
        : juce::jmax<juce::int64>(0, TRIAL_DURATION_SECONDS - elapsedSec);

    {
        const juce::ScopedLock sl(stateLock);
        currentActivatedKey = validLicense ? data.activatedKey : juce::String();
    }

    const bool trialOk = (!data.clockTampered && remSec > 0);
    const bool unlocked = (validLicense || trialOk) && !isDebuggerAttached();

    licensedFlag.store(validLicense, std::memory_order_relaxed);
    clockTamperedFlag.store(data.clockTampered, std::memory_order_relaxed);
    remainingTrialSeconds.store(remSec, std::memory_order_relaxed);
    trialActiveFlag.store(trialOk, std::memory_order_relaxed);
    runtimeCanaryToken.store(computeExpectedCanary(unlocked), std::memory_order_relaxed);

    saveToAllAnchors(data);

    if (validLicense)
        verifyOnlineRevocationHeartbeat();
}

void LicenseManager::verifyOnlineRevocationHeartbeat(const juce::String& verifyEndpointUrl)
{
    juce::String keyToCheck = getActivatedLicenseKey();
    if (keyToCheck.isEmpty() || heartbeatInProgress.exchange(true))
        return;

    juce::String mchCopy = machineId;
    juce::Thread::launch([this, verifyEndpointUrl, keyToCheck, mchCopy] {
        juce::URL url(verifyEndpointUrl);
        juce::DynamicObject::Ptr bodyObj = new juce::DynamicObject();
        bodyObj->setProperty("license_key", keyToCheck);
        bodyObj->setProperty("machine_id", mchCopy);
        url = url.withPOSTData(juce::JSON::toString(juce::var(bodyObj.get()), true));

        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                           .withExtraHeaders("Content-Type: application/json\r\n")
                           .withConnectionTimeoutMs(2200);

        std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);
        if (stream != nullptr)
        {
            juce::String resp = stream->readEntireStreamAsString();
            juce::var parsed = juce::JSON::parse(resp);
            if (auto* obj = parsed.getDynamicObject())
            {
                bool revoked = static_cast<bool>(obj->getProperty("revoked"));
                if (revoked)
                {
                    VaultData data;
                    loadAndMergeAnchors(data);
                    data.activatedKey.clear();
                    saveToAllAnchors(data);
                    licensedFlag.store(false, std::memory_order_relaxed);
                    const bool stillTrial = trialActiveFlag.load(std::memory_order_relaxed);
                    runtimeCanaryToken.store(computeExpectedCanary(stillTrial), std::memory_order_relaxed);
                }
            }
        }
        heartbeatInProgress.store(false, std::memory_order_relaxed);
    });
}

LicenseManager::ActivationResult LicenseManager::activateLicense(const juce::String& licenseKeyInput,
                                                                 const juce::String& serverEndpointUrl)
{
    if (isDebuggerAttached())
    {
        return { false, juce::String::fromUTF8("Activación bloqueada: se detectó un depurador externo en ejecución.") };
    }

    juce::String cleanKey = licenseKeyInput.trim().toUpperCase();
    if (cleanKey.isEmpty())
    {
        return { false, juce::String::fromUTF8("Por favor ingresa un código de licencia.") };
    }

    juce::String finalBoundKey = cleanKey;

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

    juce::String errorMsg;
    if (!verifyCryptographicKeyForMachine(finalBoundKey, machineId, errorMsg))
    {
        return { false, errorMsg };
    }

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

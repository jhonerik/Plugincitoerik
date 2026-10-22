#!/usr/bin/env bash
# ==============================================================================
# ClipSampler10 - macOS Installer Packaging Script
# Genera instalador .pkg con selector exclusivo de VST3 y AAX
# ==============================================================================

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="${SCRIPT_DIR}/.."
BUILD_DIR="${PROJECT_ROOT}/build"
ARTEFACTS_DIR="${BUILD_DIR}/ClipSampler10_artefacts/Release"

OUTPUT_DIR="${PROJECT_ROOT}/dist_macos"
STAGING_DIR="${PROJECT_ROOT}/staging_pkg"

echo "=========================================================="
echo " Empaquetando ClipSampler10 para macOS (Universal Binary) "
echo "=========================================================="

mkdir -p "${OUTPUT_DIR}"
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}/vst3/Library/Audio/Plug-Ins/VST3"
mkdir -p "${STAGING_DIR}/aax/Library/Application Support/Avid/Audio/Plug-Ins"

# 1. Verificar existencia de artefactos compilados
VST3_BUNDLE="${ARTEFACTS_DIR}/VST3/ClipSampler10.vst3"
AAX_BUNDLE="${ARTEFACTS_DIR}/AAX/ClipSampler10.aaxplugin"

if [ ! -d "${VST3_BUNDLE}" ]; then
    echo "❌ Error: No se encontró el bundle VST3 en: ${VST3_BUNDLE}"
    echo "Por favor compila el proyecto primero: cmake --build build --config Release"
    exit 1
fi

echo "📦 Copiando bundle VST3 a staging..."
cp -R "${VST3_BUNDLE}" "${STAGING_DIR}/vst3/Library/Audio/Plug-Ins/VST3/"

if [ -d "${AAX_BUNDLE}" ]; then
    echo "📦 Copiando bundle AAX a staging..."
    cp -R "${AAX_BUNDLE}" "${STAGING_DIR}/aax/Library/Application Support/Avid/Audio/Plug-Ins/"
else
    echo "⚠️ Advertencia: No se encontró el bundle AAX en: ${AAX_BUNDLE}"
    echo "Creando paquete AAX de marcador de posición para desarrollo..."
    mkdir -p "${STAGING_DIR}/aax/Library/Application Support/Avid/Audio/Plug-Ins/ClipSampler10.aaxplugin"
fi

# 2. Generar sub-paquetes individuales con pkgbuild
echo "🔨 Construyendo ClipSampler10_VST3.pkg..."
pkgbuild --root "${STAGING_DIR}/vst3" \
         --identifier com.audioinnovations.clipsampler10.vst3 \
         --version 1.0.0 \
         --install-location "/" \
         "${SCRIPT_DIR}/ClipSampler10_VST3.pkg"

echo "🔨 Construyendo ClipSampler10_AAX.pkg..."
pkgbuild --root "${STAGING_DIR}/aax" \
         --identifier com.audioinnovations.clipsampler10.aax \
         --version 1.0.0 \
         --install-location "/" \
         "${SCRIPT_DIR}/ClipSampler10_AAX.pkg"

# 3. Construir instalador final con productbuild usando macos_distribution.xml
FINAL_PKG="${OUTPUT_DIR}/ClipSampler10_macOS_Installer_v1.0.0.pkg"

echo "🚀 Generando paquete instalador unificado con selector de componentes..."
productbuild --distribution "${SCRIPT_DIR}/macos_distribution.xml" \
             --resources "${SCRIPT_DIR}" \
             --package-path "${SCRIPT_DIR}" \
             "${FINAL_PKG}"

# 4. Limpieza de archivos temporales
rm -f "${SCRIPT_DIR}/ClipSampler10_VST3.pkg"
rm -f "${SCRIPT_DIR}/ClipSampler10_AAX.pkg"
rm -rf "${STAGING_DIR}"

echo "=========================================================="
echo "✅ Instalador generado exitosamente en:"
echo "   ${FINAL_PKG}"
echo "=========================================================="
echo "Nota: Para distribución pública, firma el paquete con:"
echo "productsign --sign \"Developer ID Installer: Tu Nombre (ID)\" \"${FINAL_PKG}\" \"${OUTPUT_DIR}/ClipSampler10_macOS_Installer_Signed.pkg\""
echo "xcrun notarytool submit \"${OUTPUT_DIR}/ClipSampler10_macOS_Installer_Signed.pkg\" --keychain-profile \"AC_NOTARY\" --wait"

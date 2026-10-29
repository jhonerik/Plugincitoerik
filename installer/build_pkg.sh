#!/usr/bin/env bash
# ==============================================================================
# ClipSampler - Script de Empaquetado de Instalador para macOS (Español)
# Genera instalador .pkg con selector de VST3 y Manual Oficial en PDF
# ==============================================================================

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="${SCRIPT_DIR}/.."
BUILD_DIR="${PROJECT_ROOT}/build"
ARTEFACTS_DIR="${BUILD_DIR}/ClipSampler_artefacts/Release"

OUTPUT_DIR="${PROJECT_ROOT}/dist_macos"
STAGING_DIR="${PROJECT_ROOT}/staging_pkg"

echo "=========================================================="
echo " Empaquetando ClipSampler para macOS (Español)            "
echo "=========================================================="

mkdir -p "${OUTPUT_DIR}"
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}/vst3/Library/Audio/Plug-Ins/VST3"
mkdir -p "${STAGING_DIR}/vst3/Library/Application Support/ClipSampler"
mkdir -p "${STAGING_DIR}/aax/Library/Application Support/Avid/Audio/Plug-Ins"

# 1. Verificar existencia de artefactos compilados
VST3_BUNDLE="${ARTEFACTS_DIR}/VST3/ClipSampler.vst3"
AAX_BUNDLE="${ARTEFACTS_DIR}/AAX/ClipSampler.aaxplugin"

if [ ! -d "${VST3_BUNDLE}" ]; then
    echo "❌ Error: No se encontró el paquete VST3 en: ${VST3_BUNDLE}"
    echo "Por favor compila el proyecto primero: cmake --build build --config Release"
    exit 1
fi

echo "📦 Copiando paquete VST3 y Manual Oficial en PDF a preparación..."
cp -R "${VST3_BUNDLE}" "${STAGING_DIR}/vst3/Library/Audio/Plug-Ins/VST3/"

if [ -f "${SCRIPT_DIR}/Manual_ClipSampler.pdf" ]; then
    cp "${SCRIPT_DIR}/Manual_ClipSampler.pdf" "${STAGING_DIR}/vst3/Library/Application Support/ClipSampler/Manual_ClipSampler.pdf"
elif [ -f "${PROJECT_ROOT}/Manual_ClipSampler.pdf" ]; then
    cp "${PROJECT_ROOT}/Manual_ClipSampler.pdf" "${STAGING_DIR}/vst3/Library/Application Support/ClipSampler/Manual_ClipSampler.pdf"
fi

if [ -f "${PROJECT_ROOT}/MANUAL_DE_USUARIO.md" ]; then
    cp "${PROJECT_ROOT}/MANUAL_DE_USUARIO.md" "${STAGING_DIR}/vst3/Library/Application Support/ClipSampler/MANUAL_DE_USUARIO.md"
fi

if [ -d "${AAX_BUNDLE}" ]; then
    echo "📦 Copiando paquete AAX a preparación..."
    cp -R "${AAX_BUNDLE}" "${STAGING_DIR}/aax/Library/Application Support/Avid/Audio/Plug-Ins/"
else
    echo "⚠️ Advertencia: No se encontró el paquete AAX en: ${AAX_BUNDLE}"
    echo "Creando paquete AAX de marcador de posición para desarrollo..."
    mkdir -p "${STAGING_DIR}/aax/Library/Application Support/Avid/Audio/Plug-Ins/ClipSampler.aaxplugin"
fi

# 2. Generar sub-paquetes individuales con pkgbuild
echo "🔨 Construyendo ClipSampler_VST3.pkg (con Manual_ClipSampler.pdf)..."
pkgbuild --root "${STAGING_DIR}/vst3" \
         --identifier com.audioinnovations.clipsampler.vst3 \
         --version 2.6.1 \
         --install-location "/" \
         "${SCRIPT_DIR}/ClipSampler_VST3.pkg"

echo "🔨 Construyendo ClipSampler_AAX.pkg..."
pkgbuild --root "${STAGING_DIR}/aax" \
         --identifier com.audioinnovations.clipsampler.aax \
         --version 2.6.1 \
         --install-location "/" \
         "${SCRIPT_DIR}/ClipSampler_AAX.pkg"

# 3. Construir instalador final con productbuild usando macos_distribution.xml
FINAL_PKG="${OUTPUT_DIR}/ClipSampler_macOS_Installer.pkg"

echo "🚀 Generando paquete instalador unificado en español..."
productbuild --distribution "${SCRIPT_DIR}/macos_distribution.xml" \
             --resources "${SCRIPT_DIR}" \
             --package-path "${SCRIPT_DIR}" \
             "${FINAL_PKG}"

# 4. Limpieza de archivos temporales
rm -f "${SCRIPT_DIR}/ClipSampler_VST3.pkg"
rm -f "${SCRIPT_DIR}/ClipSampler_AAX.pkg"
rm -rf "${STAGING_DIR}"

echo "=========================================================="
echo "✅ Instalador generado exitosamente en:"
echo "   ${FINAL_PKG}"
echo "=========================================================="

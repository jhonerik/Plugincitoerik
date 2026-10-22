# Carpeta de Recursos y Herramientas Externas (yt-dlp & ffmpeg)

Esta carpeta está destinada a albergar los binarios auxiliares de extracción y conversión:

### Windows:
- `yt-dlp.exe`
- `ffmpeg.exe`

### macOS:
- `yt-dlp` (con permisos de ejecución `chmod +x`)
- `ffmpeg` (con permisos de ejecución `chmod +x`)

El motor `AudioExtractorWorker` del plugin busca automáticamente en las siguientes rutas en orden de prioridad:
1. Directorio de datos de la aplicación:
   - Windows: `%LOCALAPPDATA%\ClipSampler10\tools\`
   - macOS: `/Library/Application Support/ClipSampler10/tools/` o `~/Library/Application Support/ClipSampler10/tools/`
2. Subdirectorio `tools/` contiguo al binario del plugin compilado.
3. El `PATH` global del sistema (p. ej. instalado mediante `winget install yt-dlp ffmpeg` en Windows o `brew install yt-dlp ffmpeg` en macOS).

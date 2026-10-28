; ==============================================================================
; ClipSampler v2.6 - Script de Instalador de Windows de 64 bits (Inno Setup)
; Formato Exclusivo: VST3 (64-bit)
; ==============================================================================

#define MyAppName "ClipSampler"
#define MyAppVersion "2.6.0"
#define MyAppPublisher "AudioInnovations"
#define MyAppURL "https://www.audioinnovations.com"

[Setup]
AppId={{C78912A3-B245-429C-8A91-58FF807B4391}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf64}\{#MyAppName}
DisableDirPage=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist_windows
OutputBaseFilename=ClipSampler_Windows_Installer_v{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
LicenseFile=license.txt

[Types]
Name: "full"; Description: "Instalación Plugin VST3 (64-bit)"

; ==============================================================================
; FORMATO EXCLUSIVO: VST3 (64-bit)
; ==============================================================================
[Components]
Name: "vst3"; Description: "Plugin VST3 (64-bit) [FL Studio, Ableton Live, Cubase, Reaper, Studio One]"; Types: full; Flags: fixed

[InstallDelete]
; Limpia automáticamente cualquier versión anterior con el nombre antiguo ClipSampler10.vst3 o AAX residual
Type: filesandordirs; Name: "{commoncf64}\VST3\ClipSampler10.vst3"
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\ClipSampler.aaxplugin"

[Files]
; --- 1. VST3 Plugin ---
; Destino oficial de 64 bits: C:\Program Files\Common Files\VST3\ClipSampler.vst3
Source: "..\build\ClipSampler_artefacts\Release\VST3\ClipSampler.vst3\*"; \
    DestDir: "{commoncf64}\VST3\ClipSampler.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; \
    Components: vst3

; --- 2. Herramientas auxiliares opcionales de Windows (yt-dlp.exe / ffmpeg.exe si están presentes) ---
Source: "..\resources\tools\*.exe"; \
    DestDir: "{commonappdata}\ClipSampler\tools"; \
    Flags: ignoreversion skipifsourcedoesntexist

; --- 3. Avisos y Licencias de Componentes de Terceros (LGPL / Dominio Público) ---
Source: "LICENCIAS_TERCEROS.txt"; \
    DestDir: "{commonappdata}\ClipSampler\tools"; \
    Flags: ignoreversion
Source: "LICENCIAS_TERCEROS.txt"; \
    DestDir: "{commonpf64}\{#MyAppName}"; \
    Flags: ignoreversion

[Messages]
SelectComponentsLabel2=Componentes a instalar en tu sistema:

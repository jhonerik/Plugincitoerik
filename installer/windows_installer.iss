; ==============================================================================
; ClipSampler v2.5 - Script de Instalador de Windows de 64 bits (Inno Setup)
; Formatos Exclusivos: VST3 y AAX (Pro Tools)
; ==============================================================================

#define MyAppName "ClipSampler"
#define MyAppVersion "2.5.0"
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
Name: "full"; Description: "Instalación Completa (VST3 y AAX)"
Name: "custom"; Description: "Instalación Personalizada"; Flags: iscustom

; ==============================================================================
; MENÚ DE SELECCIÓN CON SOLAMENTE VST3 Y AAX
; ==============================================================================
[Components]
Name: "vst3"; Description: "VST3 Plugin (64-bit)"; Types: full custom
Name: "aax"; Description: "AAX Plugin (64-bit Pro Tools)"; Types: full custom

[InstallDelete]
; Limpia automáticamente cualquier versión anterior con el nombre antiguo ClipSampler10.vst3
Type: filesandordirs; Name: "{commoncf64}\VST3\ClipSampler10.vst3"

[Files]
; --- 1. VST3 Plugin ---
; Destino oficial de 64 bits: C:\Program Files\Common Files\VST3\ClipSampler.vst3
Source: "..\build\ClipSampler_artefacts\Release\VST3\ClipSampler.vst3\*"; \
    DestDir: "{commoncf64}\VST3\ClipSampler.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; \
    Components: vst3

; --- 2. AAX Plugin (Pro Tools) ---
; Destino oficial de 64 bits: C:\Program Files\Common Files\Avid\Audio\Plug-Ins\ClipSampler.aaxplugin
Source: "..\build\ClipSampler_artefacts\Release\AAX\ClipSampler.aaxplugin\*"; \
    DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\ClipSampler.aaxplugin"; \
    Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist; \
    Components: aax

; --- 3. Herramientas auxiliares opcionales de Windows (yt-dlp.exe / ffmpeg.exe si están presentes) ---
Source: "..\resources\tools\*.exe"; \
    DestDir: "{commonappdata}\ClipSampler\tools"; \
    Flags: ignoreversion skipifsourcedoesntexist

[Messages]
SelectComponentsLabel2=Selecciona los formatos que deseas instalar en tu sistema:

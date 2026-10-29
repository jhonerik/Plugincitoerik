; ==============================================================================
; ClipSampler v2.6.1 - Script de Instalador de Windows de 64 bits (Inno Setup)
; Idioma: 100% Español | Incluye Plugin VST3 (64 bits) y Manual Oficial en PDF
; ==============================================================================

#define MyAppName "ClipSampler"
#define MyAppVersion "2.6.1"
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
ShowLanguageDialog=no

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Types]
Name: "full"; Description: "Instalación Completa en Español (Plugin VST3 de 64 bits + Manual Oficial en PDF)"

; ==============================================================================
; COMPONENTES A INSTALAR (100% EN ESPAÑOL)
; ==============================================================================
[Components]
Name: "vst3"; Description: "Plugin de Audio VST3 (64 bits) [FL Studio, Ableton Live, Cubase, Reaper, Studio One]"; Types: full; Flags: fixed
Name: "manual"; Description: "Manual Oficial de Usuario en PDF (Manual_ClipSampler.pdf con Interfaz Gráfica Actualizada)"; Types: full; Flags: fixed

[InstallDelete]
; Limpia automáticamente cualquier versión anterior con el nombre antiguo ClipSampler10.vst3 o AAX residual
Type: filesandordirs; Name: "{commoncf64}\VST3\ClipSampler10.vst3"
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\ClipSampler.aaxplugin"

[Files]
; --- 1. Plugin VST3 (64 bits) ---
; Destino oficial de 64 bits: C:\Program Files\Common Files\VST3\ClipSampler.vst3
Source: "..\build\ClipSampler_artefacts\Release\VST3\ClipSampler.vst3\*"; \
    DestDir: "{commoncf64}\VST3\ClipSampler.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; \
    Components: vst3

; --- 2. Manual Oficial de Usuario en PDF (accesible desde el botón esquinero "Manual" del plugin) ---
Source: "Manual_ClipSampler.pdf"; \
    DestDir: "{commonappdata}\ClipSampler"; \
    Flags: ignoreversion; \
    Components: manual
Source: "Manual_ClipSampler.pdf"; \
    DestDir: "{commonpf64}\{#MyAppName}"; \
    Flags: ignoreversion; \
    Components: manual
Source: "..\MANUAL_DE_USUARIO.md"; \
    DestDir: "{commonappdata}\ClipSampler"; \
    Flags: ignoreversion; \
    Components: manual
Source: "..\MANUAL_DE_USUARIO.md"; \
    DestDir: "{commonpf64}\{#MyAppName}"; \
    Flags: ignoreversion; \
    Components: manual

; --- 3. Herramientas auxiliares opcionales de Windows (yt-dlp.exe / ffmpeg.exe si están presentes) ---
Source: "..\resources\tools\*.exe"; \
    DestDir: "{commonappdata}\ClipSampler\tools"; \
    Flags: ignoreversion skipifsourcedoesntexist

; --- 4. Avisos y Licencias de Componentes de Terceros ---
Source: "LICENCIAS_TERCEROS.txt"; \
    DestDir: "{commonappdata}\ClipSampler\tools"; \
    Flags: ignoreversion
Source: "LICENCIAS_TERCEROS.txt"; \
    DestDir: "{commonpf64}\{#MyAppName}"; \
    Flags: ignoreversion

[Run]
Filename: "{commonappdata}\ClipSampler\Manual_ClipSampler.pdf"; \
    Description: "Abrir el Manual Oficial de Usuario en PDF (Manual_ClipSampler.pdf)"; \
    Flags: postinstall shellexec skipifsilent unchecked

[Messages]
spanish.WelcomeLabel1=Bienvenido al Asistente de Instalación de [name]
spanish.WelcomeLabel2=Este asistente instalará [name/ver] (Plugin VST3 de 64 bits y el Manual Oficial de Usuario en PDF) en tu computadora.%n%nSe recomienda cerrar tu estación de trabajo de audio digital (DAW) antes de continuar.
spanish.SelectComponentsDesc=¿Qué componentes deseas instalar?
spanish.SelectComponentsLabel2=Componentes incluidos en esta instalación en español:
spanish.ClickNext=Haz clic en Siguiente para continuar, o en Cancelar para salir de la instalación.
spanish.FinishedHeadingLabel=Completando la instalación de [name]
spanish.FinishedLabelNoIcons=El programa completó la instalación de [name] y su Manual Oficial en PDF en tu sistema.%n%nPuedes abrir el manual en cualquier momento desde el botón esquinero «Manual» dentro de la interfaz gráfica del plugin.

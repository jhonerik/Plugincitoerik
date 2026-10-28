# ==============================================================================
# ClipSampler v2.6 - Script Automatizado de Compilación y Empaquetado (Windows)
# Compila el plugin VST3 en modo Release y genera el instalador oficial .exe
# ==============================================================================
$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   ClipSampler v2.6 - Sistema de Compilación (Windows)    " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$ProjectRoot = $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build"
$DistDir = Join-Path $ProjectRoot "dist_windows"

# Limpiar carpetas antiguas que tenían el sufijo "10" (ClipSampler10*) dentro de build/
if (Test-Path $BuildDir) {
    Get-ChildItem -Path $BuildDir -Filter "ClipSampler10*" -Force -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

# 1. Comprobación de herramientas necesarias en el sistema
Write-Host "`n[1/5] Verificando herramientas del sistema..." -ForegroundColor Yellow

# Buscar CMake en PATH o en ubicaciones estándar de Visual Studio / WinGet
$CmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $CmakeCmd) {
    $PotentialCmake = @(
        (Join-Path $env:ProgramFiles "CMake\bin\cmake.exe"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
        (Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Links\cmake.exe")
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1

    if ($PotentialCmake) {
        $env:Path += ";" + (Split-Path $PotentialCmake)
        $CmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
    }
}

if ($CmakeCmd) {
    Write-Host "  [OK] CMake detectado." -ForegroundColor Green
} else {
    Write-Host "  [ERROR] CMake no encontrado en PATH." -ForegroundColor Red
    exit 1
}

# Buscar Inno Setup (ISCC) para empaquetar el instalador .exe
$IsccCmd = Get-Command iscc -ErrorAction SilentlyContinue
if (-not $IsccCmd) {
    $PotentialIscc = @(
        (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 6\ISCC.exe"),
        (Join-Path $env:ProgramFiles "Inno Setup 6\ISCC.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Inno Setup 6\ISCC.exe")
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1

    if ($PotentialIscc) {
        $env:Path += ";" + (Split-Path $PotentialIscc)
        $IsccCmd = Get-Command iscc -ErrorAction SilentlyContinue
    }
}

if ($IsccCmd) {
    Write-Host "  [OK] Inno Setup (ISCC) detectado." -ForegroundColor Green
} else {
    Write-Host "  [AVISO] Inno Setup no encontrado en PATH." -ForegroundColor Yellow
}

# Verificar herramientas de extracción multimedia (yt-dlp y ffmpeg)
$YtDlp = Get-Command yt-dlp -ErrorAction SilentlyContinue
if ($YtDlp) {
    Write-Host "  [OK] yt-dlp detectado." -ForegroundColor Green
} else {
    Write-Host "  [AVISO] yt-dlp no encontrado en PATH." -ForegroundColor Yellow
}

$FFmpeg = Get-Command ffmpeg -ErrorAction SilentlyContinue
if ($FFmpeg) {
    Write-Host "  [OK] ffmpeg detectado." -ForegroundColor Green
} else {
    Write-Host "  [AVISO] ffmpeg no encontrado en PATH." -ForegroundColor Yellow
}

# 2. Verificación del compilador C++ de Visual Studio 2022 (MSVC)
Write-Host "`n[2/5] Verificando compilador de C++ (Visual Studio 2022)..." -ForegroundColor Yellow

$VsInstaller = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\setup.exe"
$VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

$HasMsvc = $false
$VsInstallPath = $null
if (Test-Path $VsWhere) {
    $VcPath = & $VsWhere -latest -products * -find "VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe"
    if ($VcPath) {
        $HasMsvc = $true
        Write-Host "  [OK] Compilador MSVC C++ encontrado." -ForegroundColor Green
    } else {
        $VsInstallPath = & $VsWhere -latest -products * -property installationPath
    }
}

if (-not $HasMsvc) {
    Write-Host "  [AVISO] El compilador de C++ de Visual Studio aún no está instalado." -ForegroundColor Yellow
    Write-Host "  Instalando la carga de trabajo de C++ (Visual Studio Workload VCTools)..." -ForegroundColor Cyan

    if (-not $VsInstallPath) {
        $VsInstallPath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\2022\BuildTools"
    }

    if (Test-Path $VsInstaller) {
        $Process = Start-Process -FilePath $VsInstaller -ArgumentList "modify", "--installPath", "`"$VsInstallPath`"", "--add", "Microsoft.VisualStudio.Workload.VCTools", "--includeRecommended", "--passive", "--norestart" -Wait -PassThru
        if ($Process.ExitCode -eq 0 -or $Process.ExitCode -eq 3010) {
            Write-Host "  [OK] Compilador C++ instalado correctamente!" -ForegroundColor Green
        } else {
            Write-Host "  [ERROR] Falló la instalación automática de Visual Studio C++ (Código: $($Process.ExitCode))." -ForegroundColor Red
            exit 1
        }
    } else {
        Write-Host "  [ERROR] No se encontró el instalador de Visual Studio." -ForegroundColor Red
        exit 1
    }
}

# 3. Configuración del proyecto con CMake (Generador Visual Studio 17 2022 x64)
Write-Host "`n[3/5] Configurando proyecto ClipSampler con CMake (MSVC x64)..." -ForegroundColor Yellow
$CmakeArgs = @(
    "-B", $BuildDir,
    "-G", "Visual Studio 17 2022",
    "-A", "x64"
)

if ($env:AAX_SDK_PATH -and (Test-Path $env:AAX_SDK_PATH)) {
    Write-Host "  [OK] AAX SDK detectado: $env:AAX_SDK_PATH" -ForegroundColor Green
    $CmakeArgs += "-DAAX_SDK_PATH=$env:AAX_SDK_PATH"
    $CmakeArgs += "-DFORCE_AAX_BUILD=ON"
}

& cmake @CmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "  [ERROR] Error durante la configuración de CMake." -ForegroundColor Red
    exit $LASTEXITCODE
}

# 4. Compilación del objetivo ClipSampler en modo Release
Write-Host "`n[4/5] Compilando plugin ClipSampler en modo Release..." -ForegroundColor Yellow
& cmake --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "  [ERROR] Error durante la compilación de C++." -ForegroundColor Red
    exit $LASTEXITCODE
}

$Vst3Path = Join-Path $BuildDir "ClipSampler_artefacts\Release\VST3\ClipSampler.vst3"
if (Test-Path $Vst3Path) {
    Write-Host "  [OK] Bundle VST3 generado en: $Vst3Path" -ForegroundColor Green
}

# 5. Generación del instalador ejecutable (.exe) con Inno Setup
Write-Host "`n[5/5] Generando instalador ejecutable (.exe) con Inno Setup..." -ForegroundColor Yellow
if (Test-Path $DistDir) {
    Remove-Item -Path (Join-Path $DistDir "*.exe") -Force -ErrorAction SilentlyContinue
} else {
    New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
}

$IssScript = Join-Path $ProjectRoot "installer\windows_installer.iss"
if ($IsccCmd -or (Get-Command iscc -ErrorAction SilentlyContinue)) {
    & iscc "/O$DistDir" $IssScript
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  [OK] Instalador compilado exitosamente!" -ForegroundColor Green
    } else {
        Write-Host "  [ERROR] Inno Setup falló al generar el instalador." -ForegroundColor Red
        exit $LASTEXITCODE
    }
} else {
    Write-Host "  [AVISO] Inno Setup no disponible para crear el .exe." -ForegroundColor Yellow
}

# Actualizar marcas de tiempo para evitar el caché NTFS Tunneling de Windows Explorer
$Now = Get-Date
try {
    if (Test-Path $DistDir) {
        (Get-Item $DistDir).CreationTime = $Now
        (Get-Item $DistDir).LastWriteTime = $Now
    }
} catch {}

$FinalExes = Get-ChildItem $DistDir -Filter "*.exe" -ErrorAction SilentlyContinue
if ($FinalExes) {
    foreach ($exe in $FinalExes) {
        $exe.CreationTime = $Now
        $exe.LastWriteTime = $Now
        $exe.LastAccessTime = $Now
        Write-Host "`n==========================================================" -ForegroundColor Cyan
        Write-Host "   Proceso de compilación completado con éxito!           " -ForegroundColor Green
        Write-Host "==========================================================" -ForegroundColor Cyan
        Write-Host "  EJECUTABLE LISTO: $($exe.FullName)" -ForegroundColor Green
        Write-Host "  Fecha Creado/Modificado: $($exe.LastWriteTime)" -ForegroundColor Green
    }
}

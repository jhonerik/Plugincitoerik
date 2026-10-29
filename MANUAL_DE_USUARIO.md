# 🎹 Manual Oficial de Usuario — ClipSampler v2.6.1
### Sampler de Transformación Local, Resintetizador Analógico, Rebanador MPC (C1–G2) y Explorador Freesound API v2

Bienvenido al **Manual Oficial de Usuario de ClipSampler v2.6.1**, el instrumento virtual y procesador DSP de estudio (**VST3 / AAX / AU / Standalone**) concebido bajo el modelo de **Sampler de Transformación Local**, suite de **Coloración Analógica y Derechos IA**, e integración directa con la biblioteca mundial de sonido **Freesound.org (+500,000 sonidos)**.

ClipSampler está diseñado para productores musicales, beatmakers y diseñadores de sonido que desean importar, trocear automáticamente (*MPC Auto-Chop C1–G2*), transformar analógicamente para generar nuevas muestras libres de derechos de autor, tocar en vivo desde su **teclado o controlador MIDI** y convertir audio en notas MIDI (**Audio a MIDI**) sin salir de su DAW.

---

## 📸 Vistas Generales de la Interfaz Gráfica Actualizada (v2.6.1)

### 1. Interfaz Principal — Sample de Freesound Cargado y Rebanador MPC (C1–G2)
![ClipSampler v2.6.1 - Sample de Freesound en Visor 1 y Rebanador MPC](pagina%20web/clipsampler-freesound-loaded.png)
*Vista 1: Muestra descargada directamente desde Freesound API v2 cargada en el **Visor 1**, segmentada automáticamente en rodajas rítmicas **C1 a D2** listas para tocar en tu teclado MIDI o controlador de pads. En la esquina superior derecha se incluye el nuevo botón **`📘 Manual`** junto a **`Limpiar Caché`**.*

### 2. Transformación Analógica y Derechos IA en el Visor 2 (Nuevo Sample Libre de Derechos)
![ClipSampler v2.6.1 - Transformación Analógica y Derechos IA en Visor 2](pagina%20web/clipsampler-analog-warmth-ui.png)
*Vista 2: Aplicación de **`📼 Calor Analógico (Tape Warmth)`** y transformación espectral **`+3 st IA`** en el **Visor 2**, generando un nuevo sample con huella acústica transformada y libre de derechos de autor, listo para tocar cromáticamente o por rodajas MPC en tu teclado MIDI, arrastrar en `.WAV` o transcribir con **`🎹 Convertir a MIDI`**.*

### 3. Explorador Integrado de Samples Freesound (API v2) con Salto Directo de Página
![ClipSampler v2.6.1 - Explorador Freesound API v2 con Salto Directo de Página](pagina%20web/freesound-modal-ui.png)
*Vista 3: Navegador modal conectado a **Freesound API v2** con más de 500,000 sonidos, etiquetas de BPM, formato y licencia (`CC0 LIBRE` / `CC-BY`), botones **`🔗 Similares`**, **`▶ Escuchar`** y **`⚡ Cargar en Pad`**, e incluyendo en la barra inferior la nueva casilla **`Ir a pág:`** para escribir cualquier número de página (ej. `1000` o `1653`) y saltar al instante presionando **`Enter`**.*

---

## ⚖️ Marco Legal, Modelo Comercial y Responsabilidad del Usuario

> [!IMPORTANT]
> **DESCARGO DE RESPONSABILIDAD LEGAL Y USO ÉTICO DEL SOFTWARE:**
> 1. **Instrumento DSP de Procesamiento Local:** ClipSampler es comercializado exclusivamente como una herramienta de ingeniería de sonido, rebanado rítmico y síntesis para audio provisto por el usuario.
> 2. **Responsabilidad 100% del Usuario:** La procedencia, legalidad y derechos de autor de cualquier muestra, archivo o audio cargado o capturado en el software recaen **única y exclusivamente en el usuario final**.
> 3. **Función Opcional de Uso Libre (Web):** El acceso a la lectura de URLs de internet (YouTube, TikTok, Instagram) permanece **bloqueado por defecto** y requiere la pulsación del botón **`⚖️ Uso Libre (Web)`** donde el usuario acepta explícitamente los términos de uso antes de su habilitación, eximiendo a los desarrolladores de cualquier responsabilidad civil, mercantil o penal derivada del uso ilícito de contenidos protegidos.
> 4. **Aviso de Componentes de Terceros (FFmpeg, yt-dlp y Freesound API):** Este software incluye binarios sin modificar de **FFmpeg** (licencia LGPL 2.1+) disponibles en [ffmpeg.org](https://ffmpeg.org) y **yt-dlp** disponibles en [github.com/yt-dlp/yt-dlp](https://github.com/yt-dlp/yt-dlp), además de conectividad opcional con **Freesound API v2** mediante clave personal del usuario.

---

## 📑 Índice del Manual

1. [Instalación 100% en Español en tu DAW (Windows y macOS)](#1-instalación-100-en-español-en-tu-daw-windows-y-macos)
2. [Período de Prueba (3 Días) y Activación de Licencia Vitalicia](#2-período-de-prueba-3-días-y-activación-de-licencia-vitalicia)
3. [Barra de Encabezado Superior y Botón Esquinero 'Manual'](#3-barra-de-encabezado-superior-y-botón-esquinero-manual)
4. [Atajos de Teclado Globales y Controles Rápidos](#4-atajos-de-teclado-globales-y-controles-rápidos)
5. [Paso 1: Modelo Sampler Local (Arrastrar WAV/MP3 o Cargar Archivo)](#5-paso-1-modelo-sampler-local-arrastrar-wavmp3-o-cargar-archivo)
6. [Paso 2: Explorador Integrado de Freesound (API v2) y Salto Rápido de Página](#6-paso-2-explorador-integrado-de-freesound-api-v2-y-salto-rápido-de-página)
7. [Paso 3: Función Opcional 'Uso Libre' (Extracción Web por Segundos o Compases)](#7-paso-3-función-opcional-uso-libre-extracción-web-por-segundos-o-compases)
8. [Paso 4: Transformación Analógica y Suite de Derechos IA (Visor 2)](#8-paso-4-transformación-analógica-y-suite-de-derechos-ia-visor-2)
9. [Paso 5: Rebanador MPC Auto-Chop (C1–G2), Teclado MIDI y Efectos Macro](#9-paso-5-rebanador-mpc-auto-chop-c1g2-teclado-midi-y-efectos-macro)
10. [Paso 6: Reverse, Half-Time, Corte Exacto y Conversor de Audio a MIDI](#10-paso-6-reverse-half-time-corte-exacto-y-conversor-de-audio-a-midi)
11. [Solución de Problemas Frecuentes](#11-solución-de-problemas-frecuentes)

---

## 1. Instalación 100% en Español en tu DAW (Windows y macOS)

### 🪟 En Windows 10 / 11 (64-bit)
1. Ejecuta el instalador **`ClipSampler_Windows_Installer_v2.6.1.exe`** como Administrador (interfaz del asistente 100% en español).
2. Acepta el acuerdo de licencia en español e instala el plugin:
   - **VST3 (64-bit):** Compatible con FL Studio, Ableton Live, Cubase, Studio One, Reaper, Bitwig, etc. (`C:\Program Files\Common Files\VST3\ClipSampler.vst3`).
3. El instalador copiará automáticamente las herramientas auxiliares en `C:\ProgramData\ClipSampler\tools` y el **Manual Oficial en PDF (`Manual_ClipSampler.pdf`)** en `C:\ProgramData\ClipSampler\Manual_ClipSampler.pdf` y `C:\Program Files\ClipSampler\Manual_ClipSampler.pdf`.
4. Abre tu DAW, escanea tus plugins y carga **ClipSampler** en una pista de **Instrumento MIDI**.

### 🍎 En macOS (Universal Binary: Apple Silicon M1/M2/M3/M4 + Intel)
1. Ejecuta el instalador en español **`ClipSampler_macOS_Installer_v2.6.1.pkg`**:
   - **`ClipSampler.vst3`** → `/Library/Audio/Plug-Ins/VST3/`
   - **`ClipSampler.aaxplugin`** → `/Library/Application Support/Avid/Audio/Plug-Ins/`
   - **`Manual_ClipSampler.pdf`** → `/Library/Application Support/ClipSampler/Manual_ClipSampler.pdf`

---

## 2. Período de Prueba (3 Días) y Activación de Licencia Vitalicia

- Al abrir **ClipSampler v2.6.1** por primera vez dispones de **3 días (72 horas) de prueba gratuita** con todas las funciones activas.
- Haz clic en el botón de estado de licencia en el encabezado superior para copiar tu **ID de Máquina (`MCH-XXXX-XXXX-XXXX`)** e ingresar tu clave **`CS10-XXXX-XXXX-XXXX-XXXX`**.
- Una vez activada, el indicador mostrará **`✓ LICENCIA ACTIVA [PRO]`** en verde esmeralda de forma permanente.

---

## 3. Barra de Encabezado Superior y Botón Esquinero 'Manual'

En la esquina superior derecha del plugin encontrarás los **5 botones de control global**:
1. **`✓ LICENCIA ACTIVA [PRO]`:** Muestra el estado de activación de tu licencia vitalicia.
2. **`🎵 Freesound`:** Abre el explorador integrado conectado a **Freesound API v2** (+500,000 sonidos).
3. **`Actualizar Motor`:** Ejecuta `yt-dlp -U` en segundo plano para mantener al día el motor de extracción web.
4. **`Limpiar Caché`:** Libera espacio eliminando preescuchas antiguas sin borrar las muestras activas de tus visores.
5. **`📘 Manual` (Nuevo Botón Esquinero):** Ubicado justo al lado derecho de **`Limpiar Caché`**. Al oprimirlo, abre instantáneamente este **Manual Oficial de Usuario en PDF (`Manual_ClipSampler.pdf`)** con toda la guía visual de la interfaz gráfica.

---

## 4. Atajos de Teclado Globales y Controles Rápidos

| Atajo (Windows) | Atajo (macOS) | Acción Inmediata |
| :--- | :--- | :--- |
| **`Espacio`** | **`Espacio`** | **Reproducir / Detener** vista previa de la muestra activa. |
| **`Ctrl + F`** | **`Cmd + F`** | Abrir el explorador integrado **Freesound API v2**. |
| **`Ctrl + O`** | **`Cmd + O`** | Abrir ventana para **Cargar archivo de audio local**. |
| **`Ctrl + M`** | **`Cmd + M`** | **Convertir audio a notas MIDI** para Piano Roll. |
| **`Ctrl + E`** | **`Cmd + E`** | **Exportar muestra activa** como archivo `.WAV` (24-bit PCM). |
| **`Teclas 1 a 8`** | **`Teclas 1 a 8`** | **Disparar en vivo las rodajas MPC (`C1` a `G1`)** desde el teclado de tu PC. |
| **`Escape`** | **`Escape`** | **Cerrar ventana modal** activa (Freesound / Licencia) o detener reproducción. |
| **`Doble Clic en Perilla`** | **`Doble Clic en Perilla`** | **Restablecer la perilla** a su valor neutro de fábrica. |

---

## 5. Paso 1: Modelo Sampler Local (Arrastrar WAV/MP3 o Cargar Archivo)

1. **Arrastrar y Soltar (Drag & Drop):** Arrastra cualquier archivo **WAV, MP3, AIFF, FLAC u OGG** desde tu explorador de archivos o navegador de tu DAW directamente sobre el **Visor 1**.
2. **Botón `📂 Cargar Audio Local` (`Ctrl/Cmd + O`):** Selecciona muestras desde tu disco duro.
3. **Detección Automática Musical:** ClipSampler detecta al instante el **BPM (ej. `77.5 BPM`)**, la **Tonalidad Musical (ej. `Tono: Am`)** y la duración, creando las divisiones rítmicas del **Rebanador MPC (`C1` a `G2`)** para tocar inmediatamente con tu teclado MIDI.

---

## 6. Paso 2: Explorador Integrado de Freesound (API v2) y Salto Rápido de Página

Gracias a la conexión directa con **Freesound API v2**, ClipSampler pone a tu alcance más de **500,000 sonidos, one-shots, loops, foley, percusiones y atmósferas** dentro de tu DAW:

1. **Búsqueda y Filtros Inteligentes:**
   - Escribe cualquier término (*kick, 808, snare, vinyl loop, vocal, anime, piano, riser*) y filtra por **Duración**, **Ordenación**, **Licencia (`Creative Commons 0 - CC0 Libre` o `CC-BY`)** y **Formato (`WAV`, `FLAC`, `AIFF`)**.
2. **Acciones por Muestra:**
   - **`🔗 Similares`:** Busca sonidos acústicamente similares mediante el motor de análisis espectral de Freesound.
   - **`▶ Escuchar` / `⏹ Detener`:** Preescucha inmediata con selector **`🔊 Salida: Auto / Solo DAW / Solo Directo`**.
   - **`⚡ Cargar en Pad`:** Descarga el sample en alta calidad, lo envía al **Visor 1 (`[FREESOUND]`)**, detecta su BPM y tonalidad, y lo asigna a los pads MIDI **C1–G2**.
3. **NUEVO — Casilla de Salto Directo de Página (`Ir a pág:` + `Enter`):**
   - En la barra inferior de paginación (junto a `◀ Anterior`, `Página X de Y` y `Siguiente ▶`), encontrarás la casilla **`Ir a pág:`**.
   - Si una búsqueda tiene miles de páginas (por ejemplo, `16211` páginas) y deseas ir directamente a la **página `1000`** o a la **página `1653`**, simplemente **escribe el número en la casilla en blanco y presiona la tecla `Enter`** (o haz clic en **`Ir ↵`**). El buscador saltará instantáneamente a esa página sin necesidad de pulsar "Siguiente" repetidamente.

---

## 7. Paso 3: Función Opcional 'Uso Libre' (Extracción Web por Segundos o Compases)

1. Haz clic en **`⚖️ Uso Libre (Web)`** y acepta el acuerdo de responsabilidad para habilitar los selectores de **YouTube, TikTok e Instagram**.
2. Pega el enlace con **`Pegar`**, verifica su longitud con **`Detectar Duración`** y elige el punto de inicio con el deslizador **`Inicio (MM:SS)`**.
3. Conmuta entre **`⏱️ Segundos`** (`Extraer 3 Segundos`, `Extraer 10 Segundos`, `Extraer 30 Segundos`) o **`🎵 Compases`** (`1 Bar`, `2 Bars`, `4 Bars` sincronizados al BPM de tu DAW).
4. Usa **`🔄 Nueva Extracción`** para limpiar ambos visores y comenzar de cero cuando lo desees.

---

## 8. Paso 4: Transformación Analógica y Suite de Derechos IA (Visor 2)

Como se aprecia en la **Vista 2**, una vez cargado un sample en el **Visor 1** (ya sea desde Freesound, archivo local o extracción web), puedes transformarlo en un **nuevo sample libre de derechos de autor** que se aloja en el **Visor 2**:

- **`📼 Calor Analógico (Tape Warmth)`:** Aplica saturación armónica de cinta analógica vintage, realce cálido de graves estilo Pultec (~160 Hz) y modelado espectral, generando en el **Visor 2 (`VISOR 2: CALOR ANALÓGICO (TAPE WARMTH)`)** una nueva onda estéreo con carácter analógico único lista para usar.
- **`▲ Alterar +3 Tonos Más` y `▼ Alterar -3 Tonos Menos`:** Transforma la altura tonal mediante interpolación de Lagrange de alta precisión, preservación de formantes y dispersión de fase all-pass para modificar la huella digital acústica (evitando coincidencias de Content ID) mientras mantienes toda la musicalidad.
- **`🛡️ Verificar Derechos`:** Compara matemáticamente la divergencia espectral entre el Audio 1 y el Audio 2 y confirma en pantalla cuando tu nueva muestra está **`✅ LIBRE DE DERECHOS / LISTO PARA USAR`**.

---

## 9. Paso 5: Rebanador MPC Auto-Chop (C1–G2), Teclado MIDI y Efectos Macro

Tanto el **Audio 1 (Original)** como el **Audio 2 (Transformado Analógicamente / Derechos IA)** pueden tocarse en vivo y procesarse con el rack DSP:

- **Selector `Efectos: [Audio 1] [Audio 2]`:** Elige qué visor deseas escuchar, procesar y tocar en tu controlador MIDI.
- **Rebanador MPC (`Rebanador MPC: SÍ / NO`):**
  - Cuando está en **`SÍ`**, el sample se divide automáticamente en rodajas rítmicas mapeadas desde **`C1` hasta `G2`** en tu teclado MIDI (y en las teclas **`1` a `8`** de tu computadora). Puedes arrastrar las líneas amarillas punteadas con el ratón para ajustar cada corte a tu gusto.
  - Cuando está en **`NO`**, el sample completo se asigna en **Modo Cromático Polifónico** a lo largo de todo tu teclado MIDI para tocar melodías y acordes.
- **5 Perillas Hardware DSP (Doble clic = Reset):**
  - **`Lo-Fi (0% - 100%)`:** Reducción de bits a 12-Bit estilo samplers clásicos SP-1200 / MPC60.
  - **`Tono (-12 st a +12 st)`:** Transposición cromática fina en semitonos.
  - **`Filtro (15% - 100%)`:** Filtro paso-bajo resonante cálido.
  - **`Atk (0.5 ms - 500 ms)` y `Rel (5 ms - 2000 ms)`:** Envolvente ADSR anti-chasquidos por voz.
- **`Sinc. BPM: SÍ/NO` y `Aplicar FX`:** Sincroniza el sample al tempo del DAW e imprime los efectos directamente en la onda.

---

## 10. Paso 6: Reverse, Half-Time, Corte Exacto y Conversor de Audio a MIDI

En la barra de transporte inferior dispones de herramientas de producción y exportación instantánea:
- **`▶ Reproducir` y `■ Detener`:** Audición inmediata con indicador de **`BPM | Tono | DAW`**.
- **`◀ Rev` (Reverse) y `⏳ 1/2` (Half-Time):**
  - Con **1 clic** en la onda (`[TODO EL AUDIO]`), invierte o aplica media velocidad a la muestra completa.
  - Con **doble clic** sobre una rodaja específica (ej. `C1`, `F#1`), invierte o aplica media velocidad **únicamente a ese corte**.
- **`✂ Corte Exacto`:** Permite escuchar y arrastrar hacia tu DAW **solamente la rodaja seleccionada**.
- **`Arrastrar .WAV`:** Mantén presionado y arrastra el audio procesado directamente a una pista de audio de tu DAW.
- **`🎹 Convertir a MIDI` (`Ctrl/Cmd + M`):** Analiza las frecuencias fundamentales del sample (Audio 1 o Audio 2) mediante bancos de resonadores Goertzel y genera un archivo `.mid` que puedes arrastrar directamente al Piano Roll de cualquier sintetizador virtual.
- **`Exportar .WAV...` (`Ctrl/Cmd + E`):** Guarda el archivo WAV de 24 bits en cualquier carpeta de tu computadora.

---

## 11. Solución de Problemas Frecuentes

- **¿Cómo abro este manual desde dentro del plugin?** Haz clic en el botón **`📘 Manual`** situado en la esquina superior derecha, justo al lado de **`Limpiar Caché`**.
- **¿Cómo salto rápido a una página avanzada en Freesound (ej. página 1653)?** Escribe `1653` en la casilla **`Ir a pág:`** de la barra inferior de Freesound y presiona **`Enter`**.
- **¿Cómo toco el nuevo sample con Calor Analógico en mi teclado MIDI?** Pulsa **`📼 Calor Analógico`**, selecciona **`Audio 2`** en el selector `Efectos: [Audio 1] [Audio 2]` y toca las notas **`C1` a `G2`** en tu teclado MIDI (o desactiva `Rebanador MPC` para tocarlo cromáticamente en todas las octavas).

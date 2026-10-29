# 🎹 Manual de Usuario — ClipSampler v2.6.1
### Sampler de Transformación Local, Resintetizador, Rebanador MPC y Explorador Freesound API v2

Bienvenido al **Manual Oficial de Usuario de ClipSampler v2.6.1**, el instrumento virtual y procesador DSP (**VST3 / AU / Standalone**) concebido comercialmente bajo el modelo de **Sampler de Transformación Local** e integración abierta con la biblioteca mundial de sonido **Freesound.org**.

ClipSampler está diseñado para productores musicales, beatmakers y diseñadores de sonido que desean importar, trocear (*slice MPC*), transformar acústicamente, granularizar o convertir en notas MIDI sus **propios archivos de audio (WAV, MP3, AIFF, FLAC, OGG)** desde su disco duro con total privacidad y procesamiento local sin depender de servidores externos, explorar más de 500,000 sonidos libres de derechos directamente desde **Freesound API v2**, incluyendo además un módulo opcional de **Uso Libre** para análisis de enlaces web bajo exclusiva responsabilidad del usuario.

---

## ⚖️ Marco Legal, Modelo Comercial y Responsabilidad del Usuario

> [!IMPORTANT]
> **DESCARGO DE RESPONSABILIDAD LEGAL Y USO ÉTICO DEL SOFTWARE:**
> 1. **Instrumento DSP de Procesamiento Local:** ClipSampler es comercializado exclusivamente como una herramienta de ingeniería de sonido, rebanado rítmico y síntesis para audio provisto por el usuario.
> 2. **Responsabilidad 100% del Usuario:** La procedencia, legalidad y derechos de autor de cualquier muestra, archivo o audio cargado o capturado en el software recaen **única y exclusivamente en el usuario final**.
> 3. **Función Opcional de Uso Libre (Web):** El acceso a la lectura de URLs de internet (YouTube, TikTok, Instagram) permanece **bloqueado por defecto** y requiere la pulsación del botón **`⚖️ Uso Libre (Web)`** donde el usuario acepta explícitamente los términos de uso antes de su habilitación, eximiendo a los desarrolladores de cualquier responsabilidad civil, mercantil o penal derivada del uso ilícito de contenidos protegidos.
> 4. **Aviso de Componentes de Terceros (FFmpeg, yt-dlp y Freesound API):** Este software incluye binarios sin modificar de **FFmpeg** (licencia LGPL 2.1+) disponibles en [ffmpeg.org](https://ffmpeg.org) y **yt-dlp** disponibles en [github.com/yt-dlp/yt-dlp](https://github.com/yt-dlp/yt-dlp), además de conectividad opcional con **Freesound API v2** mediante clave personal del usuario. Para más detalles consulte [`EULA.txt`](file:///c:/Users/joncl/Downloads/Plugin/EULA.txt) y [`LICENCIAS_TERCEROS.txt`](file:///c:/Users/joncl/Downloads/Plugin/LICENCIAS_TERCEROS.txt).

---

## 📑 Índice

1. [Instalación en tu DAW (Windows y macOS)](#1-instalación-en-tu-daw-windows-y-macos)
2. [Período de Prueba (3 Días) y Activación de Licencia](#2-período-de-prueba-3-días-y-activación-de-licencia)
3. [Vista General de la Interfaz Pro a 60 FPS y Ergonomía HIG](#3-vista-general-de-la-interfaz-pro-a-60-fps-y-ergonomía-hig)
4. [Atajos de Teclado Globales y Controles Rápidos](#4-atajos-de-teclado-globales-y-controles-rápidos)
5. [Paso 1: Modelo Sampler Local (Arrastrar WAV/MP3 o Cargar Archivo)](#5-paso-1-modelo-sampler-local-arrastrar-wavmp3-o-cargar-archivo)
6. [Paso 2: Explorador Integrado de Freesound (API v2)](#6-paso-2-explorador-integrado-de-freesound-api-v2)
7. [Paso 3: Función Opcional 'Uso Libre' (Lectura de URLs Web)](#7-paso-3-función-opcional-uso-libre-lectura-de-urls-web)
8. [Paso 4: Doble Visor de Onda, Coloración Analógica y Suite de Derechos IA](#8-paso-4-doble-visor-de-onda-coloración-analógica-y-suite-de-derechos-ia)
9. [Paso 5: Modo Auto-Chop (Estilo MPC) y Rack de Efectos Macro](#9-paso-5-modo-auto-chop-estilo-mpc-y-rack-de-efectos-macro)
10. [Paso 6: Reverse y Half-Time por Pad, Ajuste de Transientes y Exportación](#10-paso-6-reverse-y-half-time-por-pad-ajuste-de-transientes-y-exportación)
11. [Botón Nueva Extracción y Mantenimiento](#11-botón-nueva-extracción-y-mantenimiento)
12. [Administración del Generador de Licencias (Para el Propietario)](#12-administración-del-generador-de-licencias-para-el-propietario)
13. [Solución de Problemas Frecuentes](#13-solución-de-problemas-frecuentes)

---

## 1. Instalación en tu DAW (Windows y macOS)

### 🪟 En Windows 10 / 11 (64-bit)
1. Ejecuta el instalador **`ClipSampler_Windows_Installer_v2.6.1.exe`** como Administrador.
2. Acepta el acuerdo de licencia e instala el plugin:
   - **VST3 (64-bit):** Compatible con FL Studio, Ableton Live, Cubase, Studio One, Reaper, Bitwig, etc. (Se instala automáticamente en `C:\Program Files\Common Files\VST3\ClipSampler.vst3`).
3. El instalador copiará automáticamente las herramientas auxiliares en `C:\ProgramData\ClipSampler\tools` para que el plugin funcione de inmediato.
4. Abre tu DAW, actualiza tu lista de plugins y carga **ClipSampler** en una pista de **Instrumento MIDI / Instrument Track**.

### 🍎 En macOS (Universal Binary: Apple Silicon M1/M2/M3/M4 + Intel)
1. Ejecuta el instalador **`ClipSampler_Installer.pkg`** o copia los binarios en las carpetas estándar del sistema:
   - **`ClipSampler.vst3`** → `/Library/Audio/Plug-Ins/VST3/`
   - **`ClipSampler.component` (Audio Unit)** → `/Library/Audio/Plug-Ins/Components/` (Para Logic Pro y GarageBand)
   - **`ClipSampler.app` (Standalone)** → `/Applications/`

---

## 2. Período de Prueba (3 Días) y Activación de Licencia

Al abrir **ClipSampler v2.6.1** por primera vez:
- Contarás automáticamente con **3 días (72 horas) de prueba gratuita** con todas las funciones desbloqueadas.
- En la esquina superior derecha verás el botón **`⏳ PRUEBA (3 DÍAS)`**.
- Al hacer clic sobre ese botón (o cuando finalicen los 3 días), se abrirá la ventana de **Activación de Licencia**:
  1. Verás tu **ID de Máquina único** (formato `MCH-XXXX-XXXX-XXXX`). Puedes pulsar **📋 Copiar ID** para enviarlo al administrador o usarlo en tu generador de licencias.
  2. Ingresa tu código de licencia (`CS10-XXXX-XXXX-XXXX-XXXX`) en el campo de texto.
  3. Pulsa **🔓 ACTIVAR LICENCIA**. El botón superior cambiará a **`✅ LICENCIA PRO`** en color verde esmeralda y el plugin quedará activado permanentemente en tu equipo.

---

## 3. Vista General de la Interfaz Pro a 60 FPS y Ergonomía HIG

La interfaz de **ClipSampler v2.6.1** está construida bajo la arquitectura **`diseno-gui-pc-pro`** (*Layered Dark Mode + Hardware DSP LookAndFeel*):
- **Paleta Dark Mode por Capas de Elevación:**
  - **Nivel 0 (Chasis Base):** Obsidiana Carbono profundo (`#090E1C`) con iluminación ambiental dinámica y encabezado en Titanio Anodizado con **indicador LED de estado en vivo**.
  - **Nivel 1 (Tarjetas de Sección):** Paneles elevados con sombra proyectada, bisel especular superior de `1px` y cápsulas de título con barra LED lateral.
  - **Nivel 2 (Pozos de Osciloscopio y Cajas de Texto):** Visores empotrados (`#060B16`) con líneas de referencia de decibelios (`+6 dB`, `0 dB`, `-6 dB`) y anillo de foco cian (`#38BDF8`).
- **Perillas Rotatorias Hardware DSP:** Corona de **11 marcas radiales** que se iluminan progresivamente según el valor actual, arco de valor con halo luminoso (*glow*) y aguja con punto LED.
- **Fluidez Visual a 60 FPS:** Cabezal de reproducción (*playhead*) y animaciones renderizadas a **60 fotogramas por segundo** sin consumo extra en el hilo de audio.
- **Tooltips Flotantes y Cursores Interactivos:** Al pasar el cursor sobre cualquier botón o perilla, el puntero cambia a mano interactiva y muestra una descripción emergente con su función y atajo de teclado.
- **Redimensionamiento 100% Adaptable (Responsive):** Arrastra la esquina inferior derecha desde tamaños compactos (`680x540`) hasta monitores 4K (`1800x1300`) sin que ningún texto se recorte.

---

## 4. Atajos de Teclado Globales y Controles Rápidos

**ClipSampler v2.6.1** incorpora atajos de teclado de nivel de estudio (**Human Interface Guidelines**) para acelerar tu flujo de producción sin depender del ratón:

| Atajo (Windows) | Atajo (macOS) | Acción Inmediata |
| :--- | :--- | :--- |
| **`Espacio`** | **`Espacio`** | **Reproducir / Detener** vista previa de la muestra activa. |
| **`Ctrl + F`** | **`Cmd + F`** | Abrir el explorador integrado **Freesound API v2**. |
| **`Ctrl + O`** | **`Cmd + O`** | Abrir ventana para **Cargar archivo de audio local**. |
| **`Ctrl + M`** | **`Cmd + M`** | **Convertir audio a notas MIDI** para Piano Roll. |
| **`Ctrl + E`** | **`Cmd + E`** | **Exportar muestra activa** como archivo `.WAV` (24-bit PCM). |
| **`Teclas 1 a 8`** | **`Teclas 1 a 8`** | **Disparar en vivo las rodajas MPC (`C1` a `G1`)** desde el teclado de tu PC. |
| **`Escape`** | **`Escape`** | **Cerrar ventana modal** activa (Freesound / Licencia) o detener reproducción. |
| **`Doble Clic en Perilla`** | **`Doble Clic en Perilla`** | **Restablecer la perilla** a su valor neutro de fábrica (`Lo-Fi: 0%`, `Tono: 0 st`, `Filtro: 100%`, `Atk: 2 ms`, `Rel: 50 ms`). |

---

## 5. Paso 1: Modelo Sampler Local (Arrastrar WAV/MP3 o Cargar Archivo)

**ClipSampler v2.6.1** funciona de forma nativa e inmediata como un **Sampler de Transformación Local**:

1. **Arrastrar y Soltar desde el DAW o Explorador de Archivos (Drag & Drop Directo):**
   - Arrastra cualquier archivo de audio en formato **WAV, MP3, AIFF, FLAC u OGG** directamente desde el explorador de Windows/Finder o desde el browser de muestras de tu DAW (FL Studio, Ableton, Logic Pro, Reaper, etc.) y suéltalo sobre cualquiera de los visores de onda.
   - El visor se iluminará con un halo cian reactivo al pasar el archivo por encima y se cargará en memoria RAM instantáneamente.
2. **Botón `📂 Cargar Audio Local` (o `Ctrl/Cmd + O`):**
   - Haz clic en este botón (ubicado a la izquierda en la Fila 0) o presiona `Ctrl+O` / `Cmd+O` para elegir cualquier sample o grabación de tu librería.
3. **Procesamiento Inmediato:**
   - Al cargar el archivo local, ClipSampler calcula automáticamente su **BPM**, detecta su **Tonalidad Musical (Key)**, ajusta la frecuencia de muestreo al proyecto de tu DAW y genera las rebanadas automáticas para tocar en pads MIDI (**C1 a G2**) o con las teclas **`1` a `8`** de tu computadora.

---

## 6. Paso 2: Explorador Integrado de Freesound (API v2)

**ClipSampler v2.6.1** integra un navegador modal completo conectado directamente a la API oficial v2 de **Freesound.org** para buscar, filtrar, preescuchar y cargar sonidos libres de derechos directamente a tus pads (accesible con el botón **`🎵 Freesound`** o el atajo **`Ctrl/Cmd + F`**).

### 🔑 Activación con tu API Key Gratuita
1. Haz clic en el botón naranja **`🎵 Freesound`** (o presiona **`Ctrl/Cmd + F`**).
2. Si es la primera vez que lo abres, verás una ventana modal con el cuadro central: **"Pega tu API Key de Freesound aquí"**.
3. Haz clic en el enlace **`🌐 ¿Cómo obtener una API Key gratuita? (freesound.org)`** o visita directamente [freesound.org/apiv2/apply](https://freesound.org/apiv2/apply) para registrarte y generar una clave gratuita al instante.
4. Pulsa el botón **`📋 Pegar`** y haz clic en **`🔓 Conectar y Cargar Buscador`**.
5. Tu API Key se guardará de forma segura y persistente, por lo que no necesitarás volver a introducirla en futuras sesiones. Si deseas actualizarla o cambiarla más adelante, dispones del botón **`🔑 Cambiar API Key`**.

### 🔍 Buscador, Filtros y Descriptores de Audio Avanzados
Una vez conectada la API Key, se desplegará el panel del buscador conectado al endpoint `/apiv2/search/text/`:
- **Caja de Búsqueda:** Escribe cualquier término (ej. *snare, kick, 808, vinyl scratch, synth pad, vocal chop, ambient rain*) y pulsa Enter o haz clic en **`🔍 Buscar Sonidos`**.
- **Filtro de Duración:** *Cualquiera*, *One-Shots (0 a 2s)*, *Loops Cortos (2 a 6s)*, *Frases (6 a 15s)*, *Muestras Largas (15s a 5m)*.
- **Filtro de Ordenación:** *Relevancia (Score)*, *Más Descargados*, *Mejor Calificados*, *Más Recientes*, *Más Cortos*.
- **Filtro de Licencia:** *Todas las Licencias*, *Creative Commons 0 (Dominio Público)*, *Creative Commons Attribution (CC-BY)*.
- **Filtro de Formato:** *Cualquier Formato*, *Solo WAV*, *Solo FLAC*, *WAV o FLAC*, *Solo AIFF*.

### 🎧 Enrutamiento de Audio Híbrido, Sonidos Similares y Carga en Pads
- **Botón `🔊 Salida: Auto / Mixer DAW / Directo`:** Permite elegir cómo se escucha la previsualización:
  - *Auto:* Enruta por el mezclador del DAW y, si el transporte del DAW está en pausa, activa automáticamente el motor de salida directa de Windows/macOS para que siempre escuches el sonido.
  - *Mixer DAW:* Enruta exclusivamente a través del canal de pista del plugin en tu DAW.
  - *Directo:* Reproduce directamente hacia tu tarjeta de sonido del sistema.
- **`▶ Escuchar` / `⏹ Detener`:** Audición inmediata de la muestra en alta calidad.
- **`🔗 Similares`:** Consulta el análisis espectral de Freesound para mostrarte sonidos acústicamente similares al instante.
- **`⚡ Cargar en Pad`:** Descarga la muestra a la caché local, la carga automáticamente en el **Visor 1**, calcula su **BPM y Tono** y activa el **Rebanador MPC (C1 a G2)**.
- Pulsa **`✕ Volver a ClipSampler`** o la tecla **`Escape`** para regresar a la vista principal.

---

## 7. Paso 3: Función Opcional 'Uso Libre' (Lectura de URLs Web)

Para habilitar la captura opcional desde plataformas de internet:

1. **Botón `⚖️ Uso Libre (Web)`:**
   - Haz clic en **`⚖️ Uso Libre (Web)`** y acepta el **Aviso Legal y Descargo de Responsabilidad** para desbloquear el campo de URL y los selectores de **YouTube, TikTok e Instagram**.
2. **Extracción y Recorte (Segundos o Compases):**
   - **Pegar Enlace:** Pega la URL del video. El selector de plataforma se auto-configurará.
   - **`⏱️ Segundos` / `🎵 Compases`:** Elige si deseas recortar por tiempo fijo (`3s`, `10s`, `30s`) o por compases musicales exactos (`1 Bar`, `2 Bars`, `4 Bars`) sincronizados al tempo de tu DAW.
   - **`🔄 Nueva Extracción`:** Limpia ambos visores y permite iniciar una nueva sesión con un solo clic.

---

## 8. Paso 4: Doble Visor de Onda, Coloración Analógica y Suite de Derechos IA

En la Fila 4 dispones de la suite de **Modificación y Coloración de Audio**:
- **`📼 Calor Analógico`:** Aplica saturación armónica de cinta vintage (*tape warmth*), realce de cuerpo en graves (~160 Hz estilo Pultec) y suavizado de agudos extremos, cargando el resultado en el **Visor 2**.
- **`▲ Alterar +3 Tonos Más` y `▼ Alterar -3 Tonos Menos`:** Aplica interpolación de Lagrange, preservación de formantes y dispersión de fase all-pass para transformar la huella acústica y cargar el resultado en el **Visor 2**.
- **`🛡️ Verificar Derechos`:** Evalúa matemáticamente la divergencia espectral entre el Audio 1 y el Audio 2 (`✅ LIBRE DE DERECHOS`).

---

## 9. Paso 5: Modo Auto-Chop (Estilo MPC) y Rack de Efectos Macro

En la barra superior del Panel Inferior:
1. **Selector de Objetivo (`Efectos: Audio 1 / Audio 2`):** Elige cuál de los dos visores procesas y tocas en tiempo real.
2. **5 Perillas Hardware DSP con Reset por Doble Clic:**
   - **`Lo-Fi (0% - 100%)`:** Reducción de resolución vintage de 12 bits (*Doble clic = 0%*).
   - **`Tono (-12 st a +12 st)`:** Afinación cromática en semitonos (*Doble clic = 0 st*).
   - **`Filtro (15% a 100%)`:** Filtro paso-bajo analógico cálido (*Doble clic = 100%*).
   - **`Atk (0.5 ms a 500 ms)`:** Ataque anti-chasquidos (*Doble clic = 2 ms*).
   - **`Rel (5 ms a 2000 ms)`:** Caída suave al soltar la nota (*Doble clic = 50 ms*).
3. **Botones `Sinc. BPM`, `Aplicar FX` y `Rebanador MPC`:** Sincroniza al tempo del DAW, imprime los efectos en el archivo WAV y conmuta entre modo Rebanador MPC (`C1..G2`) o modo Cromático.

---

## 10. Paso 6: Reverse y Half-Time (Todo el Audio o Por Corte), Ajuste de Transientes y Exportación

- **Etiqueta Exacta de Origen en el Visor:** En la esquina superior derecha de cada visor se muestra el origen exacto de la muestra cargada: **`[LOCAL]`** (archivo desde tu PC), **`[FREESOUND]`** (muestra descargada desde Freesound API v2), **`[YOUTUBE]`**, **`[TIKTOK]`** o **`[INSTAGRAM]`**, junto con su BPM, Tonalidad y duración.
- **Modificadores Inteligentes (`◀ Rev` y `⏳ 1/2`):**
  - **Modo Audio Completo (1 Clic en el visor):** Si haces **1 solo clic** en la forma de onda (`[TODO EL AUDIO]`) y pulsas **`◀ Rev`** o **`⏳ 1/2`**, se invertirá o aplicará media velocidad a **TODO el audio completo de principio a fin** sin seleccionar un corte individual.
  - **Modo Corte Exacto (Doble Clic en una nota):** Si haces **doble clic** sobre una rodaja específica (ej. `C1`, `D1`, `F#1`) y pulsas **`◀ Rev`** o **`⏳ 1/2`**, el efecto se aplicará **únicamente a ese corte exacto** (también disponible haciendo **clic derecho** sobre el visor).
- **Ajuste Manual de Transientes:** Arrastra con el ratón cualquiera de las líneas divisorias amarillas (`↔`) para ajustar el inicio o fin de cada corte con precisión milimétrica.
- **Selección y Exportación Rápida:**
  - **1 Clic en el visor:** Selecciona y reproduce el audio completo.
  - **Doble Clic en una nota del visor:** Selecciona únicamente ese **Corte Exacto** (`[✂ CORTE EXACTO: C1]`).
  - **Arrastrar `.WAV` / `✂ Corte Exacto`:** Mantén presionado y arrastra directamente hacia cualquier pista de tu DAW (incluyendo el efecto Reverse o Half-Time si está activo).
  - **`🎹 Convertir a MIDI` (o `Ctrl/Cmd + M`):** Transcribe el audio a notas MIDI y permite arrastrar el archivo `.mid` al Piano Roll.
  - **`Exportar .WAV...` (o `Ctrl/Cmd + E`):** Guarda la muestra completa o rodaja activa en disco en formato WAV de 24 bits.

---

## 11. Botón Nueva Extracción y Mantenimiento

En la esquina superior derecha del encabezado:
- **`Actualizar Motor`:** Ejecuta `yt-dlp -U` en segundo plano para mantener actualizado el extractor web.
- **`Limpiar Caché`:** Elimina archivos temporales antiguos preservando las muestras cargadas actualmente en los visores.

---

## 12. Administración del Generador de Licencias (Para el Propietario)

1. Entra a la carpeta `license_system` y ejecuta **`Iniciar_Generador_Licencias.bat`**.
2. Accede a `http://127.0.0.1:8765` para generar licencias **Atadas a ID de Máquina (`MCH-...`)** o **Auto-Vinculables Web (`CS10-AUTO-...`)**, exportar clientes a CSV o liberar equipos.

---

## 13. Solución de Problemas Frecuentes

- **El texto o los botones se ven pequeños en mi monitor 4K:** Arrastra la esquina inferior derecha del plugin para ampliar la ventana; toda la interfaz y las fuentes escalarán proporcionalmente a 60 FPS.
- **No escucho la previsualización de Freesound cuando el DAW está detenido:** Asegúrate de que el botón superior derecho del buscador Freesound esté en **`🔊 Salida: Auto`** o **`🔊 Salida: Directo`**, lo cual activa el reproductor directo de tarjeta de sonido.
- **Quiero probar las rodajas MPC sin un teclado MIDI conectado:** Simplemente presiona las teclas numéricas **`1` a `8`** en el teclado de tu computadora para disparar las primeras 8 rodajas (`C1` a `G1`) en tiempo real.

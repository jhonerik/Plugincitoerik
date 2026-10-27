# 🎹 Manual de Usuario — ClipSampler v2.4

Bienvenido al **Manual Oficial de Usuario de ClipSampler v2.4**, el instrumento virtual (**VST3 / AAX**) diseñado para productores musicales, beatmakers y diseñadores de sonido que desean capturar fragmentos de audio desde **YouTube, TikTok e Instagram**, transformarlos creativamente y convertirlos en samples listos para tocar en MIDI o arrastrar directamente al DAW.

---

## 📑 Índice

1. [Instalación en tu DAW](#1-instalación-en-tu-daw)
2. [Período de Prueba (3 Días) y Activación de Licencia](#2-período-de-prueba-3-días-y-activación-de-licencia)
3. [Vista General de la Interfaz (Responsive)](#3-vista-general-de-la-interfaz-responsive)
4. [Paso 1: Seleccionar Plataforma y Analizar Enlace](#4-paso-1-seleccionar-plataforma-y-analizar-enlace)
5. [Paso 2: Elegir Tiempo de Inicio y Duración (3s, 10s o 30s)](#5-paso-2-elegir-tiempo-de-inicio-y-duración-3s-10s-o-30s)
6. [Paso 3: Doble Visor de Onda, Derechos IA y Detector de Derechos](#6-paso-3-doble-visor-de-onda-derechos-ia-y-detector-de-derechos)
7. [Paso 4: Modo Auto-Chop (Estilo MPC) y Rack de Efectos Macro](#7-paso-4-modo-auto-chop-estilo-mpc-y-rack-de-efectos-macro)
8. [Paso 5: Arrastrar Audio (.WAV) y Arrastrar Notas (.MIDI) al DAW](#8-paso-5-arrastrar-audio-wav-y-arrastrar-notas-midi-al-daw)
9. [Botón Nueva Extracción y Mantenimiento](#9-botón-nueva-extracción-y-mantenimiento)
10. [Administración del Generador de Licencias (Para el Propietario)](#10-administración-del-generador-de-licencias-para-el-propietario)
11. [Solución de Problemas Frecuentes](#11-solución-de-problemas-frecuentes)

---

## 1. Instalación en tu DAW

1. Ejecuta el instalador **`ClipSampler_Windows_Installer_v2.4.0.exe`** como Administrador.
2. Acepta el acuerdo de licencia y selecciona los formatos que utilices en tu estudio:
   - **VST3 (64-bit):** Compatible con FL Studio, Ableton Live, Cubase, Studio One, Reaper, Bitwig, etc. (Se instala en `C:\Program Files\Common Files\VST3\ClipSampler.vst3`).
   - **AAX (64-bit):** Compatible con Avid Pro Tools.
3. El instalador copiará automáticamente las herramientas de extracción (`yt-dlp.exe` y `ffmpeg.exe`) en `C:\ProgramData\ClipSampler\tools` para que el plugin funcione de inmediato sin configuraciones adicionales.
4. Abre tu DAW, actualiza tu lista de plugins y carga **ClipSampler** en una pista de **Instrumento MIDI / Instrument Track**.

---

## 2. Período de Prueba (3 Días) y Activación de Licencia

Al abrir **ClipSampler v2.4** por primera vez:
- Contarás automáticamente con **3 días (72 horas) de prueba gratuita** con todas las funciones desbloqueadas.
- En la esquina superior derecha verás el botón **`⏳ PRUEBA (3 DÍAS)`**.
- Al hacer clic sobre ese botón (o cuando finalicen los 3 días), se abrirá la ventana de **Activación de Licencia**:
  1. Verás tu **ID de Máquina único** (formato `MCH-XXXX-XXXX-XXXX`). Puedes pulsar **📋 Copiar ID** para enviarlo al administrador o usarlo en tu generador de licencias.
  2. Ingresa tu código de licencia (`CS10-XXXX-XXXX-XXXX-XXXX`) en el campo de texto.
  3. Pulsa **🔓 ACTIVAR LICENCIA**. El botón superior cambiará a **`✅ LICENCIA PRO`** en color verde esmeralda y el plugin quedará activado permanentemente en tu equipo.

---

## 3. Vista General de la Interfaz (Responsive)

La interfaz de **ClipSampler v2.4** cuenta con un diseño de estudio **Royal Sapphire & Satin Titanium** (azul zafiro profundo con iluminación ambiental) y es **100% adaptable (Responsive)**:
- **Redimensionamiento libre:** Puedes arrastrar la esquina inferior derecha del plugin para achicarlo (`680x540`) en pantallas portátiles o expandirlo (`1800x1300`) en monitores de estudio 4K.
- **Tipografía inteligente:** Todos los botones, etiquetas y encabezados escalan automáticamente su tamaño de letra para que ningún texto se corte ni se superponga.

---

## 4. Paso 1: Seleccionar Plataforma y Analizar Enlace

En la **Fila 1 (Plataforma + URL)**:

1. **Selector de Plataforma:** Elige el origen de tu enlace:
   - **🌐 Auto:** Detecta automáticamente a qué red pertenece el link.
   - **▶️ YouTube:** Optimizado para videos, canciones y Shorts de YouTube.
   - **🎵 TikTok:** Optimizado para videos públicos de TikTok.
   - **📸 Instagram:** Optimizado para Reels y publicaciones de video de Instagram.
2. **Campo de Enlace:** Pega la URL del video. Notarás que el selector de plataforma cambia automáticamente según el enlace pegado.
3. **Botón `🔍 Analizar`:** Haz clic aquí si deseas que el plugin lea el título y la duración total del video antes de descargar nada. El deslizador de tiempo se ajustará automáticamente a la duración exacta del video (por ejemplo, `00:00 / 03:45`).

---

## 5. Paso 2: Elegir Tiempo de Inicio y Duración (3s, 10s o 30s)

En la **Fila 2 (Tiempo, Duración y Captura)**:

1. **Inicio (`MM:SS` o segundos):** Puedes escribir el minuto y segundo exacto donde quieres que empiece la extracción (ej. `01:15` o `75`) o mover la **barra deslizadora de tiempo** que aparece al lado.
2. **Botones de Duración (`3s`, `10s`, `30s`):**
   - **`3s`:** Ideal para capturar golpes de batería (one-shots), efectos vocales cortos o tags.
   - **`10s`:** Ideal para frases melódicas, acordes o loops de 2 a 4 compases.
   - **`30s`:** Ideal para secciones completas, intros o muestras largas para chopear.
3. **Casilla `Norm -1dB`:** Si está activada (recomendado), nivela el volumen del audio extraído a `-1.0 dB` y aplica micro-fundidos de 10 ms al inicio y final para evitar chasquidos (*clicks*).
4. **Botón `⚡ EXTRAER`:** Inicia la descarga y recorte en alta fidelidad (sincronizado automáticamente con los Hz de tu DAW: 44.1 kHz, 48 kHz, 96 kHz, etc.).
5. **Botón `🔄 Nueva Extracción`:** Se habilita cuando ya tienes los dos visores con audio y te permite limpiar toda la sesión con un solo clic para pegar un nuevo enlace desde cero.

---

## 6. Paso 3: Doble Visor de Onda, Derechos IA y Detector de Derechos

**ClipSampler v2.4** divide la visualización en **2 pantallas acústicas independientes**:

### 🔹 Visor 1 (Izquierda): `1. MUESTRA ORIGINAL EXTRAÍDA`
- Muestra la forma de onda original extraída de YouTube, TikTok o Instagram.
- En su esquina superior derecha muestra automáticamente el **BPM detectado** y la **Tonalidad musical (Key)** (por ejemplo: `124.0 BPM | Tono: A minor`).
- Puedes hacer clic en cualquier punto de la onda para escuchar desde esa posición.

### 🔹 Visor 2 (Derecha): `2. AUDIO PROCESADO (IA DERECHOS)`
- Se activa al pulsar el botón **`🤖 Derechos IA (±3 st)`** de la Fila 3.
- Este motor aplica procesamiento DSP transformativo (desplazamiento de tono de ±3 semitonos con preservación de formantes, micro-estiramiento temporal y ecualización sutil) para alterar la huella acústica de la muestra original.

### 🛡️ Botón `🛡️ Detectar Derechos`
- Analiza matemáticamente la correlación espectral entre la muestra y patrones de Content ID:
  - Si solo has extraído el **Visor 1**, te indicará el riesgo de coincidencia de derechos del audio original.
  - Si ya generaste el **Visor 2 con Derechos IA**, verificará y confirmará que la huella acústica del Visor 2 ha sido transformada exitosamente (`✅ LIBRE DE DERECHOS`).

---

## 7. Paso 4: Modo Auto-Chop (Estilo MPC) y Rack de Efectos Macro

En la **Fila 4 (Rack de Efectos Macro + Modo MPC)**:

1. **Selector de Objetivo (`🎯 Aplicar FX a: Visor 1 / Visor 2`):**
   - Elige cuál de los dos audios (el Original del Visor 1 o el Procesado del Visor 2) quieres tocar con tu teclado MIDI y procesar con los efectos en tiempo real.
2. **Botón `🥁 Modo Auto-Chop (MPC)`:**
   - **Desactivado (Modo Cromático):** Puedes tocar el sample melódicamente en todo tu teclado MIDI (como un instrumento tradicional donde `C3` es la nota original).
   - **Activado (Modo Auto-Chop MPC):** El plugin detecta automáticamente los golpes de ritmo (transientes) de la muestra, dibuja líneas divisorias amarillas (`C1`, `C#1`, `D1`... hasta `D#2`) sobre la forma de onda y asigna cada pedacito a una tecla de tu controlador MIDI o Piano Roll para que puedas reordenar el sample y armar un beat al instante.
3. **Perilla `📻 Lo-Fi (0% - 100%)`:**
   - Añade reducción de resolución y calidez vintage estilo sampler clásico de 12 bits (SP-1200 / MPC60).
4. **Perilla `🌀 Pitch (-12 st a +12 st)`:**
   - Sube o baja la afinación del sample hasta una octava completa en semitonos.
5. **Perilla `🎚️ Filtro (15% a 100%)`:**
   - Filtro paso-bajo cálido para suavizar agudos o crear ambientes envolventes.
6. **Perilla `⚡ Atk (Ataque: 0.5 ms a 500 ms)`:**
   - Envolvente de ataque anti-chasquidos. Suaviza el inicio del golpe para eliminar cualquier "pop/click" al sonar, o permite crear efectos de colchón/violín (*pad swell*).
7. **Perilla `⏳ Rel (Caída / Release: 5 ms a 2000 ms)`:**
   - Determina cómo termina el sonido al soltar la tecla o pad MIDI: en corte seco inmediato o desvaneciéndose con una cola suave y musical.
8. **Botón `🔗 Sync BPM DAW`:**
   - Sincroniza automáticamente la velocidad de reproducción del sample con el tempo (BPM) actual de tu proyecto en el DAW.

---

## 8. Paso 5: Selección por Corte Exacto (`✂ Corte Exacto`), Ajuste Manual de Transientes, Arrastrar Audio (.WAV) y Conversor a MIDI (`🎹 Convertir a MIDI`)

En los **Visores de Onda** y en la **Fila 5 (Barra Inferior de Transporte, Corte Exacto y Exportación)**:

- **Ajuste Manual de Transientes (Mover Líneas de Corte con el Cursor):**
  - Pasa el cursor del ratón sobre cualquiera de las líneas divisorias amarillas entre rodajas en el visor.
  - El cursor cambiará automáticamente a la flecha doble de ajuste (`↔`).
  - **Haz clic y arrastra hacia la izquierda o derecha** para mover el inicio o final del corte con precisión milimétrica. La onda se actualiza en tiempo real y el audio respetará el nuevo corte al instante al tocarlo en MIDI o arrastrarlo al DAW.
- **Atajo de Teclado (Barra Espaciadora / Spacebar):**
  - Presiona la **Barra Espaciadora** en tu teclado para **reproducir o detener** el audio del plugin en cualquier momento sin necesidad de mover el ratón hasta el botón de play.
- **Selección de Audio Completo (1 Clic):** Haz **1 solo clic** sobre el visor de onda (Audio 1 o Audio 2) para seleccionar y reproducir **el audio completo**. Al arrastrar hacia el DAW se enviará la muestra completa.
- **Selección de Corte Exacto por Nota (2 Clics Seguidos / Doble Clic):** Haz **2 clics seguidos (doble clic)** sobre cualquier nota o división del visor (`C1`, `C#1`, `D1`, `D#1`, `E1`, `F1`, `F#1`, etc.):
  - Se iluminará únicamente ese trozo exacto en color cian/dorado con el indicador **`[✂ CORTE EXACTO: C1]`** y se reproducirá solo ese fragmento.
  - El plugin generará automáticamente un archivo `.WAV` independiente (24-bit PCM con micro-fundidos anti-clic) que contiene **exclusivamente ese corte exacto**.
- **Botón `✂ Corte Exacto`:**
  - Si haces clic en **`✂ Corte Exacto`**, selecciona y reproduce el corte exacto activo (por defecto `C1` si aún no habías hecho doble clic en otro trozo).
  - Mantén presionado el clic izquierdo sobre el botón **`✂ Corte Exacto`** (o directamente sobre la nota iluminada en el visor de onda) y **arrástralo hacia una pista de tu DAW** para pasar únicamente ese trozo de audio y no la muestra completa.
- **Botón `▶ Reproducir` / `⏹ Detener` (o Barra Espaciadora):** Escucha directamente la muestra activa (o el corte exacto seleccionado) a través del mezclador de tu DAW.
- **Botón `Arrastrar .WAV`:** Mantén presionado el clic izquierdo sobre este botón (o directamente sobre cualquiera de los dos visores de onda) y arrástralo hacia una pista de audio o sampler de tu DAW. Si tienes un corte exacto seleccionado (ej. `C1`), el botón mostrará `Arrastrar C1 (.WAV)` y arrastrará únicamente ese trozo.
- **Botón `🎹 Convertir a MIDI`:** Haz clic o mantén presionado el clic izquierdo sobre este botón y arrástralo hacia una pista MIDI o al Piano Roll de tu DAW. **ClipSampler** analiza el audio, transcribe automáticamente sus notas melódicas y transientes, y genera un archivo `.mid` listo para ser interpretado por cualquier sintetizador o instrumento virtual (Serum, Omnisphere, FL Keys, etc.).

---

## 9. Botón Nueva Extracción y Mantenimiento

En la esquina superior derecha del encabezado encontrarás dos botones de mantenimiento rápido:
- **`🔄 Actualizar Motor`:** Ejecuta `yt-dlp -U` en segundo plano para actualizar el motor de descarga cuando YouTube, TikTok o Instagram cambien sus algoritmos.
- **`🗑️ Limpiar Caché`:** Elimina de forma segura los archivos `.wav` y `.mid` antiguos guardados en la carpeta temporal (`%APPDATA%\ClipSampler\AudioCache`) sin borrar las muestras que tienes cargadas actualmente en pantalla.

---

## 10. Administración del Generador de Licencias (Para el Propietario)

Si eres el administrador del software y deseas generar licencias para tus clientes:

1. Entra a la carpeta [`license_system`](file:///c:/Users/joncl/Downloads/Plugin/license_system) y haz doble clic en **`Iniciar_Generador_Licencias.bat`**.
2. Se abrirá automáticamente tu navegador en `http://127.0.0.1:8765` con el **Panel de Licencias y Base de Datos SQLite (`licenses.db`)**.
3. Puedes elegir entre dos modos:
   - **🔒 Atada a ID de Máquina (Offline/Online):** Pegas el ID de Máquina del cliente (`MCH-XXXX-XXXX-XXXX`) y generas un código que solo funcionará matemáticamente en esa computadora.
   - **🌐 Auto-Vinculable Web:** Generas un código `CS10-AUTO-XXXX-XXXX-XXXX` que se vincula automáticamente a la primera computadora que lo active por internet y bloquea cualquier otra PC.
4. Desde el mismo panel puedes **buscar clientes, exportar a CSV, liberar una PC o revocar licencias** en tiempo real.

---

## 11. Solución de Problemas Frecuentes

- **El texto o los botones se ven pequeños en mi monitor 4K:**
  Arrastra el triángulo de la esquina inferior derecha del plugin para ampliar la ventana al tamaño que prefieras; toda la interfaz y las fuentes crecerán proporcionalmente.
- **Un enlace de TikTok o Instagram da error de extracción:**
  Asegúrate de que el video o Reel sea **público** (no de una cuenta privada) y pulsa el botón **`🔄 Actualizar Motor`** en la barra superior para tener la última versión del extractor.
- **No suena al presionar teclas en mi controlador MIDI:**
  Verifica si tienes activado el **`🥁 Modo Auto-Chop (MPC)`**. En modo Auto-Chop las rebanadas se disparan desde la octava **`C1` a `D#2`** (notas MIDI 36 a 51). Si deseas tocar en todo el teclado, desactiva la casilla Auto-Chop.

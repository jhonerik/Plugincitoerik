# 🎹 Manual de Usuario — ClipSampler v2.6
### Sampler de Transformación Local, Resintetizador, Rebanador MPC y Explorador Freesound API v2

Bienvenido al **Manual Oficial de Usuario de ClipSampler v2.6**, el instrumento virtual y procesador DSP (**VST3 / AAX**) concebido comercialmente bajo el modelo de **Sampler de Transformación Local** e integración abierta con la biblioteca mundial de sonido **Freesound.org**. 

ClipSampler está diseñado para productores musicales, beatmakers y diseñadores de sonido que desean importar, trocear (slice MPC), transformar acústicamente, granularizar o convertir en notas MIDI sus **propios archivos de audio (WAV, MP3, AIFF, FLAC, OGG)** desde su disco duro con total privacidad y procesamiento local sin depender de servidores externos, explorar millones de sonidos libres de derechos directamente desde **Freesound**, incluyendo además un módulo opcional de **Uso Libre** para análisis de enlaces web bajo exclusiva responsabilidad del usuario.

---

## ⚖️ Marco Legal, Modelo Comercial y Responsabilidad del Usuario

> [!IMPORTANT]
> **DESCARGO DE RESPONSABILIDAD LEGAL Y USO ÉTICO DEL SOFTWARE:**
> 1. **Instrumento DSP de Procesamiento Local:** ClipSampler es comercializado exclusivamente como una herramienta de ingeniería de sonido, rebanado rítmico y síntesis para audio provisto por el usuario.
> 2. **Responsabilidad 100% del Usuario:** La procedencia, legalidad y derechos de autor de cualquier muestra, archivo o audio cargado o capturado en el software recaen **única y exclusivamente en el usuario final**.
> 3. **Función Opcional de Uso Libre (Web):** El acceso a la lectura de URLs de internet (YouTube, TikTok, Instagram) permanece **bloqueado por defecto** y requiere la pulsación del botón **`⚖️ Uso Libre (Web)`** donde el usuario acepta explícitamente los términos de uso antes de su habilitación, eximiendo a los desarrolladores de cualquier responsabilidad civil, mercantil o penal derivada del uso ilícito de contenidos protegidos.
> 4. **Aviso de Componentes de Terceros (FFmpeg y yt-dlp):** Este software incluye binarios sin modificar de **FFmpeg** (licencia LGPL 2.1+) disponibles en [ffmpeg.org](https://ffmpeg.org) y **yt-dlp** disponibles en [github.com/yt-dlp/yt-dlp](https://github.com/yt-dlp/yt-dlp). Para más detalles y acceso al código fuente abierto de estas herramientas, consulte el archivo [`LICENCIAS_TERCEROS.txt`](file:///c:/Users/joncl/Downloads/Plugin/LICENCIAS_TERCEROS.txt).

---

## 📑 Índice

1. [Instalación en tu DAW](#1-instalación-en-tu-daw)
2. [Período de Prueba (3 Días) y Activación de Licencia](#2-período-de-prueba-3-días-y-activación-de-licencia)
3. [Vista General de la Interfaz (Responsive)](#3-vista-general-de-la-interfaz-responsive)
4. [Paso 1: Modelo Sampler Local (Arrastrar WAV/MP3 o Cargar Archivo)](#4-paso-1-modelo-sampler-local-arrastrar-wavmp3-o-cargar-archivo)
5. [Paso 2: Explorador Integrado de Freesound (API v2)](#5-paso-2-explorador-integrado-de-freesound-api-v2)
6. [Paso 3: Función Opcional 'Uso Libre' (Lectura de URLs Web)](#6-paso-3-función-opcional-uso-libre-lectura-de-urls-web)
7. [Paso 4: Doble Visor de Onda, Coloración Analógica y Suite de Derechos IA](#7-paso-4-doble-visor-de-onda-coloración-analógica-y-suite-de-derechos-ia)
8. [Paso 5: Modo Auto-Chop (Estilo MPC) y Rack de Efectos Macro](#8-paso-5-modo-auto-chop-estilo-mpc-y-rack-de-efectos-macro)
9. [Paso 6: Reverse y Half-Time por Pad, Ajuste de Transientes y Exportación](#9-paso-6-reverse-y-half-time-por-pad-ajuste-de-transientes-y-exportación)
10. [Botón Nueva Extracción y Mantenimiento](#10-botón-nueva-extracción-y-mantenimiento)
11. [Administración del Generador de Licencias (Para el Propietario)](#11-administración-del-generador-de-licencias-para-el-propietario)
12. [Solución de Problemas Frecuentes](#12-solución-de-problemas-frecuentes)

---

## 1. Instalación en tu DAW

1. Ejecuta el instalador **`ClipSampler_Windows_Installer_v2.6.0.exe`** como Administrador.
2. Acepta el acuerdo de licencia y selecciona los formatos que utilices en tu estudio:
   - **VST3 (64-bit):** Compatible con FL Studio, Ableton Live, Cubase, Studio One, Reaper, Bitwig, etc. (Se instala en `C:\Program Files\Common Files\VST3\ClipSampler.vst3`).
   - **AAX (64-bit):** Compatible con Avid Pro Tools.
3. El instalador copiará automáticamente las herramientas auxiliares en `C:\ProgramData\ClipSampler\tools` para que el plugin funcione de inmediato sin configuraciones adicionales.
4. Abre tu DAW, actualiza tu lista de plugins y carga **ClipSampler** en una pista de **Instrumento MIDI / Instrument Track**.

---

## 2. Período de Prueba (3 Días) y Activación de Licencia

Al abrir **ClipSampler v2.5** por primera vez:
- Contarás automáticamente con **3 días (72 horas) de prueba gratuita** con todas las funciones desbloqueadas.
- En la esquina superior derecha verás el botón **`⏳ PRUEBA (3 DÍAS)`**.
- Al hacer clic sobre ese botón (o cuando finalicen los 3 días), se abrirá la ventana de **Activación de Licencia**:
  1. Verás tu **ID de Máquina único** (formato `MCH-XXXX-XXXX-XXXX`). Puedes pulsar **📋 Copiar ID** para enviarlo al administrador o usarlo en tu generador de licencias.
  2. Ingresa tu código de licencia (`CS10-XXXX-XXXX-XXXX-XXXX`) en el campo de texto.
  3. Pulsa **🔓 ACTIVAR LICENCIA**. El botón superior cambiará a **`✅ LICENCIA PRO`** en color verde esmeralda y el plugin quedará activado permanentemente en tu equipo.

---

## 3. Vista General de la Interfaz (Responsive)

La interfaz de **ClipSampler v2.5** cuenta con un diseño de estudio **Royal Sapphire & Satin Titanium** (azul zafiro profundo con iluminación ambiental) y es **100% adaptable (Responsive)**:
- **Redimensionamiento libre:** Puedes arrastrar la esquina inferior derecha del plugin para achicarlo (`680x540`) en pantallas portátiles o expandirlo (`1800x1300`) en monitores de estudio 4K.
- **Tipografía inteligente:** Todos los botones, etiquetas y encabezados escalan automáticamente su tamaño de letra para que ningún texto se corte ni se superponga.

---

## 4. Paso 1: Modelo Sampler Local (Arrastrar WAV/MP3 o Cargar Archivo)

**ClipSampler v2.5** funciona de forma nativa e inmediata como un **Sampler de Transformación Local**:

1. **Arrastrar y Soltar desde el DAW o Windows (Drag & Drop Directo):**
   - Arrastra cualquier archivo de audio en formato **WAV, MP3, AIFF, FLAC u OGG** directamente desde el explorador de Windows o desde el browser de muestras de tu DAW (FL Studio, Ableton, Reaper, etc.) y suéltalo sobre la ventana del plugin o sobre cualquiera de los visores de onda.
   - El visor se iluminará con un halo cian reactivo al pasar el archivo por encima y se cargará en memoria RAM instantáneamente.
2. **Botón `📂 Cargar Audio Local`:**
   - Haz clic en este botón (ubicado a la izquierda en la Fila 0) para abrir el explorador de archivos nativo y elegir cualquier sample o grabación de tu librería.
3. **Procesamiento Inmediato:**
   - Al cargar el archivo local, ClipSampler calcula automáticamente su **BPM**, detecta su **Tonalidad Musical (Key)**, ajusta la frecuencia de muestreo al proyecto de tu DAW y genera las rebanadas automáticas para tocar en pads MIDI (**C1 a G2**).

---

## 5. Paso 2: Explorador Integrado de Freesound (API v2)

**ClipSampler v2.6** integra un navegador modal completo conectado directamente a la API oficial v2 de **Freesound.org** para buscar, filtrar, preescuchar y cargar sonidos libres de derechos directamente a tus pads.

### 🔑 Activación con tu API Key Gratuita
1. Haz clic en el botón naranja **`🎵 Freesound`** ubicado en la barra de encabezado superior.
2. Si es la primera vez que lo abres, verás una ventana modal con el cuadro central: **"Pega tu API Key de Freesound aquí"**.
3. Haz clic en el enlace **`🌐 ¿Cómo obtener una API Key gratuita? (freesound.org)`** o visita directamente [freesound.org/apiv2/apply](https://freesound.org/apiv2/apply) para registrarte y generar una clave de desarrollador gratuita al instante.
4. Pulsa el botón **`📋 Pegar`** y haz clic en **`🔓 Conectar y Cargar Buscador`**.
5. Tu API Key se guardará de forma segura y persistente en `%APPDATA%\ClipSampler\freesound_api_key.txt`, por lo que no necesitarás volver a introducirla en futuras sesiones. Si deseas actualizarla o cambiarla más adelante, dispones del botón **`🔑 Cambiar API Key`**.

### 🔍 Buscador, Filtros y Descriptores de Audio Avanzados
Una vez conectada la API Key, se desplegará el panel del buscador conectado al endpoint canónico `/apiv2/search/`:
- **Caja de Búsqueda:** Escribe cualquier término (ej. *snare, kick, vinyl scratch, synth pad, vocal chop, hi-hat, ambient rain*) y pulsa Enter o haz clic en **`🔍 Buscar Sonidos`**.
- **Filtro de Duración:**
  - *Cualquiera:* Sin límite de duración.
  - *One-Shots (0 a 2s):* Ideal para bombos, cajas, platillos y golpes percusivos.
  - *Loops Cortos (2 a 6s):* Para compases rítmicos o fills.
  - *Frases (6 a 15s):* Riffs melódicos, líneas vocales y secuencias.
  - *Muestras Largas (15s a 5m):* Texturas ambientales, grabaciones de campo y pistas completas.
- **Filtro de Ordenación:**
  - *Más Relevantes (Score):* Coincidencia semántica con tu búsqueda.
  - *Más Descargados:* Las muestras más populares y probadas de la comunidad.
  - *Mejor Calificados:* Los sonidos con mayor puntuación de usuarios.
  - *Más Recientes:* Novedades y samples recién subidos.
  - *Más Cortos:* Orden ascendente por longitud de muestra.
- **Filtro de Licencia:**
  - *Todas las Licencias:* Acceso a todo el catálogo.
  - *Creative Commons 0 (Dominio Público):* 100% libre para cualquier uso comercial o privado sin necesidad de atribución.
  - *Creative Commons Attribution (CC-BY):* Libre para producciones dando crédito al autor original.
- **Filtro de Formato de Audio Original:**
  - *Cualquier Formato:* Muestra todo tipo de archivos.
  - *Solo WAV:* Filtra exclusivamente grabaciones subidas en formato WAV sin compresión.
  - *Solo FLAC:* Muestras en compresión sin pérdida (Lossless).
  - *WAV o FLAC:* Lo mejor de ambos formatos de estudio.
  - *Solo AIFF:* Grabaciones en estándar Apple AIFF.

### 🎧 Preescucha, Sonidos Similares, Paginación y Carga en Pads
- **Insignias y Descriptores en la Lista:** Cada elemento muestra su nombre, autor, licencia, tipo de canal (Stereo/Mono), frecuencia de muestreo (kHz) y etiquetas (#tags). Si Freesound detectó el tempo de la muestra, se resalta la insignia de tempo (ej. `[124 BPM]`).
- **`▶ Escuchar` / `⏹ Detener`:** Audición inmediata de la muestra con control de transporte para escuchar antes de cargar.
- **`🔗 Similares` (Descubrimiento Acústico Inteligente):** Al pulsar este botón en cualquier sonido, ClipSampler consulta el endpoint `/apiv2/sounds/<id>/similar/` y te devuelve al instante una lista de samples acústicamente parecidos analizados espectralmente por Freesound. Puedes regresar a tu búsqueda original pulsando **`✕ Volver a la Búsqueda`**.
- **Barra de Paginación (`◀ Anterior` / `Siguiente ▶`):** Navega cómodamente entre páginas viendo el número total de sonidos encontrados (ej. *Página 1 de 24 (480 sonidos encontrados)*).
- **`⚡ Cargar en Pad`:** Al pulsar este botón, ClipSampler descarga la muestra en alta fidelidad a la caché local, la carga automáticamente en el **Visor 1**, calcula su **BPM y Tono**, activa el **Rebanador MPC (C1 a G2)** y deja el sample listo para tocar, aplicar efectos o arrastrar a tu DAW.
- Pulsa **`✕ Volver a ClipSampler`** en cualquier momento para regresar a la vista principal.

---

## 6. Paso 3: Función Opcional 'Uso Libre' (Lectura de URLs Web)

Para habilitar la captura opcional desde plataformas de internet:

1. **Botón `⚖️ Uso Libre (Web)`:**
   - Por defecto, las cajas de texto y botones de extracción web permanecen desactivados para garantizar la operación local.
   - Haz clic en **`⚖️ Uso Libre (Web)`**. Aparecerá una ventana modal con el **Aviso Legal y Descargo de Responsabilidad**.
   - Al pulsar **`Aceptar y Activar`**, el botón cambiará a **`✅ Uso Libre (Web Activo)`** en color verde esmeralda, desbloqueando el campo de URL y los selectores de **YouTube, TikTok e Instagram**.
2. **Extracción y Recorte (Segundos o Compases):**
   - **Pegar Enlace:** Pega la URL del video. El selector de plataforma se auto-configurará.
   - **`⏱️ Segundos` / `🎵 Compases`:** Elige si deseas recortar por tiempo fijo (`3s`, `10s`, `30s`) o por compases musicales exactos (`1 Bar`, `2 Bars`, `4 Bars`) sincronizados al tempo de tu DAW.
   - **`⚡ EXTRAER`:** Inicia el recorte en alta resolución sincronizado a la tasa de muestreo del DAW.
   - **`🔄 Nueva Extracción`:** Se habilita inmediatamente tras el primer audio para limpiar ambos visores y permitir una nueva sesión con un solo clic.

---

## 7. Paso 4: Doble Visor de Onda, Coloración Analógica y Suite de Derechos IA

**ClipSampler v2.6** cuenta con una suite de herramientas avanzadas de **Modificación y Coloración de Audio** ubicada en la Fila 4:

### 📼 Botón `📼 Calor Analógico`
- **¿Qué hace?** Aplica una coloración analógica vintage inspirada en grabadoras de cinta de carrete abierto (*tape machines*) y previos a válvulas:
  - Genera saturación armónica asimétrica suave (2º y 3º armónicos musicales) que añade grosor y pegada.
  - Realza sutilmente el cuerpo en graves (~160 Hz estilo Pultec EQ) y suaviza los agudos extremos (> 15.5 kHz) eliminando la aspereza digital de videos de internet.
- El resultado se carga automáticamente en el **Visor 2 (`VISOR 2: CALOR ANALÓGICO (TAPE WARMTH)`)**.
- **Usos creativos:** Dar calidez, cohesión y pegada a samples fríos o delgados sin distorsión áspera.

### 🤖 Botones `▲ Alterar +3 Tonos Más` y `▼ Alterar -3 Tonos Menos`
- **Control Directo y sin Confusiones:** Ahora cuentas con dos botones dedicados e independientes:
  - **`▲ Alterar +3 Tonos Más`:** Sube el tono exactamente +3 semitonos musicales.
  - **`▼ Alterar -3 Tonos Menos`:** Baja el tono exactamente -3 semitonos musicales.
- **Procesamiento DSP Anti-Copyright:** Ambos botones aplican interpolación de Lagrange, preservación de formantes y dispersión de fase all-pass para transformar la huella acústica y evadir la detección de Content ID sin arruinar la musicalidad del sample.
- El resultado se carga en el **Visor 2 (`VISOR 2: AUDIO MODIFICADO (DERECHOS IA)`)**.

### 🛡️ Botón `🛡️ Verificar Derechos`
- Analiza matemáticamente la correlación espectral y la divergencia entre el Audio 1 y el Audio 2:
  - Si estás en el **Visor 1**, evalúa el riesgo de huella acústica del audio original.
  - Si estás en el **Visor 2**, confirma la divergencia matemática confirmando que el audio procesado es apto para uso seguro (`✅ LIBRE DE DERECHOS`).

---

## 8. Paso 5: Modo Auto-Chop (Estilo MPC) y Rack de Efectos Macro

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
9. **Botón `⚡ Aplicar FX / ✓ Efectos Activados`:**
   - Imprime de forma permanente e instantánea los ajustes de efectos (Lo-Fi, Tono, Filtro) en el archivo WAV del Audio seleccionado.
   - **Indicador Dinámico de Estado:** Al hacer clic, el botón cambia inmediatamente a **`✓ Efectos Activados`** en color verde neón brillante, confirmando con total claridad que el audio tiene los efectos aplicados y activos. Si vuelves a mover las perillas para un nuevo ajuste, el botón volverá a mostrar `Aplicar FX` para que puedas volver a imprimir la nueva versión.

---

## 9. Paso 6: Reverse y Half-Time por Pad, Ajuste de Transientes y Exportación

En los **Visores de Onda** y en la **Fila 5 (Barra Inferior de Transporte, Controles de Pad y Exportación)**:

- **Modificadores Individuales por Pad / Rodaja (Reverse y Half-Time):**
  - **`◀ Rev` (Invertir Dirección de Audio):**
    - Reproduce el corte seleccionado en reversa en tiempo real con cero latencia y desvanecimiento anti-chasquidos.
    - Se activa haciendo clic en el botón **`◀ Rev`** de la barra inferior o haciendo **clic derecho** sobre cualquier rodaja en la onda y seleccionando **`◀ Invertir (Reverse)`**.
    - El pad reflejará el símbolo **`◀`** en su etiqueta (por ejemplo: `✂C1 ◀`).
  - **`⏳ 1/2` (Half-Time / Mitad de Velocidad):**
    - Reproduce el corte al 50% de velocidad (doble de duración) con interpolación suave cúbica tipo cinta vintage (*tape half-speed*).
    - Se activa con el botón **`⏳ 1/2`** o haciendo **clic derecho** sobre la rodaja y seleccionando **`⏳ Media Velocidad (Half-Time 0.5x)`**.
    - El pad reflejará el símbolo **`½`** en su etiqueta (por ejemplo: `✂C1 ½` o combinado `✂C1 ◀ ½`).
  - **Exportación Fiel al DAW:** Al arrastrar la rodaja con Reverse o Half-Time al DAW mediante el botón **`Arrastrar .WAV`** o **`✂ Corte Exacto`**, ClipSampler renderiza automáticamente el audio invertido y/o estirado a media velocidad con calidad PCM de 24-bit.
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
- **Botón `Arrastrar .WAV`:** Mantén presionado el clic izquierdo sobre este botón (o directamente sobre cualquiera de los dos visores de onda) y arrástralo hacia una pista de audio o sampler de tu DAW. Si tienes un corte exacto seleccionado (ej. `C1`), el botón mostrará `Arrastrar C1 (.WAV)` y arrastrará únicamente ese trozo con sus modificadores de Reverse/Half-time aplicados.
- **Botón `🎹 Convertir a MIDI`:** Haz clic o mantén presionado el clic izquierdo sobre este botón y arrástralo hacia una pista MIDI o al Piano Roll de tu DAW. **ClipSampler** analiza el audio, transcribe automáticamente sus notas melódicas y transientes, y genera un archivo `.mid` listo para ser interpretado por cualquier sintetizador o instrumento virtual (Serum, Omnisphere, FL Keys, etc.).

---

## 10. Botón Nueva Extracción y Mantenimiento

En la esquina superior derecha del encabezado encontrarás dos botones de mantenimiento rápido:
- **`🔄 Actualizar Motor`:** Ejecuta `yt-dlp -U` en segundo plano para actualizar el motor de descarga cuando YouTube, TikTok o Instagram cambien sus algoritmos.
- **`🗑️ Limpiar Caché`:** Elimina de forma segura los archivos `.wav` y `.mid` antiguos guardados en la carpeta temporal (`%APPDATA%\ClipSampler\AudioCache`) sin borrar las muestras que tienes cargadas actualmente en pantalla.

---

## 11. Administración del Generador de Licencias (Para el Propietario)

Si eres el administrador del software y deseas generar licencias para tus clientes:

1. Entra a la carpeta [`license_system`](file:///c:/Users/joncl/Downloads/Plugin/license_system) y haz doble clic en **`Iniciar_Generador_Licencias.bat`**.
2. Se abrirá automáticamente tu navegador en `http://127.0.0.1:8765` con el **Panel de Licencias y Base de Datos SQLite (`licenses.db`)**.
3. Puedes elegir entre dos modos:
   - **🔒 Atada a ID de Máquina (Offline/Online):** Pegas el ID de Máquina del cliente (`MCH-XXXX-XXXX-XXXX`) y generas un código que solo funcionará matemáticamente en esa computadora.
   - **🌐 Auto-Vinculable Web:** Generas un código `CS10-AUTO-XXXX-XXXX-XXXX` que se vincula automáticamente a la primera computadora que lo active por internet y bloquea cualquier otra PC.
4. Desde el mismo panel puedes **buscar clientes, exportar a CSV, liberar una PC o revocar licencias** en tiempo real.

---

## 12. Solución de Problemas Frecuentes

- **El texto o los botones se ven pequeños en mi monitor 4K:**
  Arrastra el triángulo de la esquina inferior derecha del plugin para ampliar la ventana al tamaño que prefieras; toda la interfaz y las fuentes crecerán proporcionalmente.
- **Un enlace de TikTok o Instagram da error de extracción:**
  Asegúrate de que el video o Reel sea **público** (no de una cuenta privada) y pulsa el botón **`🔄 Actualizar Motor`** en la barra superior para tener la última versión del extractor.
- **No suena al presionar teclas en mi controlador MIDI:**
  Verifica si tienes activado el **`🥁 Modo Auto-Chop (MPC)`**. En modo Auto-Chop las rebanadas se disparan desde la octava **`C1` a `D#2`** (notas MIDI 36 a 51). Si deseas tocar en todo el teclado, desactiva la casilla Auto-Chop.

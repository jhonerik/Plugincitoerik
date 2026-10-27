# 🎹 Manual de Usuario — ClipSampler v2.5

Bienvenido al **Manual Oficial de Usuario de ClipSampler v2.5**, el instrumento virtual (**VST3 / AAX**) diseñado para productores musicales, beatmakers y diseñadores de sonido que desean capturar fragmentos de audio desde **YouTube, TikTok e Instagram**, transformarlos creativamente y convertirlos en samples listos para tocar en MIDI o arrastrar directamente al DAW.

---

## 📑 Índice

1. [Instalación en tu DAW](#1-instalación-en-tu-daw)
2. [Período de Prueba (3 Días) y Activación de Licencia](#2-período-de-prueba-3-días-y-activación-de-licencia)
3. [Vista General de la Interfaz (Responsive)](#3-vista-general-de-la-interfaz-responsive)
4. [Paso 1: Seleccionar Plataforma y Analizar Enlace](#4-paso-1-seleccionar-plataforma-y-analizar-enlace)
5. [Paso 2: Modo de Extracción (Segundos vs Compases) y Captura](#5-paso-2-modo-de-extracción-segundos-vs-compases-y-captura)
6. [Paso 3: Doble Visor de Onda, Coloración Analógica, Aire 3D y Derechos IA](#6-paso-3-doble-visor-de-onda-coloración-analógica-aire-3d-y-derechos-ia)
7. [Paso 4: Modo Auto-Chop (Estilo MPC) y Rack de Efectos Macro](#7-paso-4-modo-auto-chop-estilo-mpc-y-rack-de-efectos-macro)
8. [Paso 5: Reverse y Half-Time por Pad, Ajuste de Transientes y Exportación](#8-paso-5-reverse-y-half-time-por-pad-ajuste-de-transientes-y-exportación)
9. [Botón Nueva Extracción y Mantenimiento](#9-botón-nueva-extracción-y-mantenimiento)
10. [Administración del Generador de Licencias (Para el Propietario)](#10-administración-del-generador-de-licencias-para-el-propietario)
11. [Solución de Problemas Frecuentes](#11-solución-de-problemas-frecuentes)

---

## 1. Instalación en tu DAW

1. Ejecuta el instalador **`ClipSampler_Windows_Installer_v2.5.0.exe`** (o `v2.4.0.exe`) como Administrador.
2. Acepta el acuerdo de licencia y selecciona los formatos que utilices en tu estudio:
   - **VST3 (64-bit):** Compatible con FL Studio, Ableton Live, Cubase, Studio One, Reaper, Bitwig, etc. (Se instala en `C:\Program Files\Common Files\VST3\ClipSampler.vst3`).
   - **AAX (64-bit):** Compatible con Avid Pro Tools.
3. El instalador copiará automáticamente las herramientas de extracción (`yt-dlp.exe` y `ffmpeg.exe`) en `C:\ProgramData\ClipSampler\tools` para que el plugin funcione de inmediato sin configuraciones adicionales.
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

## 5. Paso 2: Modo de Extracción (Segundos vs Compases) y Captura

En la **Fila 2 y Fila 3 (Tiempo, Duración y Captura)**:

1. **Inicio (`MM:SS` o segundos):** Puedes escribir el minuto y segundo exacto donde quieres que empiece la extracción (ej. `01:15` o `75`) o mover la **barra deslizadora de tiempo** que aparece al lado.
2. **Selector de Modo (`⏱️ Segundos` / `🎵 Compases`):**
   - **Modo Segundos (`⏱️ Segundos`):** Muestra los botones clásicos de tiempo fijo:
     - **`3s`:** Ideal para one-shots, golpes de batería o efectos vocales.
     - **`10s`:** Frases melódicas y acordes estándar.
     - **`30s`:** Secciones largas, intros completas para chopear.
   - **Modo Compases Musicales (`🎵 Compases`):** ¡Exclusivo de ClipSampler v2.5! Sincronizado automáticamente con el tempo (BPM) del DAW o el BPM detectado:
     - **`1 Bar`:** Recorta exactamente 1 compás (4 tiempos). *Ejemplo: 2.0s a 120 BPM, 1.7s a 140 BPM*.
     - **`2 Bars`:** Recorta exactamente 2 compases (8 tiempos). *Ejemplo: 4.0s a 120 BPM, 3.4s a 140 BPM*.
     - **`4 Bars`:** Recorta una progresión completa de 4 compases (16 tiempos) en bucle perfecto.
3. **Casilla `Norm -1dB`:** Si está activada (recomendado), nivela el volumen del audio extraído a `-1.0 dB` y aplica micro-fundidos de 10 ms al inicio y final para evitar chasquidos (*clicks*).
4. **Botón `⚡ EXTRAER`:** Inicia la descarga y recorte en alta fidelidad sincronizado a los Hz de tu proyecto.
5. **Botón `🔄 Nueva Extracción`:** Permite limpiar la sesión con un solo clic para comenzar una nueva captura desde cero.

---

## 6. Paso 3: Doble Visor de Onda, Coloración Analógica, Aire 3D y Derechos IA

**ClipSampler v2.4** cuenta con una suite de herramientas avanzadas de **Modificación y Coloración de Audio** ubicada en la Fila 4:

### 📼 Botón `📼 Calor Analógico`
- **¿Qué hace?** Aplica una coloración analógica vintage inspirada en grabadoras de cinta de carrete abierto (*tape machines*) y previos a válvulas:
  - Genera saturación armónica asimétrica suave (2º y 3º armónicos musicales) que añade grosor y pegada.
  - Realza sutilmente el cuerpo en graves (~160 Hz estilo Pultec EQ) y suaviza los agudos extremos (> 15.5 kHz) eliminando la aspereza digital de videos de internet.
- El resultado se carga automáticamente en el **Visor 2 (`VISOR 2: CALOR ANALÓGICO (TAPE WARMTH)`)**.
- **Usos creativos:** Dar calidez, cohesión y pegada a samples fríos o delgados sin distorsión áspera.

### ✨ Botón `✨ Aire y Espacio 3D`
- **¿Qué hace?** Abre el campo estéreo dimensional y aporta brillo cristalino sedoso:
  - Separa la señal en componentes *Mid/Side* (centro y laterales). El centro monofónico se mantiene 100% puro para preservar total compatibilidad mono sin desfases.
  - Expande la imagen espacial estéreo (+2.2 dB) con micro-decorrelación de fase envolvente.
  - Aplica un excitador armónico en la banda de aire (*Air Band* > 10.5 kHz) aportando brillo moderno y presencia.
- El resultado se carga en el **Visor 2 (`VISOR 2: AIRE Y ESPACIO 3D`)**.
- **Usos creativos:** Abrir sintetizadores, guitarras, pianos y bucles para que tengan profundidad 3D y brillo profesional en la mezcla.

### 🤖 Botón `🤖 Derechos IA (±3 st)`
- Aplica procesamiento DSP transformativo (desplazamiento de tono de ±3 semitonos con preservación de formantes, micro-estiramiento temporal y dispersión de fase all-pass) para alterar la huella acústica de la muestra original y evadir sistemas de Content ID.
- El resultado se carga en el **Visor 2 (`VISOR 2: AUDIO MODIFICADO (DERECHOS ±3 ST IA)`)**.

### 🛡️ Botón `🛡️ Verificar Derechos`
- Analiza matemáticamente la correlación espectral y la divergencia entre el Audio 1 y el Audio 2:
  - Si estás en el **Visor 1**, evalúa el riesgo de huella acústica del audio original.
  - Si estás en el **Visor 2**, confirma la divergencia matemática confirmando que el audio procesado es apto para uso seguro (`✅ LIBRE DE DERECHOS`).

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
9. **Botón `⚡ Aplicar FX / ✓ Efectos Activados`:**
   - Imprime de forma permanente e instantánea los ajustes de efectos (Lo-Fi, Tono, Filtro) en el archivo WAV del Audio seleccionado.
   - **Indicador Dinámico de Estado:** Al hacer clic, el botón cambia inmediatamente a **`✓ Efectos Activados`** en color verde neón brillante, confirmando con total claridad que el audio tiene los efectos aplicados y activos. Si vuelves a mover las perillas para un nuevo ajuste, el botón volverá a mostrar `Aplicar FX` para que puedas volver a imprimir la nueva versión.

---

## 8. Paso 5: Reverse y Half-Time por Pad, Ajuste de Transientes y Exportación

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

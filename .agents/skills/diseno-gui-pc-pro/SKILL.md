---
name: diseno-gui-pc-pro
description: >-
  Mega-Skill de diseño de interfaces gráficas de escritorio de nivel comercial Pro.
  Fusiona desktop-ergonomics-hig (El Arquitecto: jerarquía de paneles, Dark Mode por
  capas de elevación, atajos de teclado y ergonomía de ratón) con juce-custom-lookandfeel
  (El Diseñador Audio/DSP en C++/JUCE: LookAndFeel_V4 personalizado, perillas rotatorias
  con halo, botones con bisel especular, visores de onda a 60 FPS) y python-modern-desktop-ui
  (CustomTkinter y PySide6/Qt con hojas de estilo modernas QSS). Úsala siempre que diseñes,
  mejores o refactorices software de PC o plugins VST3/AU en C++/JUCE o Python.
---

# Mega-Skill: Diseño GUI PC Pro (`diseno-gui-pc-pro`)

Esta Mega-Skill unifica la arquitectura ergonómica de software de escritorio (**El Arquitecto**) con el diseño visual vectorial de alto nivel (**El Diseñador**) para **C++ / JUCE (Audio & DSP)** y **Python (CustomTkinter / PySide6 Qt)**.

```text
┌─────────────────────────────────────────────────────────────────┐
│ 🧠 PILAR 1: desktop-ergonomics-hig (El Arquitecto)              │
│   ├── 📐 Organiza los paneles (Header, Deck, Canvas, Status Bar)│
│   ├── 🎨 Define la paleta Dark Mode por capas y contrastes      │
│   └── ⌨️ Añade atajos de teclado, tooltips y ergonomía de ratón │
├─────────────────────────────────────────────────────────────────┤
│                     ➕ SE FUSIONA CON ➕                         │
├─────────────────────────────────────────────────────────────────┤
│ 🎨 PILAR 2: juce-custom-lookandfeel / python-modern-ui          │
│   ├── 🔘 Dibuja botones, switches y perillas de alto nivel      │
│   ├── ✨ Aplica sombras suaves, bordes redondeados y animaciones │
│   └── ⚡ Asegura que la ventana vaya fluida a 60 FPS            │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🧠 PILAR 1: `desktop-ergonomics-hig` (El Arquitecto Universal)

### 1.1. Sistema de Color Dark Mode por Capas de Elevación (Elevation Palette)
Nunca uses negro puro plano ni grises lavados sin contraste. La profundidad espacial se construye en 4 niveles de luminancia:

| Nivel | Rol Arquitectónico | Color Hex | Borde / Luz Especular |
| :--- | :--- | :--- | :--- |
| **Nivel 0 (Chasis)** | Fondo base de la aplicación | `#070B14` → `#0B111E` | Viñeta perimetral oscura |
| **Nivel 1 (Paneles)** | Contenedores de sección / Tarjetas | `#101829` → `#141F35` | Borde `#1E2D4A` + Luz superior `rgba(255,255,255,0.08)` |
| **Nivel 2 (Pozos/Visores)** | Visores de onda, cajas de texto, listas | `#060A12` | Sombra interior oscura + borde activo `#38BDF8` |
| **Nivel 3 (Controles)** | Botones, perillas, faders, badges | Gradiente activo | Sombra proyectada inferior `rgba(0,0,0,0.45)` |

**Acentos Semánticos de Alto Contraste (Estudio / Pro Audio):**
- **Cian Primario (Foco / Selección / Playhead):** `#38BDF8`
- **Esmeralda / Turquesa (Activo / Licencia Pro / Reproducir):** `#10B981` / `#2DD4BF`
- **Ámbar / Naranja (Calor Analógico / Freesound / MIDI):** `#F59E0B` / `#EA580C`
- **Violeta / Magenta (IA / Extracción / WAV Drag):** `#8B5CF6` / `#EC4899`

### 1.2. Jerarquía Espacial de Paneles (Regla de las 4 Zonas)
1. **Header / Top Toolbar (44–48 px):** Identidad del software con tipografía limpia, indicador de estado en vivo (píldora con LED) y accesos globales rápidos.
2. **Deck de Controles Primarios:** Agrupación lógica con margen interno uniforme (`12–16 px`), separación entre grupos (`8–10 px`) y etiquetas de sección tipo cápsula.
3. **Canvas / Visor Central de Trabajo:** Zona de máximo protagonismo visual (formas de onda, espectro o lienzo de edición) con rejilla técnica sutil (dB / divisiones de tiempo).
4. **Status & Transport Bar Inferior:** Telemetría en tiempo real (BPM, Tono, Estado de proceso) y acciones de salida (Reproducir, Detener, Arrastrar, Exportar).

### 1.3. Ergonomía de Ratón y Teclado (Human Interface Guidelines)
- **Cursores Interactivos:** Todo botón, selector, pestaña o perilla debe activar `PointingHandCursor` al pasar el cursor.
- **Tooltips Informativos:** Incluir `TooltipWindow` o descripciones emergentes breves que indiquen función y atajo de teclado.
- **Atajos de Teclado Estándar:**
  - `Espacio`: Reproducir / Detener vista previa.
  - `Ctrl+O` / `Cmd+O`: Abrir o cargar archivo local.
  - `Ctrl+F` / `Cmd+F`: Abrir buscador / explorador de sonidos.
  - `Ctrl+M` / `Cmd+M`: Convertir / Procesar a MIDI.
  - `Ctrl+E` / `Cmd+E`: Exportar archivo resultante.
  - `Escape`: Cerrar modal activo o detener reproducción.

---

## 🎨 PILAR 2A: Módulo C++ / JUCE (`juce-custom-lookandfeel`)

### 2A.1. Reglas de `juce::LookAndFeel_V4` de Nivel Comercial (Estilo FabFilter / Serum / Output)
1. **Botones Biselados con Iluminación Especular (`drawButtonBackground`):**
   - Dibujar sombra proyectada suave (`1.5px` hacia abajo con alfa `0.38f`).
   - Relleno con `ColourGradient` vertical: tono superior ligeramente iluminado (`brighter(0.10f)`) hacia tono base inferior (`darker(0.12f)`).
   - En estado `shouldDrawButtonAsHighlighted` (Hover): incrementar brillo y añadir halo sutil en el borde.
   - En estado `shouldDrawButtonAsDown` (Click): desplazar contenido `1px` y comprimir el gradiente.
   - Añadir línea de reflejo superior interna (`1px` con `Colours::white.withAlpha(0.16f)`) para efecto de cristal/hardware.

2. **Perillas Rotatorias DSP (`drawRotarySlider`):**
   - **Pista de Fondo (Track Arc):** Arco de `2.4f` a `3.5f` px en color `#1E293B`.
   - **Arco de Valor (Value Arc):** Gradiente luminoso con halo exterior suave (`strokePath` doble: uno ancho con alfa `0.18f` para glow y otro nítido de `3.0f` px al `100%`).
   - **Cuerpo de la Perilla (3D Cap):** Círculo interior con gradiente metálico oscuro (`#1E293B` → `#0F172A`), anillo biselado exterior y línea/punto indicador brillante alineado al ángulo actual.

3. **Sliders Lineales y Cajas de Texto (`drawLinearSlider`, `drawTextEditorOutline`):**
   - Pista empotrada oscura (`#090E1A`), relleno de progreso con brillo cian/esmeralda y cabezal circular con núcleo blanco y halo de color.
   - `TextEditor` y `ComboBox` con fondo `#090E1A`, esquinas redondeadas (`6px`) y borde que se ilumina en `#38BDF8` cuando tiene el foco (`hasKeyboardFocus(true)`).

4. **Visores de Onda a 60 FPS:**
   - Usar `startTimerHz(60)` en el editor o visor para animar el cursor de reproducción (`playhead`) con interpolación suave.
   - Dibujar rejilla de osciloscopio sutil (línea central `0 dB` y divisiones verticales de compás/rodaja) y cabezal de lectura con marcador triangular superior.

---

## 🐍 PILAR 2B: Módulo Python (`python-modern-desktop-ui`)

### 2B.1. Transformación de Interfaces Python (Prohibido Tkinter Clásico Win95)
Cuando el proyecto o herramienta auxiliar esté en Python:
1. **Opción A — `PySide6` / `PyQt6` con QSS Moderno:**
   - Usar `QMainWindow` con layout de 3 paneles (`QSplitter` o `QHBoxLayout`), aplicando hoja de estilos `.qss` centralizada con variables de la paleta Dark Mode por capas (`#070B14`, `#101829`, `#38BDF8`).
   - Botones con `border-radius: 7px; padding: 8px 14px;` y estados `:hover`, `:pressed`, `:disabled`.
2. **Opción B — `CustomTkinter`:**
   - Configurar `ctk.set_appearance_mode("Dark")` y `ctk.UseTheme` personalizado con `corner_radius=8`, `border_width=1` y fuentes `Inter` o `Segoe UI Semibold`.

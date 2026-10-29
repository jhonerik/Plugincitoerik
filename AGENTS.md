# Reglas de Exploración, Rendimiento y Arquitectura del Proyecto (ClipSampler)

## 1. Lectura Estricta y Optimizada del Proyecto (Omitir Carpetas Pesadas)
Cada vez que analices, busques o leas archivos en este repositorio, **SOLO debes leer las carpetas de código fuente necesarias** y **OMITIR ESTRICTAMENTE** las carpetas de instalaciones, submódulos, compilaciones y control de versiones:

### ✅ Carpetas Permitidas para Lectura y Análisis:
- `Source/` (en la rama `windows-v2.6.1`) o `source/` (en las ramas `mac-v2.5` y `main`): Contiene el código C++/JUCE del plugin (`UI/`, `DSP/`, `Services/`, `Security/`).
- `CMakeLists.txt` e `installer_*.iss`: Configuración de compilación e instalador.
- `pagina web/`: Código fuente de la landing page promocional (`index.html`, `styles.css`, `app.js`).
- `.github/workflows/`: Pipelines de CI/CD para compilación en la nube.

### 🚫 Carpetas PROHIBIDAS de Escanear o Indexar:
- `.git/` (objetos, logs e índice interno de Git).
- `JUCE/` (submódulo completo del framework JUCE).
- `build/`, `build-win/`, `build_v261/`, `build_verify/` o cualquier directorio `build*`.
- `modules/` y `cmake/` (módulos externos de terceros como Melatonin Inspector).
- `Release_v*/` e instaladores `.exe` / `.pkg` / `.zip` binarios.

---

## 2. Estándar de Diseño GUI Profesional (`diseno-gui-pc-pro`)
Toda modificación de interfaz gráfica en este proyecto debe aplicar la fusión de:
1. **`desktop-ergonomics-hig` (El Arquitecto)**:
   - Organización clara por paneles jerárquicos (Header Bar, Source/Action Deck, Dual Canvas Area, Transport/Status Bar).
   - Paleta **Dark Mode por Capas de Elevación** (Nivel 0 Chasis Profundo `#070B14`, Nivel 1 Paneles Elevados `#101829` con borde especular superior, Nivel 2 Controles/Visores `#060A12`, Acentos de alto contraste Cian `#38BDF8`, Esmeralda `#10B981`, Ámbar `#F59E0B`, Violeta `#A855F7`).
   - Ergonomía de ratón (`PointingHandCursor` en todos los controles interactivos, `TooltipWindow` activo) y atajos de teclado globales (`Space` Play/Stop, `Ctrl/Cmd+F` Freesound, `Ctrl/Cmd+O` Cargar Audio, `Ctrl/Cmd+M` Audio a MIDI, `Ctrl/Cmd+E` Exportar WAV, `Esc` Cerrar/Detener).
2. **`juce-custom-lookandfeel` / `python-modern-desktop-ui` (El Diseñador)**:
   - Renderizado vectorial en `LookAndFeel_V4` con botones biselados, sombras suaves multicapa, bordes redondeados (`7px`), iluminación de estado hover/active y perillas rotatorias tipo hardware DSP con halo luminoso e indicador de precisión.
   - Fluidez visual a **60 FPS** (`startTimerHz(60)`) sin bloqueos en el hilo de interfaz (`MessageManager`).

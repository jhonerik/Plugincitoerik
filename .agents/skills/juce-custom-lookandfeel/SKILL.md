---
name: juce-custom-lookandfeel
description: >-
  Especialista en diseño visual de plugins de Audio y DSP con C++ y JUCE (LookAndFeel_V4).
  Dibuja botones biselados con reflejo especular, perillas rotatorias tipo hardware con halo
  luminoso, faders modernos, visores de onda con rejilla de osciloscopio y animaciones fluidas
  a 60 FPS. Úsala siempre que edites o mejores la interfaz gráfica de un plugin JUCE VST3/AU.
---

# JUCE Custom LookAndFeel (Audio & DSP Pro UI)

Guía técnica para transformar componentes estándar de JUCE en interfaces comerciales de nivel Pro (estilo FabFilter, Output, Xfer Serum).

## 1. Arquitectura de `LookAndFeel_V4`
- Sobrescribir siempre:
  - `drawButtonBackground` y `drawButtonText`
  - `drawRotarySlider` y `drawLinearSlider`
  - `drawComboBox` y `positionComboBoxText`
  - `fillTextEditorBackground` y `drawTextEditorOutline`
- Evitar colores planos (`g.fillAll(...)` sin matiz): utilizar `juce::ColourGradient` verticales con iluminación especular superior (`1px` blanco al `14–18%` de opacidad) y sombra proyectada inferior (`rgba(0,0,0,0.40)`).

## 2. Perillas Rotatorias (`drawRotarySlider`)
1. **Halo Exterior (Glow):** Trazo del arco activo con grosor `6.0f` y alfa `0.18f`.
2. **Arco de Valor:** Trazo principal de `2.8f` con color vivo (`#38BDF8`, `#10B981`, `#A855F7` o `#F59E0B`).
3. **Capuchón 3D:** Círculo central con gradiente oscuro (`#1E293B` a `#0B1324`), borde metálico (`#334155`) y aguja indicadora iluminada desde el centro hacia el borde.

## 3. Rendimiento a 60 FPS
- Usar `startTimerHz(60)` para actualizar cursores de reproducción (`playhead`) y medidores sin bloquear el hilo de audio.
- Marcar componentes estáticos u opacos cuando corresponda y evitar asignaciones de memoria pesadas dentro de `paint(juce::Graphics& g)`.

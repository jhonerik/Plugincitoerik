---
name: desktop-ergonomics-hig
description: >-
  Arquitecto de ergonomía y Human Interface Guidelines (HIG) para aplicaciones de escritorio
  y plugins de audio. Organiza paneles (Header, Deck, Canvas, Status Bar), define la paleta
  Dark Mode por capas de elevación y contraste, e implementa atajos de teclado, cursores
  interactivos y tooltips. Úsala al estructurar o mejorar cualquier interfaz de escritorio.
---

# Desktop Ergonomics & HIG (El Arquitecto)

Reglas universales de ergonomía, jerarquía visual y usabilidad para software de escritorio profesional en Windows y macOS.

## 1. Organización de Paneles y Jerarquía Visual
- **Header Superior:** Marca + Estado de Licencia/Conexión (Píldora LED) + Acciones Globales.
- **Paneles por Capas (Layered Dark Mode):**
  - **Nivel 0 (Chasis):** `#070B14` (fondo profundo).
  - **Nivel 1 (Secciones/Tarjetas):** `#101829` con borde `#1E2D4A` y línea de luz superior (`rgba(255,255,255,0.07)`).
  - **Nivel 2 (Pozos de Entrada / Visores):** `#060A12` con sombra interior y borde de foco `#38BDF8`.

## 2. Ergonomía de Ratón y Teclado
- **Cursores:** Todo control clicable debe mostrar `PointingHandCursor`.
- **Tooltips:** Proveer pistas claras de uso y atajo de teclado en cada botón o perilla principal.
- **Atajos de Teclado Estándar:**
  - `Espacio`: Reproducir / Detener.
  - `Ctrl/Cmd + O`: Cargar archivo local.
  - `Ctrl/Cmd + F`: Abrir buscador (Freesound).
  - `Ctrl/Cmd + M`: Convertir a MIDI.
  - `Ctrl/Cmd + E`: Exportar WAV.
  - `Escape`: Cerrar ventana superpuesta o detener reproducción.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/PluginProcessor.h"
#include "WaveformComponent.h"
#include "LicenseOverlayComponent.h"

/**
 * @class ScalablePluginLookAndFeel
 * @brief Sistema de diseño visual adaptable (Responsive) con estética de Estudio Zafiro y Titanio Satinado.
 *
 * Características visuales y de adaptabilidad:
 * - Botones con gradiente metálico 3D, bisel superior especular y escalado automático de texto
 *   (drawButtonText) para que ninguna palabra se recorte al achicar la ventana.
 * - Tarjetas de sección en Azul Zafiro Satinado con cabeceras tipo cápsula que ajustan su fuente
 *   automáticamente al ancho disponible.
 * - Perillas rotatorias 3D (Lo-Fi, Tono, Filtro) con corona de marcas radiales y arco luminoso.
 * - Deslizador lineal de tiempo (MM:SS) con pista iluminada y cursor de doble anillo.
 */
class ScalablePluginLookAndFeel : public juce::LookAndFeel_V4
{
public:
    /** Ajusta el factor de escala global según el tamaño actual de la ventana. */
    void setUiScale(float scale) { uiScale = juce::jlimit(0.75f, 1.85f, scale); }

    /** Cambia el color de acento dinámico según la plataforma activa (YouTube, TikTok o Instagram). */
    void setThemeAccent(juce::Colour accent) { themeAccent = accent; }

    /** Calcula la fuente base para los botones de texto. */
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        float targetSize = juce::jlimit(10.0f, 18.0f, buttonHeight * 0.42f);
        return juce::Font(targetSize, juce::Font::bold);
    }

    /**
     * Dibuja el texto de todos los botones de forma 100% adaptable (Responsive):
     * mide el ancho real del texto y reduce automáticamente el tamaño de la fuente si el botón es estrecho,
     * garantizando que jamás se corte ni una sola letra.
     */
    void drawButtonText(juce::Graphics& g,
                        juce::TextButton& button,
                        bool /*shouldDrawButtonAsHighlighted*/,
                        bool /*shouldDrawButtonAsDown*/) override
    {
        const auto text = button.getButtonText();
        if (text.isEmpty())
            return;

        auto textBounds = button.getLocalBounds().reduced(6, 2);
        if (textBounds.getWidth() <= 4 || textBounds.getHeight() <= 4)
            return;

        float fontSize = juce::jlimit(9.5f, 16.5f, (float)button.getHeight() * 0.42f);
        juce::Font font(fontSize, juce::Font::bold);

        const float stringW = font.getStringWidthFloat(text);
        if (stringW > (float)textBounds.getWidth() && stringW > 0.0f)
        {
            const float ratio = (float)textBounds.getWidth() / stringW;
            font = juce::Font(juce::jmax(8.5f, fontSize * ratio * 0.95f), juce::Font::bold);
        }

        g.setFont(font);
        const auto textCol = button.findColour(button.getToggleState()
                                                   ? juce::TextButton::textColourOnId
                                                   : juce::TextButton::textColourOffId);
        g.setColour(textCol.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.50f));
        g.drawFittedText(text, textBounds, juce::Justification::centred, 1, 0.68f);
    }

    /** Dibuja el fondo metálico satinado con iluminación superior de cada botón. */
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        const float cornerSize = 6.5f;

        if (!button.isEnabled())
        {
            g.setColour(juce::Colour(0xff1e2a44).withAlpha(0.65f));
            g.fillRoundedRectangle(bounds, cornerSize);
            g.setColour(juce::Colour(0xff33476b));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
            return;
        }

        juce::Colour base = backgroundColour;
        if (shouldDrawButtonAsDown)
            base = base.brighter(0.25f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter(0.14f);

        juce::Colour topCol = base.brighter(0.18f);
        juce::Colour botCol = base.darker(0.24f);

        juce::ColourGradient grad(topCol, bounds.getX(), bounds.getY(),
                                  botCol, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Bisel superior perlado y borde luminoso al pasar el cursor
        juce::Colour borderCol = shouldDrawButtonAsHighlighted
            ? base.brighter(0.60f)
            : base.brighter(0.26f).withAlpha(0.80f);
        g.setColour(borderCol);
        g.drawRoundedRectangle(bounds, cornerSize, shouldDrawButtonAsHighlighted ? 1.6f : 1.1f);
    }

    /** Dibuja las tarjetas de sección en Azul Zafiro Satinado con cabecera adaptable. */
    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                   const juce::String& text, const juce::Justification&,
                                   juce::GroupComponent&) override
    {
        auto bounds = juce::Rectangle<float>(1.0f, 4.0f, (float)width - 2.0f, (float)height - 5.0f);
        const float corner = 10.0f;

        // Superficie elegante Azul Zafiro / Titanio Satinado
        juce::ColourGradient cardGrad(juce::Colour(0xee22314f), bounds.getX(), bounds.getY(),
                                      juce::Colour(0xee18243c), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(cardGrad);
        g.fillRoundedRectangle(bounds, corner);

        // Bisel interior perlado sutil
        g.setColour(juce::Colour(0x18ffffff));
        g.drawRoundedRectangle(bounds.reduced(1.2f), corner, 1.0f);

        // Borde exterior con el color de acento de la plataforma activa
        g.setColour(themeAccent.withAlpha(0.42f));
        g.drawRoundedRectangle(bounds, corner, 1.3f);

        // Cápsula superior de título adaptable (reduce la fuente si la ventana es pequeña)
        if (text.isNotEmpty())
        {
            const float maxPillW = juce::jmax(100.0f, (float)width - 28.0f);
            float fontSize = juce::jlimit(9.5f, 13.0f, 11.5f * uiScale);
            juce::Font f(fontSize, juce::Font::bold);
            float reqW = f.getStringWidthFloat(text) + 24.0f;
            if (reqW > maxPillW && reqW > 0.0f)
            {
                fontSize = juce::jmax(8.5f, fontSize * (maxPillW / reqW) * 0.96f);
                f = juce::Font(fontSize, juce::Font::bold);
                reqW = maxPillW;
            }

            const float pillH = juce::jlimit(17.0f, 23.0f, 19.5f * uiScale);
            juce::Rectangle<float> pill(14.0f, 0.0f, juce::jmin(maxPillW, reqW), pillH);

            g.setColour(juce::Colour(0xff293b5f));
            g.fillRoundedRectangle(pill, 5.5f);
            g.setColour(themeAccent.withAlpha(0.75f));
            g.drawRoundedRectangle(pill, 5.5f, 1.1f);

            g.setFont(f);
            g.setColour(juce::Colour(0xfff8fafc));
            g.drawFittedText(text, pill.reduced(8.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.70f);
        }
    }

    /** Dibuja la barra deslizadora horizontal de tiempo de inicio (MM:SS). */
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        auto trackBounds = juce::Rectangle<float>((float)x, (float)y + (float)height * 0.5f - 4.0f, (float)width, 8.0f);
        g.setColour(juce::Colour(0xff141e33));
        g.fillRoundedRectangle(trackBounds, 4.0f);
        g.setColour(juce::Colour(0xff384f78));
        g.drawRoundedRectangle(trackBounds, 4.0f, 1.0f);

        float fillW = juce::jlimit(0.0f, (float)width, sliderPos - (float)x);
        if (fillW > 2.0f)
        {
            auto fillRect = trackBounds.withWidth(fillW);
            juce::Colour trackCol = slider.findColour(juce::Slider::trackColourId);
            juce::ColourGradient fillGrad(trackCol.darker(0.15f), fillRect.getX(), fillRect.getY(),
                                          trackCol.brighter(0.25f), fillRect.getRight(), fillRect.getY(), false);
            g.setGradientFill(fillGrad);
            g.fillRoundedRectangle(fillRect, 4.0f);
        }

        // Cursor luminoso de doble anillo
        float thumbX = juce::jlimit((float)x + 6.0f, (float)(x + width) - 6.0f, sliderPos);
        float thumbY = (float)y + (float)height * 0.5f;
        juce::Colour thumbCol = slider.findColour(juce::Slider::thumbColourId);

        g.setColour(thumbCol.withAlpha(0.35f));
        g.fillEllipse(thumbX - 9.5f, thumbY - 9.5f, 19.0f, 19.0f);

        g.setColour(thumbCol);
        g.fillEllipse(thumbX - 6.0f, thumbY - 6.0f, 12.0f, 12.0f);

        g.setColour(juce::Colours::white);
        g.fillEllipse(thumbX - 2.5f, thumbY - 2.5f, 5.0f, 5.0f);
    }

    /** Dibuja las perillas rotatorias 3D (Lo-Fi, Tono, Filtro) con marcas radiales de estudio. */
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        if (radius <= 4.0f)
            return;

        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Marcas radiales exteriores
        g.setColour(juce::Colour(0xff526b96));
        for (int i = 0; i <= 8; ++i)
        {
            float tickAngle = rotaryStartAngle + (i / 8.0f) * (rotaryEndAngle - rotaryStartAngle);
            float tx1 = centreX + (radius + 1.5f) * std::sin(tickAngle);
            float ty1 = centreY - (radius + 1.5f) * std::cos(tickAngle);
            float tx2 = centreX + (radius + 3.2f) * std::sin(tickAngle);
            float ty2 = centreY - (radius + 3.2f) * std::cos(tickAngle);
            g.drawLine(tx1, ty1, tx2, ty2, 1.2f);
        }

        // Cuerpo 3D de titanio azulado
        juce::ColourGradient knobGrad(juce::Colour(0xff32456b), rx, ry,
                                      juce::Colour(0xff19243b), rx, ry + rw, false);
        g.setGradientFill(knobGrad);
        g.fillEllipse(rx, ry, rw, rw);

        // Arco base
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff2c3e61));
        g.strokePath(bgArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arco de valor iluminado
        juce::Colour arcCol = slider.findColour(juce::Slider::rotarySliderFillColourId);
        juce::Path valArc;
        valArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(arcCol);
        g.strokePath(valArc, juce::PathStrokeType(3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Aguja indicadora blanca
        juce::Path p;
        auto pointerLength = radius * 0.68f;
        auto pointerThickness = 2.3f;
        p.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 1.5f, pointerThickness, pointerLength, 1.0f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(p);
    }

private:
    float uiScale = 1.0f;
    juce::Colour themeAccent { 0xff38bdf8 };
};

/**
 * @class ClipSamplerAudioProcessorEditor
 * @brief Interfaz Gráfica Principal 100% Adaptable (Responsive) y en Español de ClipSampler v2.4.
 *
 * Características principales:
 * - Estética elegante en Azul Zafiro Real y Titanio Satinado con iluminación ambiental e indicadores dinámicos.
 * - Diseño 100% responsivo desde tamaños compactos (680x540) hasta pantallas grandes (1800x1300) sin solapar textos.
 * - Selector multi-plataforma (YouTube, TikTok, Instagram) con auto-detección al pegar enlaces.
 * - Extracción en 3s, 10s o 30s + botón condicional "Nueva Extracción" al completar los 2 audios.
 * - Suite de 3 botones de Derechos de Autor e IA (±3 semitonos + dispersión de fase).
 * - Doble visor de onda con Rebanador MPC (C1-G2), perillas de efectos (Lo-Fi, Tono, Filtro, Sinc. BPM)
 *   y botones para arrastrar archivos .WAV y .MIDI directamente al DAW.
 */
class ClipSamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::DragAndDropContainer,
                                        private juce::Timer
{
public:
    explicit ClipSamplerAudioProcessorEditor(ClipSamplerAudioProcessor&);
    ~ClipSamplerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;

    // Manejadores de eventos de usuario
    void onSelectPlatform(SourcePlatform platform, bool updateStatusMsg = true);
    void onPasteClicked();
    void onDetectDurationClicked();
    void onExtractDurationClicked(int durationSec);
    void onNewExtractionClicked();
    void onTapeWarmthClicked();
    void onSpatialAirClicked();
    void onCheckCopyrightOriginalClicked();
    void onApplyDerechosClicked();
    void onVerifyDerechosFreeClicked();
    void onSelectFxTargetSlot(int slotIndex);
    void onToggleAutoChopClicked();
    void onToggleAutoSyncClicked();
    void onBakeFxClicked();
    void onPlayClicked();
    void onStopClicked();
    void onExactCutClicked();
    void onExportWavClicked();
    void onDragDawClicked();
    void onDragMidiClicked();
    void onUpdateExtractorClicked();
    void onCleanCacheClicked();
    void onLicenseBadgeClicked();
    void updateLicenseUiState();
    void updatePlatformUiState();
    void updateFxAndMpcUiState();
    void refreshVideoTimeRangeLabel();

    ClipSamplerAudioProcessor& audioProcessor;
    ScalablePluginLookAndFeel scalableLnf;
    int licenseCheckTickCounter = 0;
    int lastKnownVideoDuration = -1;
    bool nextShiftPositive = true;

    // Botones de la barra de encabezado (Estado de Licencia / Prueba de 3 Días y Mantenimiento)
    juce::TextButton licenseBadgeButton;
    juce::TextButton updateExtractorButton { juce::String::fromUTF8("Actualizar Motor") };
    juce::TextButton cleanCacheButton      { juce::String::fromUTF8("Limpiar Caché") };

    // =========================================================================
    // Panel Superior: Plataforma (YouTube / TikTok / Instagram), URL, Duración y Coloración de Audio
    // =========================================================================
    juce::GroupComponent topSectionGroup {
        "topGroup",
        juce::String::fromUTF8("1. PLATAFORMA (YOUTUBE / TIKTOK / INSTAGRAM), EXTRACCIÓN Y COLORACIÓN DE AUDIO")
    };

    // Fila 0: Selector de Plataforma
    juce::Label platformLabel          { {}, juce::String::fromUTF8("Plataforma:") };
    juce::TextButton platformYoutubeBtn   { juce::String::fromUTF8("▶ YouTube") };
    juce::TextButton platformTiktokBtn    { juce::String::fromUTF8("♪ TikTok") };
    juce::TextButton platformInstagramBtn { juce::String::fromUTF8("◉ Instagram") };
    juce::Label platformHintLabel;

    // Fila 1: Enlace URL + Pegar + Detectar Duración
    juce::Label urlLabel               { {}, juce::String::fromUTF8("Enlace (URL):") };
    juce::TextEditor urlEditor;
    juce::TextButton pasteButton          { juce::String::fromUTF8("Pegar") };
    juce::TextButton detectDurationButton { juce::String::fromUTF8("Detectar Duración") };

    // Fila 2: Tiempo de inicio (MM:SS) + Barra deslizadora + Rango del video
    juce::Label startTimeLabel         { {}, juce::String::fromUTF8("Inicio (MM:SS):") };
    juce::TextEditor startTimeEditor;
    juce::Slider startTimeSlider;
    juce::Label videoRangeBadgeLabel;

    // Fila 3: Botones de extracción (3s, 10s, 30s) + Botón condicional "Nueva Extracción"
    juce::TextButton extract3sButton     { juce::String::fromUTF8("Extraer 3 Segundos") };
    juce::TextButton extract10sButton    { juce::String::fromUTF8("Extraer 10 Segundos") };
    juce::TextButton extract30sButton    { juce::String::fromUTF8("Extraer 30 Segundos") };
    juce::TextButton newExtractionButton { juce::String::fromUTF8("Nueva Extracción (2 Audios)") };

    // Fila 4: Suite de 4 botones de Coloración y Derechos IA (Calor Analógico, Aire 3D, Derechos IA, Verificar)
    juce::TextButton tapeWarmthBtn      { juce::String::fromUTF8("📼 Calor Analógico") };
    juce::TextButton spatialAirBtn      { juce::String::fromUTF8("✨ Aire y Espacio 3D") };
    juce::TextButton derechosAiBtn      { juce::String::fromUTF8("🤖 Derechos IA (±3 st)") };
    juce::TextButton detectCopyrightBtn { juce::String::fromUTF8("🛡️ Verificar Derechos") };
    juce::TextButton verifyFreeBtn      { juce::String::fromUTF8("Verificar Audio 2") };

    // Fila 5: Barra de progreso y estado
    double currentProgressValue = 0.0;
    juce::ProgressBar progressBar { currentProgressValue };
    juce::Label statusLabel;
    juce::Label copyrightBadgeLabel;
    juce::String lastDisplayedStatus;

    // =========================================================================
    // Panel Inferior: Visor Dual, Rebanador MPC (C1-G2), Perillas de Efectos y Conversor MIDI
    // =========================================================================
    juce::GroupComponent bottomSectionGroup {
        "bottomGroup",
        juce::String::fromUTF8("2. VISOR DUAL DE ONDAS, REBANADOR MPC (C1-G2), EFECTOS Y CONVERSOR A MIDI")
    };

    // Selector de audio objetivo para efectos (Audio 1 vs Audio 2)
    juce::Label fxMacroTitleLabel      { {}, juce::String::fromUTF8("Efectos:") };
    juce::TextButton fxTargetAudio1Btn { juce::String::fromUTF8("Audio 1") };
    juce::TextButton fxTargetAudio2Btn { juce::String::fromUTF8("Audio 2") };

    // 5 Perillas rotatorias principales de efectos y envolvente (Lo-Fi, Tono, Filtro, Ataque, Caída)
    juce::Slider lofiKnob;
    juce::Label  lofiLabel  { {}, juce::String::fromUTF8("Lo-Fi: 0%") };

    juce::Slider pitchKnob;
    juce::Label  pitchLabel { {}, juce::String::fromUTF8("Tono: 0 st") };

    juce::Slider toneKnob;
    juce::Label  toneLabel  { {}, juce::String::fromUTF8("Filtro: 100%") };

    juce::Slider attackKnob;
    juce::Label  attackLabel { {}, juce::String::fromUTF8("Atk: 2 ms") };

    juce::Slider releaseKnob;
    juce::Label  releaseLabel { {}, juce::String::fromUTF8("Rel: 50 ms") };

    // Botones de Sincronización BPM, Impresión de Efectos y Rebanador MPC
    juce::TextButton autoSyncButton { juce::String::fromUTF8("Sinc. BPM: NO") };
    juce::TextButton autoChopButton { juce::String::fromUTF8("Rebanador MPC: SÍ") };
    juce::TextButton bakeFxButton   { juce::String::fromUTF8("Aplicar FX") };
    juce::Label bpmKeyBadgeLabel;

    // Visores de onda duales (Audio 1 Original y Audio 2 Modificado)
    WaveformComponent originalWaveformComponent;
    WaveformComponent transformedWaveformComponent;

    // Controles de transporte inferior, Corte Exacto y exportación Drag & Drop (.WAV y .MIDI)
    juce::TextButton playButton      { juce::String::fromUTF8("▶ Reproducir") };
    juce::TextButton stopButton      { juce::String::fromUTF8("■ Detener") };
    juce::TextButton exactCutButton  { juce::String::fromUTF8("✂ Corte Exacto") };
    juce::TextButton dragDawButton   { juce::String::fromUTF8("Arrastrar .WAV") };
    juce::TextButton dragMidiButton  { juce::String::fromUTF8("🎹 Convertir a MIDI") };
    juce::TextButton exportWavButton { juce::String::fromUTF8("Exportar .WAV...") };

    std::unique_ptr<juce::FileChooser> fileChooser;
    LicenseOverlayComponent licenseOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSamplerAudioProcessorEditor)
};

// Alias de compatibilidad hacia atrás
using ClipSampler10AudioProcessorEditor = ClipSamplerAudioProcessorEditor;

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/PluginProcessor.h"
#include "WaveformComponent.h"
#include "LicenseOverlayComponent.h"
#include "FreesoundOverlayComponent.h"

/**
 * @class ScalablePluginLookAndFeel
 * @brief Sistema de diseño visual Pro (diseno-gui-pc-pro: desktop-ergonomics-hig + juce-custom-lookandfeel).
 *
 * Características visuales y de adaptabilidad:
 * - Paleta Dark Mode por capas de elevación (Chasis Nivel 0, Paneles Nivel 1, Visores Nivel 2, Controles Nivel 3).
 * - Botones con sombra proyectada multicapa, bisel superior especular (1px), micro-desplazamiento táctil y escalado
 *   automático de texto (drawButtonText) para que ninguna palabra se recorte al redimensionar.
 * - Tarjetas de sección en Obsidiana Zafiro con cabeceras tipo cápsula e indicador LED lateral.
 * - Perillas rotatorias 3D tipo hardware DSP (FabFilter / Serum) con corona de 11 marcas radiales activas,
 *   halo luminoso (glow) y aguja con punto LED.
 * - Cajas de texto empotradas con anillo de foco cian y Tooltips flotantes de alto contraste.
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
     * garantizando que jamás se corte ni una sola letra. Aplica micro-desplazamiento táctil al presionar.
     */
    void drawButtonText(juce::Graphics& g,
                        juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override
    {
        const auto text = button.getButtonText();
        if (text.isEmpty())
            return;

        auto textBounds = button.getLocalBounds().reduced(6, 2);
        if (textBounds.getWidth() <= 4 || textBounds.getHeight() <= 4)
            return;

        if (shouldDrawButtonAsDown)
            textBounds.translate(0, 1);

        float fontSize = juce::jlimit(9.5f, 16.5f, (float)button.getHeight() * 0.42f);
        juce::Font font(fontSize, juce::Font::bold);

        const float stringW = font.getStringWidthFloat(text);
        if (stringW > (float)textBounds.getWidth() && stringW > 0.0f)
        {
            const float ratio = (float)textBounds.getWidth() / stringW;
            font = juce::Font(juce::jmax(8.5f, fontSize * ratio * 0.95f), juce::Font::bold);
        }

        g.setFont(font);
        auto textCol = button.findColour(button.getToggleState()
                                             ? juce::TextButton::textColourOnId
                                             : juce::TextButton::textColourOffId);
        if (shouldDrawButtonAsHighlighted && button.isEnabled())
            textCol = textCol.brighter(0.15f);

        // Sombra de texto sutil para máximo contraste (HIG)
        if (button.isEnabled() && textCol.getPerceivedBrightness() > 0.45f)
        {
            g.setColour(juce::Colour(0x66000000));
            g.drawFittedText(text, textBounds.translated(0, 1), juce::Justification::centred, 1, 0.68f);
        }

        g.setColour(textCol.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.45f));
        g.drawFittedText(text, textBounds, juce::Justification::centred, 1, 0.68f);
    }

    /** Dibuja el fondo biselado Pro con sombra proyectada, reflejo especular superior y halo hover. */
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.2f);
        const float cornerSize = 7.0f;

        if (!button.isEnabled())
        {
            g.setColour(juce::Colour(0xff0f172a).withAlpha(0.72f));
            g.fillRoundedRectangle(bounds, cornerSize);
            g.setColour(juce::Colour(0xff1e293b));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
            return;
        }

        // 1. Sombra proyectada inferior suave (Elevación Nivel 3)
        if (!shouldDrawButtonAsDown)
        {
            g.setColour(juce::Colour(0x55000000));
            g.fillRoundedRectangle(bounds.translated(0.0f, 1.6f), cornerSize);
        }
        else
        {
            bounds.translate(0.0f, 0.8f);
        }

        juce::Colour base = backgroundColour;
        if (shouldDrawButtonAsDown)
            base = base.darker(0.06f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter(0.14f);

        // 2. Gradiente multicapa tipo hardware satinado
        juce::Colour topCol = base.brighter(shouldDrawButtonAsDown ? 0.06f : 0.22f);
        juce::Colour midCol = base;
        juce::Colour botCol = base.darker(0.28f);

        juce::ColourGradient grad(topCol, bounds.getX(), bounds.getY(),
                                  botCol, bounds.getX(), bounds.getBottom(), false);
        grad.addColour(0.48, midCol);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, cornerSize);

        // 3. Línea de luz especular superior interna (Efecto cristal/metal 1px)
        auto innerTop = bounds.reduced(2.0f, 1.0f).removeFromTop(2.0f);
        g.setColour(juce::Colours::white.withAlpha(shouldDrawButtonAsHighlighted ? 0.26f : 0.15f));
        g.drawHorizontalLine((int)innerTop.getY(), innerTop.getX() + 3.0f, innerTop.getRight() - 3.0f);

        // 4. Halo exterior en estado Hover y borde de precisión
        if (shouldDrawButtonAsHighlighted)
        {
            g.setColour(base.brighter(0.55f).withAlpha(0.28f));
            g.drawRoundedRectangle(bounds.expanded(0.8f), cornerSize + 0.8f, 2.2f);
        }

        juce::Colour borderCol = shouldDrawButtonAsHighlighted
            ? base.brighter(0.68f)
            : base.brighter(0.28f).withAlpha(0.82f);
        g.setColour(borderCol);
        g.drawRoundedRectangle(bounds, cornerSize, shouldDrawButtonAsHighlighted ? 1.5f : 1.05f);
    }

    /** Dibuja las tarjetas de sección por capas (Dark Mode Nivel 1) con sombra, bisel especular y cápsula LED. */
    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                   const juce::String& text, const juce::Justification&,
                                   juce::GroupComponent&) override
    {
        auto bounds = juce::Rectangle<float>(1.5f, 4.5f, (float)width - 3.0f, (float)height - 6.0f);
        const float corner = 10.5f;

        // 1. Sombra exterior profunda para separar el panel del chasis (Nivel 0 -> Nivel 1)
        g.setColour(juce::Colour(0x48000000));
        g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), corner);

        // 2. Superficie Dark Mode por capas (Obsidiana Zafiro Elevada)
        juce::ColourGradient cardGrad(juce::Colour(0xf2111b30), bounds.getX(), bounds.getY(),
                                      juce::Colour(0xf50a1120), bounds.getX(), bounds.getBottom(), false);
        cardGrad.addColour(0.35, juce::Colour(0xf20e1729));
        g.setGradientFill(cardGrad);
        g.fillRoundedRectangle(bounds, corner);

        // 3. Reflejo especular superior interno (1px)
        g.setColour(juce::Colour(0x20ffffff));
        g.drawRoundedRectangle(bounds.reduced(1.0f), corner, 1.0f);

        // 4. Borde perimetral con acento dinámico
        g.setColour(themeAccent.withAlpha(0.40f));
        g.drawRoundedRectangle(bounds, corner, 1.25f);

        // 5. Cápsula superior de título adaptable con indicador LED lateral
        if (text.isNotEmpty())
        {
            const float maxPillW = juce::jmax(100.0f, (float)width - 28.0f);
            float fontSize = juce::jlimit(9.5f, 13.0f, 11.5f * uiScale);
            juce::Font f(fontSize, juce::Font::bold);
            float reqW = f.getStringWidthFloat(text) + 30.0f;
            if (reqW > maxPillW && reqW > 0.0f)
            {
                fontSize = juce::jmax(8.5f, fontSize * (maxPillW / reqW) * 0.96f);
                f = juce::Font(fontSize, juce::Font::bold);
                reqW = maxPillW;
            }

            const float pillH = juce::jlimit(18.0f, 24.0f, 20.0f * uiScale);
            juce::Rectangle<float> pill(14.0f, 0.0f, juce::jmin(maxPillW, reqW), pillH);

            juce::ColourGradient pillGrad(juce::Colour(0xff1e2f4d), pill.getX(), pill.getY(),
                                          juce::Colour(0xff111c33), pill.getX(), pill.getBottom(), false);
            g.setGradientFill(pillGrad);
            g.fillRoundedRectangle(pill, 6.0f);

            // Barra de acento luminosa izquierda dentro de la cápsula
            auto accentBar = pill.removeFromLeft(5.0f).reduced(1.5f, 3.5f);
            g.setColour(themeAccent);
            g.fillRoundedRectangle(accentBar, 1.5f);

            g.setColour(themeAccent.withAlpha(0.75f));
            g.drawRoundedRectangle(juce::Rectangle<float>(14.0f, 0.0f, juce::jmin(maxPillW, reqW), pillH), 6.0f, 1.1f);

            g.setFont(f);
            g.setColour(juce::Colour(0xfff8fafc));
            g.drawFittedText(text, pill.reduced(6.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.70f);
        }
    }

    /** Dibuja la barra deslizadora horizontal de tiempo de inicio (MM:SS) con pista LED y halo. */
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        auto trackBounds = juce::Rectangle<float>((float)x, (float)y + (float)height * 0.5f - 4.0f, (float)width, 8.0f);
        g.setColour(juce::Colour(0xff080d1a));
        g.fillRoundedRectangle(trackBounds, 4.0f);
        g.setColour(juce::Colour(0xff283a5e));
        g.drawRoundedRectangle(trackBounds, 4.0f, 1.0f);

        float fillW = juce::jlimit(0.0f, (float)width, sliderPos - (float)x);
        if (fillW > 2.0f)
        {
            auto fillRect = trackBounds.withWidth(fillW);
            juce::Colour trackCol = slider.findColour(juce::Slider::trackColourId);
            juce::ColourGradient fillGrad(trackCol.darker(0.20f), fillRect.getX(), fillRect.getY(),
                                          trackCol.brighter(0.30f), fillRect.getRight(), fillRect.getY(), false);
            g.setGradientFill(fillGrad);
            g.fillRoundedRectangle(fillRect, 4.0f);
        }

        // Cursor luminoso de triple anillo con halo suave
        float thumbX = juce::jlimit((float)x + 6.0f, (float)(x + width) - 6.0f, sliderPos);
        float thumbY = (float)y + (float)height * 0.5f;
        juce::Colour thumbCol = slider.findColour(juce::Slider::thumbColourId);

        g.setColour(thumbCol.withAlpha(0.28f));
        g.fillEllipse(thumbX - 10.5f, thumbY - 10.5f, 21.0f, 21.0f);

        g.setColour(thumbCol);
        g.fillEllipse(thumbX - 6.2f, thumbY - 6.2f, 12.4f, 12.4f);

        g.setColour(juce::Colours::white);
        g.fillEllipse(thumbX - 2.6f, thumbY - 2.6f, 5.2f, 5.2f);
    }

    /** Dibuja las perillas rotatorias 3D tipo Hardware DSP (FabFilter / Serum) con corona iluminada y halo. */
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
        juce::Colour arcCol = slider.findColour(juce::Slider::rotarySliderFillColourId);

        // 1. Corona de 11 marcas radiales (se iluminan las marcas alcanzadas por el valor actual)
        for (int i = 0; i <= 10; ++i)
        {
            float prop = (float)i / 10.0f;
            float tickAngle = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle);
            bool isReached = (prop <= sliderPosProportional + 0.01f);
            g.setColour(isReached ? arcCol.withAlpha(0.82f) : juce::Colour(0xff33476b));
            float tx1 = centreX + (radius + 1.4f) * std::sin(tickAngle);
            float ty1 = centreY - (radius + 1.4f) * std::cos(tickAngle);
            float tx2 = centreX + (radius + 3.2f) * std::sin(tickAngle);
            float ty2 = centreY - (radius + 3.2f) * std::cos(tickAngle);
            g.drawLine(tx1, ty1, tx2, ty2, isReached ? 1.4f : 1.0f);
        }

        // 2. Sombra proyectada bajo el cuerpo de la perilla
        g.setColour(juce::Colour(0x66000000));
        g.fillEllipse(rx, ry + 1.5f, rw, rw);

        // 3. Capuchón 3D anodizado oscuro con bisel especular
        juce::ColourGradient knobGrad(juce::Colour(0xff293a5c), rx, ry,
                                      juce::Colour(0xff0d1526), rx, ry + rw, false);
        g.setGradientFill(knobGrad);
        g.fillEllipse(rx, ry, rw, rw);

        g.setColour(juce::Colour(0x22ffffff));
        g.drawEllipse(rx + 1.2f, ry + 1.2f, rw - 2.4f, rw - 2.4f, 0.9f);

        // 4. Pista base del arco
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff1b2842));
        g.strokePath(bgArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 5. Halo luminoso exterior + Arco de valor activo
        if (sliderPosProportional > 0.005f)
        {
            juce::Path valArc;
            valArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);

            // Halo suave (Glow)
            g.setColour(arcCol.withAlpha(0.24f));
            g.strokePath(valArc, juce::PathStrokeType(5.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Núcleo brillante
            g.setColour(arcCol.brighter(0.15f));
            g.strokePath(valArc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 6. Aguja indicadora de precisión con punto LED en la punta
        juce::Path p;
        auto pointerLength = radius * 0.68f;
        auto pointerThickness = 2.2f;
        p.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 1.8f, pointerThickness, pointerLength, 1.0f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(p);

        float dotDist = radius * 0.62f;
        float dotX = centreX + dotDist * std::sin(angle);
        float dotY = centreY - dotDist * std::cos(angle);
        g.setColour(arcCol.brighter(0.35f));
        g.fillEllipse(dotX - 1.8f, dotY - 1.8f, 3.6f, 3.6f);
    }

    /** Cajas de texto empotradas (Nivel 2) con bordes redondeados y anillo de foco cian. */
    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override
    {
        auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(0.5f);
        g.setColour(textEditor.findColour(juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle(bounds, 6.0f);
    }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override
    {
        if (!textEditor.isEnabled())
            return;

        auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
        if (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly())
        {
            g.setColour(juce::Colour(0xff38bdf8).withAlpha(0.25f));
            g.drawRoundedRectangle(bounds.expanded(0.8f), 6.5f, 2.4f);
            g.setColour(juce::Colour(0xff38bdf8));
            g.drawRoundedRectangle(bounds, 6.0f, 1.5f);
        }
        else
        {
            g.setColour(textEditor.findColour(juce::TextEditor::outlineColourId));
            g.drawRoundedRectangle(bounds, 6.0f, 1.1f);
        }
    }

    /** Tooltips flotantes modernos de alto contraste (HIG). */
    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override
    {
        auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
        juce::ColourGradient bg(juce::Colour(0xfa0f172a), 0.0f, 0.0f,
                                juce::Colour(0xfa080d1a), 0.0f, (float)height, false);
        g.setGradientFill(bg);
        g.fillRoundedRectangle(bounds, 6.0f);

        g.setColour(juce::Colour(0xff38bdf8).withAlpha(0.80f));
        g.drawRoundedRectangle(bounds, 6.0f, 1.2f);

        g.setColour(juce::Colour(0xfff8fafc));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawFittedText(text, juce::Rectangle<int>(width, height).reduced(8, 4), juce::Justification::centred, 3);
    }

    /** Menús desplegables (ComboBox) empotrados de nivel profesional con bisel y flecha vectorial cian. */
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                      juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
        const float corner = 6.5f;
        const bool isHovered = box.isMouseOverOrDragging(true);

        juce::Colour topCol = isHovered ? juce::Colour(0xff162647) : juce::Colour(0xff101c36);
        juce::Colour botCol = isHovered ? juce::Colour(0xff0b1428) : juce::Colour(0xff080f1f);
        juce::ColourGradient grad(topCol, bounds.getX(), bounds.getY(),
                                  botCol, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, corner);

        // Bisel especular superior interno
        g.setColour(juce::Colours::white.withAlpha(isHovered ? 0.14f : 0.08f));
        g.drawHorizontalLine(1, bounds.getX() + 4.0f, bounds.getRight() - 4.0f);

        // Borde iluminado al pasar el ratón o desplegar
        juce::Colour borderCol = (isButtonDown || isHovered)
            ? juce::Colour(0xff38bdf8)
            : juce::Colour(0xff2c426b);
        g.setColour(borderCol);
        g.drawRoundedRectangle(bounds, corner, (isButtonDown || isHovered) ? 1.4f : 1.05f);

        // Flecha chevron vectorial moderna a la derecha
        const float arrowX = (float)width - 17.0f;
        const float arrowY = (float)height * 0.5f;
        juce::Path arrow;
        arrow.startNewSubPath(arrowX - 4.2f, arrowY - 2.0f);
        arrow.lineTo(arrowX, arrowY + 2.4f);
        arrow.lineTo(arrowX + 4.2f, arrowY - 2.0f);
        g.setColour((isButtonDown || isHovered) ? juce::Colour(0xff38bdf8) : juce::Colour(0xff94a3b8));
        g.strokePath(arrow, juce::PathStrokeType(1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override
    {
        return juce::Font(juce::jlimit(10.5f, 13.0f, (float)box.getHeight() * 0.44f), juce::Font::bold);
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(8, 1, box.getWidth() - 28, box.getHeight() - 2);
        label.setFont(getComboBoxFont(box));
        label.setColour(juce::Label::textColourId, juce::Colour(0xfff1f5f9));
    }

    /** Menú emergente (PopupMenu) en Dark Mode Zafiro con selección iluminada. */
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
        juce::ColourGradient bg(juce::Colour(0xff111d36), 0.0f, 0.0f,
                                juce::Colour(0xff080f1e), 0.0f, (float)height, false);
        g.setGradientFill(bg);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(juce::Colour(0xff38bdf8).withAlpha(0.65f));
        g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           const bool isSeparator, const bool isActive,
                           const bool isHighlighted, const bool isTicked,
                           const bool /*hasSubMenu*/, const juce::String& text,
                           const juce::String& /*shortcutKeyText*/,
                           const juce::Drawable* /*icon*/, const juce::Colour* const /*textColourToUse*/) override
    {
        if (isSeparator)
        {
            auto r = area.reduced(6, 0);
            g.setColour(juce::Colour(0xff1e293b));
            g.fillRect(r.removeFromTop(1));
            return;
        }

        auto r = area.reduced(4, 2).toFloat();
        if (isHighlighted && isActive)
        {
            juce::ColourGradient hl(juce::Colour(0xff0284c7), r.getX(), r.getY(),
                                    juce::Colour(0xff0369a1), r.getX(), r.getBottom(), false);
            g.setGradientFill(hl);
            g.fillRoundedRectangle(r, 4.5f);
        }
        else if (isTicked)
        {
            g.setColour(juce::Colour(0xff17294a));
            g.fillRoundedRectangle(r, 4.5f);
        }

        g.setColour(isHighlighted ? juce::Colours::white
                                  : (isTicked ? juce::Colour(0xff38bdf8) : juce::Colour(0xffe2e8f0)));
        g.setFont(juce::Font(12.2f, (isHighlighted || isTicked) ? juce::Font::bold : juce::Font::plain));

        auto textRect = area.reduced(12, 0);
        if (isTicked)
        {
            g.drawText(juce::String::fromUTF8("✓ ") + text, textRect, juce::Justification::centredLeft, true);
        }
        else
        {
            g.drawText(text, textRect, juce::Justification::centredLeft, true);
        }
    }

private:
    float uiScale = 1.0f;
    juce::Colour themeAccent { 0xff38bdf8 };
};

/**
 * @class ClipSamplerAudioProcessorEditor
 * @brief Interfaz Gráfica Principal 100% Adaptable (Responsive) y en Español de ClipSampler v2.6.1.
 */
class ClipSamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::DragAndDropContainer,
                                        public juce::FileDragAndDropTarget,
                                        private juce::Timer
{
public:
    explicit ClipSamplerAudioProcessorEditor(ClipSamplerAudioProcessor&);
    ~ClipSamplerAudioProcessorEditor() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;

    // Manejadores de eventos de usuario
    void onLoadLocalFileClicked();
    void onUsoLibreClicked();
    void loadLocalAudioFile(const juce::File& file);
    void onSelectPlatform(SourcePlatform platform, bool updateStatusMsg = true);
    void onPasteClicked();
    void onDetectDurationClicked();
    void onToggleExtractModeClicked();
    void onExtractDurationClicked(int durationSec);
    void onNewExtractionClicked();
    void onTapeWarmthClicked();
    void onCheckCopyrightOriginalClicked();
    void onApplyDerechosClicked(int semitones);
    void onVerifyDerechosFreeClicked();
    void onSelectFxTargetSlot(int slotIndex);
    void onToggleAutoChopClicked();
    void onToggleAutoSyncClicked();
    void onBakeFxClicked();
    void onPlayClicked();
    void onStopClicked();
    void onExactCutClicked();
    void onPadReverseClicked();
    void onPadHalfTimeClicked();
    void onExportWavClicked();
    void onDragDawClicked();
    void onDragMidiClicked();
    void onUpdateExtractorClicked();
    void onCleanCacheClicked();
    void onManualButtonClicked();
    void onLicenseBadgeClicked();
    void onFreesoundButtonClicked();
    void updateLicenseUiState();
    void updatePlatformUiState();
    void updateFxAndMpcUiState();
    void refreshVideoTimeRangeLabel();

    ClipSamplerAudioProcessor& audioProcessor;
    ScalablePluginLookAndFeel scalableLnf;
    juce::TooltipWindow tooltipWindow { this, 420 };
    int licenseCheckTickCounter = 0;
    int uiSyncTickCounter = 0;
    int lastKnownVideoDuration = -1;

    // Botones de la barra de encabezado (Estado de Licencia / Prueba de 3 Días, Freesound, Mantenimiento y Manual PDF)
    juce::TextButton licenseBadgeButton;
    juce::TextButton freesoundButton       { juce::String::fromUTF8("🎵 Freesound") };
    juce::TextButton updateExtractorButton { juce::String::fromUTF8("Actualizar Motor") };
    juce::TextButton cleanCacheButton      { juce::String::fromUTF8("Limpiar Caché") };
    juce::TextButton manualButton          { juce::String::fromUTF8("📘 Manual") };

    // =========================================================================
    // Panel Superior: Sampler de Transformación Local y Extracción Web (Uso Libre)
    // =========================================================================
    juce::GroupComponent topSectionGroup {
        "topGroup",
        juce::String::fromUTF8("1. SAMPLER DE TRANSFORMACIÓN LOCAL Y EXTRACCIÓN (USO LIBRE)")
    };

    // Fila 0: Cargar Audio Local + Activar Uso Libre (Web) + Selector de Plataforma Web
    juce::TextButton loadLocalFileButton  { juce::String::fromUTF8("📂 Cargar Audio Local") };
    juce::TextButton usoLibreButton       { juce::String::fromUTF8("⚖️ Uso Libre (Web)") };
    juce::TextButton platformYoutubeBtn   { juce::String::fromUTF8("▶ YouTube") };
    juce::TextButton platformTiktokBtn    { juce::String::fromUTF8("♪ TikTok") };
    juce::TextButton platformInstagramBtn { juce::String::fromUTF8("◉ Instagram") };
    juce::Label platformLabel             { {}, juce::String::fromUTF8("Plataforma:") };
    juce::Label platformHintLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;

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

    // Fila 3: Conmutador de Modo (Segundos vs Compases) + Botones de extracción + "Nueva Extracción"
    bool isBarsExtractMode = false;
    juce::TextButton extractModeButton   { juce::String::fromUTF8("⏱️ Segundos") };
    juce::TextButton extract3sButton     { juce::String::fromUTF8("Extraer 3 Segundos") };
    juce::TextButton extract10sButton    { juce::String::fromUTF8("Extraer 10 Segundos") };
    juce::TextButton extract30sButton    { juce::String::fromUTF8("Extraer 30 Segundos") };
    juce::TextButton newExtractionButton { juce::String::fromUTF8("🔄 Nueva Extracción") };

    // Fila 4: Suite de Coloración y Derechos IA (Calor Analógico, Alterar +3 Tonos Más, Alterar -3 Tonos Menos, Verificar)
    juce::TextButton tapeWarmthBtn      { juce::String::fromUTF8("📼 Calor Analógico") };
    juce::TextButton derechosPlusBtn    { juce::String::fromUTF8("▲ Alterar +3 Tonos Más") };
    juce::TextButton derechosMinusBtn   { juce::String::fromUTF8("▼ Alterar -3 Tonos Menos") };
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
    juce::TextButton exactCutButton   { juce::String::fromUTF8("✂ Corte Exacto") };
    juce::TextButton padReverseButton { juce::String::fromUTF8("◀ Rev") };
    juce::TextButton padHalfTimeButton{ juce::String::fromUTF8("⏳ 1/2") };
    juce::TextButton dragDawButton    { juce::String::fromUTF8("Arrastrar .WAV") };
    juce::TextButton dragMidiButton  { juce::String::fromUTF8("🎹 Convertir a MIDI") };
    juce::TextButton exportWavButton { juce::String::fromUTF8("Exportar .WAV...") };

    LicenseOverlayComponent licenseOverlay;
    FreesoundOverlayComponent freesoundOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSamplerAudioProcessorEditor)
};

// Alias de compatibilidad hacia atrás
using ClipSampler10AudioProcessorEditor = ClipSamplerAudioProcessorEditor;

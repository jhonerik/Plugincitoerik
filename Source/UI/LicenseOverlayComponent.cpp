#include "LicenseOverlayComponent.h"

// ==============================================================================
// Constructor e Inicialización de Controles del Modal de Licencia
// ==============================================================================
LicenseOverlayComponent::LicenseOverlayComponent(LicenseManager& manager, std::function<void()> onStateChangedCallback)
    : licenseManager(manager),
      onStateChanged(std::move(onStateChangedCallback))
{
    setInterceptsMouseClicks(true, true);

    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setMinimumHorizontalScale(0.70f);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setFont(juce::Font(13.0f, juce::Font::plain));
    subtitleLabel.setJustificationType(juce::Justification::centred);
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    subtitleLabel.setMinimumHorizontalScale(0.75f);
    addAndMakeVisible(subtitleLabel);

    // 1. Sección de ID de Máquina (Hardware ID)
    machineIdTitleLabel.setText(juce::String::fromUTF8("1. Tu ID de Máquina Único (Hardware ID):"), juce::dontSendNotification);
    machineIdTitleLabel.setFont(juce::Font(12.5f, juce::Font::bold));
    machineIdTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff93c5fd));
    addAndMakeVisible(machineIdTitleLabel);

    machineIdBox.setReadOnly(true);
    machineIdBox.setText(licenseManager.getMachineId(), false);
    machineIdBox.setJustification(juce::Justification::centred);
    machineIdBox.setFont(juce::Font(15.0f, juce::Font::bold));
    machineIdBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff152036));
    machineIdBox.setColour(juce::TextEditor::textColourId, juce::Colour(0xff38bdf8));
    machineIdBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3b527d));
    addAndMakeVisible(machineIdBox);

    copyMachineIdButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2c3e63));
    copyMachineIdButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    copyMachineIdButton.onClick = [this] { onCopyMachineIdClicked(); };
    addAndMakeVisible(copyMachineIdButton);

    // 2. Sección de Clave de Licencia
    licenseKeyTitleLabel.setText(juce::String::fromUTF8("2. Ingresa tu Código de Activación de Licencia:"), juce::dontSendNotification);
    licenseKeyTitleLabel.setFont(juce::Font(12.5f, juce::Font::bold));
    licenseKeyTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff93c5fd));
    addAndMakeVisible(licenseKeyTitleLabel);

    licenseKeyEditor.setTextToShowWhenEmpty("CS10-XXXX-XXXX-XXXX-XXXX", juce::Colour(0xff64748b));
    licenseKeyEditor.setJustification(juce::Justification::centred);
    licenseKeyEditor.setFont(juce::Font(15.0f, juce::Font::bold));
    licenseKeyEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff152036));
    licenseKeyEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    licenseKeyEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(licenseKeyEditor);

    pasteLicenseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2c3e63));
    pasteLicenseButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pasteLicenseButton.onClick = [this] { onPasteLicenseClicked(); };
    addAndMakeVisible(pasteLicenseButton);

    // Botón principal de activación
    activateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00c853));
    activateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    activateButton.onClick = [this] { onActivateClicked(); };
    addAndMakeVisible(activateButton);

    // Botón para continuar la prueba gratuita (visible solo si los 3 días siguen activos)
    continueTrialButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff283859));
    continueTrialButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe2e8f0));
    continueTrialButton.onClick = [this] { onContinueTrialClicked(); };
    addAndMakeVisible(continueTrialButton);

    feedbackLabel.setFont(juce::Font(12.5f, juce::Font::bold));
    feedbackLabel.setJustificationType(juce::Justification::centred);
    feedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffca28));
    feedbackLabel.setMinimumHorizontalScale(0.70f);
    addAndMakeVisible(feedbackLabel);

    refreshState();
}

// ==============================================================================
// Actualización Dinámica de Estado (Licencia Activa / Prueba 3 Días / Expirada)
// ==============================================================================
void LicenseOverlayComponent::refreshState()
{
    licenseManager.verifyAndSyncState();
    machineIdBox.setText(licenseManager.getMachineId(), false);

    if (licenseManager.isLicensed())
    {
        titleLabel.setText(juce::String::fromUTF8("CLIPSAMPLER — ACTIVADO CON LICENCIA OFICIAL"), juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e676));
        subtitleLabel.setText(juce::String::fromUTF8("Tu copia de ClipSampler está vinculada y autorizada en este equipo.\nClave activa: ")
                              + licenseManager.getActivatedLicenseKey(), juce::dontSendNotification);
        continueTrialButton.setButtonText(juce::String::fromUTF8("Volver al Plugin"));
        continueTrialButton.setVisible(true);
    }
    else if (licenseManager.isTrialActive())
    {
        const juce::int64 rem = licenseManager.getRemainingTrialSeconds();
        const juce::int64 days = rem / 86400;
        const juce::int64 hours = (rem % 86400) / 3600;
        const juce::int64 mins = (rem % 3600) / 60;

        titleLabel.setText(juce::String::fromUTF8("PERIODO DE PRUEBA GRATUITO (3 DÍAS)"), juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
        subtitleLabel.setText(juce::String::fromUTF8("Tiempo restante de prueba: ")
                              + juce::String(days) + juce::String::fromUTF8(" día(s), ")
                              + juce::String(hours) + "h " + juce::String(mins)
                              + juce::String::fromUTF8("m.\nAl finalizar los 3 días, el programa se desactivará hasta que ingreses tu código de licencia."),
                              juce::dontSendNotification);
        continueTrialButton.setButtonText(juce::String::fromUTF8("Continuar usando versión de prueba"));
        continueTrialButton.setVisible(true);
    }
    else
    {
        titleLabel.setText(juce::String::fromUTF8("PRUEBA DE 3 DÍAS FINALIZADA — ACTIVACIÓN REQUERIDA"), juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff5252));

        if (licenseManager.isClockTampered())
        {
            subtitleLabel.setText(juce::String::fromUTF8("Se detectó una alteración en la fecha/hora del sistema. El periodo de prueba ha sido bloqueado.\nIngresa tu código de licencia oficial para activar ClipSampler."),
                                  juce::dontSendNotification);
        }
        else
        {
            subtitleLabel.setText(juce::String::fromUTF8("Tus 3 días de prueba gratuita han concluido y el plugin se ha desactivado.\nEnvía tu ID de Máquina o adquiere tu licencia para desbloquear todas las funciones permanentemente."),
                                  juce::dontSendNotification);
        }

        continueTrialButton.setVisible(false);
    }

    repaint();
}

void LicenseOverlayComponent::onCopyMachineIdClicked()
{
    juce::SystemClipboard::copyTextToClipboard(licenseManager.getMachineId());
    feedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    feedbackLabel.setText(juce::String::fromUTF8("ID de Máquina copiado al portapapeles: ") + licenseManager.getMachineId(), juce::dontSendNotification);
}

void LicenseOverlayComponent::onPasteLicenseClicked()
{
    juce::String clip = juce::SystemClipboard::getTextFromClipboard().trim();
    if (clip.isNotEmpty())
    {
        licenseKeyEditor.setText(clip.toUpperCase(), false);
        feedbackLabel.setText({}, juce::dontSendNotification);
    }
}

void LicenseOverlayComponent::onActivateClicked()
{
    auto res = licenseManager.activateLicense(licenseKeyEditor.getText());
    if (res.success)
    {
        feedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e676));
        feedbackLabel.setText(res.message, juce::dontSendNotification);
        refreshState();
        if (onStateChanged)
            onStateChanged();
        setVisible(false);
    }
    else
    {
        feedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff5252));
        feedbackLabel.setText(res.message, juce::dontSendNotification);
        refreshState();
    }
}

void LicenseOverlayComponent::onContinueTrialClicked()
{
    refreshState();
    if (licenseManager.isUnlocked())
    {
        setVisible(false);
        if (onStateChanged)
            onStateChanged();
    }
}

// ==============================================================================
// Renderizado Elegante y Diseño Responsivo del Modal de Licencia
// ==============================================================================
void LicenseOverlayComponent::paint(juce::Graphics& g)
{
    // Velo traslúcido azul zafiro profundo sobre la ventana
    g.fillAll(juce::Colour(0xe6111a2e));

    const int cardW = juce::jlimit(460, 820, getWidth() - 60);
    const int cardH = juce::jlimit(360, 500, getHeight() - 60);
    auto card = getLocalBounds().withSizeKeepingCentre(cardW, cardH).toFloat();

    juce::ColourGradient cardGrad(juce::Colour(0xff233252), card.getX(), card.getY(),
                                  juce::Colour(0xff18233b), card.getX(), card.getBottom(), false);
    g.setGradientFill(cardGrad);
    g.fillRoundedRectangle(card, 12.0f);

    juce::Colour borderCol = licenseManager.isLicensed()
        ? juce::Colour(0xff00e676)
        : (licenseManager.isTrialActive() ? juce::Colour(0xff38bdf8) : juce::Colour(0xffff5252));

    g.setColour(borderCol.withAlpha(0.85f));
    g.drawRoundedRectangle(card, 12.0f, 2.0f);
}

void LicenseOverlayComponent::resized()
{
    const int cardW = juce::jlimit(460, 820, getWidth() - 60);
    const int cardH = juce::jlimit(360, 500, getHeight() - 60);
    auto card = getLocalBounds().withSizeKeepingCentre(cardW, cardH).reduced(24, 20);
    const float hScale = juce::jlimit(0.80f, 1.15f, (float)cardH / 460.0f);

    titleLabel.setBounds(card.removeFromTop(juce::roundToInt(28 * hScale)));
    card.removeFromTop(juce::roundToInt(4 * hScale));
    subtitleLabel.setBounds(card.removeFromTop(juce::roundToInt(40 * hScale)));
    card.removeFromTop(juce::roundToInt(14 * hScale));

    // Fila de ID de Máquina
    machineIdTitleLabel.setBounds(card.removeFromTop(juce::roundToInt(20 * hScale)));
    card.removeFromTop(4);
    auto mchRow = card.removeFromTop(juce::roundToInt(32 * hScale));
    copyMachineIdButton.setBounds(mchRow.removeFromRight(105));
    mchRow.removeFromRight(8);
    machineIdBox.setBounds(mchRow);

    card.removeFromTop(juce::roundToInt(12 * hScale));

    // Fila de Clave de Licencia
    licenseKeyTitleLabel.setBounds(card.removeFromTop(juce::roundToInt(20 * hScale)));
    card.removeFromTop(4);
    auto keyRow = card.removeFromTop(juce::roundToInt(34 * hScale));
    pasteLicenseButton.setBounds(keyRow.removeFromRight(105));
    keyRow.removeFromRight(8);
    licenseKeyEditor.setBounds(keyRow);

    card.removeFromTop(juce::roundToInt(14 * hScale));

    // Botón de Activación
    activateButton.setBounds(card.removeFromTop(juce::roundToInt(38 * hScale)));
    card.removeFromTop(6);

    // Etiqueta de respuesta
    feedbackLabel.setBounds(card.removeFromTop(juce::roundToInt(22 * hScale)));
    card.removeFromTop(4);

    // Botón para continuar la prueba gratuita
    continueTrialButton.setBounds(card.removeFromTop(juce::roundToInt(32 * hScale)).reduced(juce::jmin(80, card.getWidth() / 6), 0));
}

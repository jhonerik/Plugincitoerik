#include "LicenseOverlayComponent.h"

LicenseOverlayComponent::LicenseOverlayComponent(LicenseManager& manager, std::function<void()> onStateChangedCallback)
    : licenseManager(manager),
      onStateChanged(std::move(onStateChangedCallback))
{
    setInterceptsMouseClicks(true, true);

    titleLabel.setFont(juce::Font(21.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setFont(juce::Font(13.0f, juce::Font::plain));
    subtitleLabel.setJustificationType(juce::Justification::centred);
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0bac9));
    addAndMakeVisible(subtitleLabel);

    // Hardware Machine ID section
    machineIdTitleLabel.setText(juce::String::fromUTF8("1. Tu ID de Máquina Único (Hardware ID):"), juce::dontSendNotification);
    machineIdTitleLabel.setFont(juce::Font(12.5f, juce::Font::bold));
    machineIdTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8c9ba5));
    addAndMakeVisible(machineIdTitleLabel);

    machineIdBox.setReadOnly(true);
    machineIdBox.setText(licenseManager.getMachineId(), false);
    machineIdBox.setJustification(juce::Justification::centred);
    machineIdBox.setFont(juce::Font(15.0f, juce::Font::bold));
    machineIdBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff141821));
    machineIdBox.setColour(juce::TextEditor::textColourId, juce::Colour(0xff00e5ff));
    machineIdBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2d3444));
    addAndMakeVisible(machineIdBox);

    copyMachineIdButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3444));
    copyMachineIdButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    copyMachineIdButton.onClick = [this] { onCopyMachineIdClicked(); };
    addAndMakeVisible(copyMachineIdButton);

    // License Key Input section
    licenseKeyTitleLabel.setText(juce::String::fromUTF8("2. Ingresa tu Código de Activación de Licencia:"), juce::dontSendNotification);
    licenseKeyTitleLabel.setFont(juce::Font(12.5f, juce::Font::bold));
    licenseKeyTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8c9ba5));
    addAndMakeVisible(licenseKeyTitleLabel);

    licenseKeyEditor.setTextToShowWhenEmpty("CS10-XXXX-XXXX-XXXX-XXXX", juce::Colour(0xff5a6372));
    licenseKeyEditor.setJustification(juce::Justification::centred);
    licenseKeyEditor.setFont(juce::Font(15.0f, juce::Font::bold));
    licenseKeyEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff141821));
    licenseKeyEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    licenseKeyEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(licenseKeyEditor);

    pasteLicenseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3444));
    pasteLicenseButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pasteLicenseButton.onClick = [this] { onPasteLicenseClicked(); };
    addAndMakeVisible(pasteLicenseButton);

    // Primary Activate button
    activateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00c853));
    activateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    activateButton.onClick = [this] { onActivateClicked(); };
    addAndMakeVisible(activateButton);

    // Continue Trial / Close button (only enabled if trial is still active or already licensed)
    continueTrialButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222733));
    continueTrialButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb0bac9));
    continueTrialButton.onClick = [this] { onContinueTrialClicked(); };
    addAndMakeVisible(continueTrialButton);

    feedbackLabel.setFont(juce::Font(12.5f, juce::Font::bold));
    feedbackLabel.setJustificationType(juce::Justification::centred);
    feedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffb74d));
    addAndMakeVisible(feedbackLabel);

    refreshState();
}

void LicenseOverlayComponent::refreshState()
{
    licenseManager.verifyAndSyncState();
    machineIdBox.setText(licenseManager.getMachineId(), false);

    if (licenseManager.isLicensed())
    {
        titleLabel.setText(juce::String::fromUTF8("✓ PRODUCTO ACTIVADO CON LICENCIA OFICIAL"), juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e676));
        subtitleLabel.setText(juce::String::fromUTF8("Tu copia de ClipSampler10 está vinculada y autorizada en este equipo.\nClave activa: ")
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

        titleLabel.setText(juce::String::fromUTF8("⏳ PERIODO DE PRUEBA GRATUITO (3 DÍAS)"), juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
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
        titleLabel.setText(juce::String::fromUTF8("🔒 PRUEBA DE 3 DÍAS FINALIZADA — ACTIVACIÓN REQUERIDA"), juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff1744));

        if (licenseManager.isClockTampered())
        {
            subtitleLabel.setText(juce::String::fromUTF8("Se detectó una alteración en la fecha/hora del sistema. El periodo de prueba ha sido bloqueado.\nIngresa tu código de licencia oficial para activar ClipSampler10."),
                                  juce::dontSendNotification);
        }
        else
        {
            subtitleLabel.setText(juce::String::fromUTF8("Tus 3 días de prueba gratuita han concluido y el plugin se ha desactivado.\nEnvía tu ID de Máquina o adquiere tu licencia para desbloquear todas las funciones permanentemente."),
                                  juce::dontSendNotification);
        }

        // Strictly hide the "Continue Trial" button once the 3 days are over
        continueTrialButton.setVisible(false);
    }

    repaint();
}

void LicenseOverlayComponent::onCopyMachineIdClicked()
{
    juce::SystemClipboard::copyTextToClipboard(licenseManager.getMachineId());
    feedbackLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    feedbackLabel.setText(juce::String::fromUTF8("✓ ID de Máquina copiado al portapapeles: ") + licenseManager.getMachineId(), juce::dontSendNotification);
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

void LicenseOverlayComponent::paint(juce::Graphics& g)
{
    // Semi-transparent dark backdrop over the whole plugin window
    g.fillAll(juce::Colour(0xee0b0d12));

    auto card = getLocalBounds().reduced(70, 58).toFloat();

    // Card background & border
    g.setColour(juce::Colour(0xff181c26));
    g.fillRoundedRectangle(card, 12.0f);

    juce::Colour borderCol = licenseManager.isLicensed()
        ? juce::Colour(0xff00e676)
        : (licenseManager.isTrialActive() ? juce::Colour(0xff00e5ff) : juce::Colour(0xffff1744));

    g.setColour(borderCol.withAlpha(0.75f));
    g.drawRoundedRectangle(card, 12.0f, 2.0f);
}

void LicenseOverlayComponent::resized()
{
    auto card = getLocalBounds().reduced(95, 76);

    titleLabel.setBounds(card.removeFromTop(30));
    card.removeFromTop(6);
    subtitleLabel.setBounds(card.removeFromTop(42));
    card.removeFromTop(18);

    // Machine ID Row
    machineIdTitleLabel.setBounds(card.removeFromTop(20));
    card.removeFromTop(4);
    auto mchRow = card.removeFromTop(34);
    copyMachineIdButton.setBounds(mchRow.removeFromRight(105));
    mchRow.removeFromRight(8);
    machineIdBox.setBounds(mchRow);

    card.removeFromTop(16);

    // License Key Row
    licenseKeyTitleLabel.setBounds(card.removeFromTop(20));
    card.removeFromTop(4);
    auto keyRow = card.removeFromTop(36);
    pasteLicenseButton.setBounds(keyRow.removeFromRight(105));
    keyRow.removeFromRight(8);
    licenseKeyEditor.setBounds(keyRow);

    card.removeFromTop(18);

    // Activate Button
    activateButton.setBounds(card.removeFromTop(40));
    card.removeFromTop(8);

    // Feedback Label
    feedbackLabel.setBounds(card.removeFromTop(24));
    card.removeFromTop(6);

    // Continue Trial / Return Button
    continueTrialButton.setBounds(card.removeFromTop(32).reduced(90, 0));
}

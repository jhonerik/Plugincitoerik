#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Security/LicenseManager.h"
#include <functional>

/**
 * @class LicenseOverlayComponent
 * @brief Ventana modal adaptable de activación de licencia y control de los 3 días de prueba para ClipSampler.
 *
 * Funcionalidades:
 * - Muestra el tiempo restante del periodo de prueba gratuito de 3 días.
 * - Muestra el ID de Máquina único del usuario ("MCH-XXXX-XXXX-XXXX") con botón para copiar en 1 clic.
 * - Permite pegar y activar la clave de licencia ("CS10-XXXX-XXXX-XXXX-XXXX").
 * - Bloquea el uso del plugin cuando los 3 días de prueba han concluido hasta que se ingrese una licencia válida.
 */
class LicenseOverlayComponent : public juce::Component
{
public:
    explicit LicenseOverlayComponent(LicenseManager& manager, std::function<void()> onStateChangedCallback);
    ~LicenseOverlayComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Sincroniza los textos y botones según el estado actual de la licencia o prueba de 3 días. */
    void refreshState();

private:
    void onCopyMachineIdClicked();
    void onPasteLicenseClicked();
    void onActivateClicked();
    void onContinueTrialClicked();

    LicenseManager& licenseManager;
    std::function<void()> onStateChanged;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::Label machineIdTitleLabel;
    juce::TextEditor machineIdBox;
    juce::TextButton copyMachineIdButton { juce::String::fromUTF8("Copiar ID") };

    juce::Label licenseKeyTitleLabel;
    juce::TextEditor licenseKeyEditor;
    juce::TextButton pasteLicenseButton { juce::String::fromUTF8("Pegar") };

    juce::TextButton activateButton { juce::String::fromUTF8("ACTIVAR LICENCIA AHORA") };
    juce::TextButton continueTrialButton { juce::String::fromUTF8("Continuar con los 3 días de prueba") };

    juce::Label feedbackLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LicenseOverlayComponent)
};

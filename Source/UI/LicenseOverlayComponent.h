#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Security/LicenseManager.h"
#include <functional>

/**
 * LicenseOverlayComponent
 * 
 * Modal activation and trial-lock screen for ClipSampler10.
 * - Displays remaining 3-day trial time (or lock screen when the 3-day trial expires).
 * - Shows the user's unique Hardware Machine ID ("MCH-XXXX-XXXX-XXXX") with a 1-click Copy button.
 * - Allows pasting and activating a license key ("CS10-XXXX-XXXX-XXXX-XXXX").
 * - Blocks all underlying plugin controls when the 3-day trial has expired.
 */
class LicenseOverlayComponent : public juce::Component
{
public:
    explicit LicenseOverlayComponent(LicenseManager& manager, std::function<void()> onStateChangedCallback);
    ~LicenseOverlayComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Refreshes buttons and labels based on current license/trial state. */
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

    juce::TextButton activateButton { juce::String::fromUTF8("🔐 ACTIVAR LICENCIA AHORA") };
    juce::TextButton continueTrialButton { juce::String::fromUTF8("Continuar con los 3 días de prueba") };

    juce::Label feedbackLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LicenseOverlayComponent)
};

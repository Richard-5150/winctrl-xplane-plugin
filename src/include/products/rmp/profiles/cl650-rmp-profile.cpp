#include "cl650-rmp-profile.h"

#include "dataref.h"
#include "product-rmp.h"

#include <algorithm>

// The RMP drives the radio section of the CL650 cursor control panel (CCP).
// The CL650 shows frequencies on the MFD, so the RMP displays stay off.

const char *CL650RMPProfile::ccpName() const {
    // The CL650 has no third CCP; the standby RMP drives the captain's
    return product->deviceVariant == RMPDeviceVariant::VARIANT_FIRSTOFFICER ? "2" : "1";
}

CL650RMPProfile::CL650RMPProfile(ProductRMP *product) :
    RMPAircraftProfile(product) {
    product->setLedBrightness(RMPLed::LCD_BRIGHTNESS, 0);
    product->setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, 0);

    const char *panelLighting = product->deviceVariant == RMPDeviceVariant::VARIANT_FIRSTOFFICER ? "CL650/lamps/integ/2A4FH_ccp2" : "CL650/lamps/integ/1A4FH_ccp1";
    Dataref::getInstance()->monitorExistingDataref<float>(panelLighting, [product](float brightness) {
        product->setLedBrightness(RMPLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255));
    },
        this);
}

bool CL650RMPProfile::IsEligible() {
    return Dataref::getInstance()->exists("CL650/CDU/1/screen/text_line0");
}

const std::vector<std::string> &CL650RMPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {};
    return datarefs;
}

const std::unordered_map<uint16_t, RMPButtonDef> &CL650RMPProfile::buttonDefs() const {
    static std::unordered_map<RMPDeviceVariant, std::unordered_map<uint16_t, RMPButtonDef>> cache;

    if (cache.find(product->deviceVariant) == cache.end()) {
        const std::string ccp = std::string("CL650/CCP/") + ccpName() + "/";

        cache[product->deviceVariant] = {
            {0, {"Flip frequencies", ccp + "freq"}},
            {1, {"VHF 1", ccp + "radio"}},
            {2, {"VHF 2", ccp + "1_2"}},
            {3, {"VHF 3", ccp + "dme_h"}},
            {4, {"LOAD", ""}},
            {5, {"HF1", ccp + "ident"}},
            {6, {"HF2", ""}},
            {7, {"AM", ""}},
            {8, {"NAV", ""}},
            {9, {"VOR", ""}},
            {10, {"ILS", ""}},
            {11, {"GLS", ""}},
            {12, {"MLS", ""}},
            {13, {"ADF", ""}},
            {14, {"Knob Large L", ccp + "radio_adv_down"}},
            {15, {"Knob Large R", ccp + "radio_adv_up"}},
            {16, {"Knob Small L", ccp + "radio_data_down"}},
            {17, {"Knob Small R", ccp + "radio_data_up"}},
            {18, {"Knob depress", ccp + "radio_push"}},
            // The CCP radio section has no power switch
            {19, {"Switch On", ""}},
            {20, {"Switch Off", ""}},
        };
    }

    return cache[product->deviceVariant];
}

void CL650RMPProfile::buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

void CL650RMPProfile::updateDisplays() {
    product->setDisplayText("      ", "      ");
}

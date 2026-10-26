#include "cl650-pdc-profile.h"

#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

namespace {
    // A held knob starts repeating after this delay
    constexpr float kRepeatDelay = 0.5f;
    constexpr float kSlowRepeatInterval = 0.15f;
    // The slow contact can open again just before the fast one closes, and
    // closes again briefly on the way back from the fast detent
    constexpr float kFastEngageWindow = 0.15f;
    constexpr float kFastReleaseWindow = 0.3f;
}

CL650PDCProfile::CL650PDCProfile(ProductPDC *product) :
    PDCAircraftProfile(product) {
    baroKnob.fastRepeatInterval = 0.03f;
    menuKnob.fastRepeatInterval = 0.25f;

    // Follow the FCP lighting like the MCP does, so both panels share the
    // glareshield dimmer instead of the DCP's pedestal one.
    Dataref::getInstance()->monitorExistingDataref<float>("CL650/lamps/integ/A1CA_fcp", [product](float brightness) {
        product->setLedBrightness(PDCLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255));
    },
        this);
}

bool CL650PDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("CL650/CDU/1/screen/text_line0");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &CL650PDCProfile::buttonDefs() const {
    static std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> cache;

    // try_emplace would build the whole table before the lookup; this runs per button index.
    auto it = cache.find(product->deviceVariant);
    if (it != cache.end()) {
        return it->second;
    }

    const std::string dcp = std::string("CL650/DCP/") + (product->isCaptainSide() ? "1" : "2") + "/";
    const std::string rangeCmds = dcp + "range_down," + dcp + "range_up";

    // The PDC is a 737 EFIS panel; its controls are mapped onto the CL650 DCP.
    // MINS knob: slow turns MENU DATA, fast turns MENU ADV, RST pushes the MENU knob.
    // Buttons are held for as long as the hardware button, so the cockpit button moves with it.
    return cache.try_emplace(product->deviceVariant,
                    std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                        {{0, 0}, {"FPV", dcp + "nav_src", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{1, 1}, {"MTRS", dcp + "brg_src", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{-1, 2}, {"3M VSD", ""}},
                        {{2, 3}, {"WXR", dcp + "tr_wx", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{3, 4}, {"STA", dcp + "refs", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{4, 5}, {"WPT", dcp + "menu", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{5, 6}, {"ARPT", dcp + "esc", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{6, 7}, {"DATA", dcp + "rdr_menu", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{7, 8}, {"POS", dcp + "tilt_auto", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{8, 9}, {"TERR", dcp + "radar", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{9, 10}, {"LEFT VOR1", ""}},
                        {{10, 11}, {"LEFT OFF", ""}},
                        {{11, 12}, {"LEFT ADF1", ""}},
                        {{12, 13}, {"RIGHT VOR2", ""}},
                        {{13, 14}, {"RIGHT OFF", ""}},
                        {{14, 15}, {"RIGHT ADF2", ""}},
                        {{15, 16}, {"Mins RST", dcp + "menu_push", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{16, 17}, {"VOR MAP CTR", dcp + "frmt", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{17, 18}, {"RANGE TFC", dcp + "tfc", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{18, 19}, {"Baro STD", dcp + "baro_push", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{-1, 20}, {"3M Range Minus", dcp + "range_down"}},
                        {{-1, 21}, {"3M Range Plus", dcp + "range_up"}},
                        {{21, 22}, {"Baro knob left fast", dcp + "baro_down", PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
                        {{22, 23}, {"Baro knob right fast", dcp + "baro_up", PDCDatarefType::ADD_BARO_REPEATING, 10.0}},
                        {{23, 24}, {"Mins RADIO", ""}},
                        {{24, 25}, {"Mins BARO", ""}},
                        {{25, 26}, {"Baro inHg", ""}},
                        {{26, 27}, {"Baro HPA", ""}},
                        {{27, 28}, {"Map APP", ""}},
                        {{28, 29}, {"Map VOR", ""}},
                        {{29, 30}, {"Map MAP", ""}},
                        {{30, 31}, {"Map PLN", ""}},
                        // The CL650 range has more steps than the selector and no readable
                        // position, so the selector steps the range knob by the detents turned.
                        {{31, -1}, {"3N Map range 5", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{32, -1}, {"3N Map range 10", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
                        {{33, -1}, {"3N Map range 20", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
                        {{34, -1}, {"3N Map range 40", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
                        {{35, -1}, {"3N Map range 80", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
                        {{36, -1}, {"3N Map range 160", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 5.0}},
                        {{37, -1}, {"3N Map range 320", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 6.0}},
                        {{38, -1}, {"3N Map range 640", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 7.0}},
                        {{19, 32}, {"Mins knob left fast", dcp + "menu_adv_down", PDCDatarefType::ADD_MINIMUMS_REPEATING, -10.0}},
                        {{39, 33}, {"Mins knob left slow", dcp + "menu_data_down", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                        {{40, 34}, {"Mins knob center", ""}},
                        {{41, 35}, {"Mins knob right slow", dcp + "menu_data_up", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                        {{20, 36}, {"Mins knob right fast", dcp + "menu_adv_up", PDCDatarefType::ADD_MINIMUMS_REPEATING, 10.0}},
                        {{42, 37}, {"Baro knob left slow", dcp + "baro_down", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                        {{43, 38}, {"Baro knob center", ""}},
                        {{44, 39}, {"Baro knob right slow", dcp + "baro_up", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                    })
        .first->second;
}

void CL650PDCProfile::update() {
    float now = XPLMGetElapsedTime();

    for (KnobState *knob : {&baroKnob, &menuKnob}) {
        if (knob->pendingSlow && now >= knob->pendingSlowTime) {
            Dataref::getInstance()->executeCommand(knob->pendingSlow->dataref.c_str());
            knob->pendingSlow = nullptr;
        }

        const PDCButtonDef *held = knob->fast ? knob->fast : knob->slow;
        if (!held || now < knob->lastCommandTime) {
            continue;
        }

        knob->lastCommandTime = now + (knob->fast ? knob->fastRepeatInterval : kSlowRepeatInterval);
        if (!knob->fast) {
            knob->slowFired = true;
        }
        Dataref::getInstance()->executeCommand(held->dataref.c_str());
    }
}

void CL650PDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        knobPressed(baroKnob, button, phase, false);
    } else if (button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        // Slow and fast turn different knobs here, so a slow flick only fires
        // on release, once it is clear the fast detent was not on its way.
        knobPressed(menuKnob, button, phase, true);
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE_USING_COMMANDS) {
        stepRange(button);
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PDCDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void CL650PDCProfile::knobPressed(KnobState &knob, const PDCButtonDef *button, XPLMCommandPhase phase, bool deferSlow) {
    bool isFast = std::abs(button->value) > 1.0;
    float now = XPLMGetElapsedTime();

    if (phase == xplm_CommandBegin) {
        knob.lastCommandTime = now + kRepeatDelay;

        if (isFast) {
            knob.fast = button;
            knob.fastSeen = true;
            knob.pendingSlow = nullptr;
            Dataref::getInstance()->executeCommand(button->dataref.c_str());
        } else {
            bool returningFromFast = now - knob.fastReleaseTime < kFastReleaseWindow;
            knob.slow = button;
            knob.fastSeen = false;
            knob.slowFired = !deferSlow || returningFromFast;
            if (!deferSlow && !returningFromFast) {
                Dataref::getInstance()->executeCommand(button->dataref.c_str());
            }
        }
        return;
    }

    if (isFast) {
        // Back in the slow detent: no single step on release, but repeat again if held
        knob.fast = nullptr;
        knob.fastReleaseTime = now;
        knob.slowFired = true;
        knob.lastCommandTime = now + kRepeatDelay;
    } else {
        if (!knob.slowFired && !knob.fastSeen) {
            knob.pendingSlow = button;
            knob.pendingSlowTime = now + kFastEngageWindow;
        }
        knob.slow = nullptr;
        knob.fastSeen = false;
    }
}

void CL650PDCProfile::stepRange(const PDCButtonDef *button) {
    // Format: "decrease_cmd,increase_cmd"
    size_t comma = button->dataref.find(',');
    if (comma == std::string::npos) {
        return;
    }

    int position = static_cast<int>(button->value);
    if (lastRangePosition >= 0) {
        const std::string cmd = position > lastRangePosition ? button->dataref.substr(comma + 1) : button->dataref.substr(0, comma);
        for (int i = std::abs(position - lastRangePosition); i > 0; i--) {
            Dataref::getInstance()->executeCommand(cmd.c_str());
        }
    }

    // The first position reported only sets the reference point
    lastRangePosition = position;
}

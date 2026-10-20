#include "ff767-pdc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include <XPLMProcessing.h>

namespace {
    std::vector<std::string> SplitDatarefs(const std::string &datarefs) {
        std::stringstream ss(datarefs);
        std::string item;
        std::vector<std::string> parts;
        while (std::getline(ss, item, ',')) {
            parts.push_back(item);
        }

        return parts;
    }
}

FF767PDCProfile::FF767PDCProfile(ProductPDC *product) : PDCAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("lights/glareshield1_rhe", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");
        uint8_t target = hasPower ? std::clamp(brightness, 0.0f, 1.0f) * 255 : 0;
        product->setLedBrightness(PDCLed::BACKLIGHT, target);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power", [](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("lights/glareshield1_rhe");
    },
        this);
}

bool FF767PDCProfile::IsEligible() {
    // Same check as the FF767 FCU/EFIS profile; the FF777 has 1-sim/output/mcp/ok.
    return Dataref::getInstance()->exists("1-sim/AP/cmd_C_Button") &&
           !Dataref::getInstance()->exists("1-sim/output/mcp/ok");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &FF767PDCProfile::buttonDefs() const {
    static std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> cache;

    // try_emplace would build the whole table before the lookup; this runs per button index.
    auto it = cache.find(product->deviceVariant);
    if (it != cache.end()) {
        return it->second;
    }

    const bool isCaptain = product->isCaptainSide();
    const std::string panel = isCaptain ? "1" : "2";
    const std::string side = isCaptain ? "L" : "R";
    const std::string ndCmd = "1-sim/command/ndpanel/" + panel + "/";
    const std::string modeSelector = "1-sim/efis/ctrlPanel/" + panel + "/hsiModeRotary," + ndCmd + "hsiModeRotary_switch-," + ndCmd + "hsiModeRotary_switch+";
    const std::string rangeCmd = "1-sim/command/ng/range" + side + "/anim_rotary";
    const std::string rangeSelector = "1-sim/ndpanel/" + panel + "/hsiRangeRotary," + rangeCmd + "-," + rangeCmd + "+";
    const std::string vorL = "1-sim/ng/leftVor/" + side + "/anim,1-sim/efis/ctrlPanel/leftVor/" + side + "/anim";
    const std::string vorR = "1-sim/ng/rightVor/" + side + "/anim,1-sim/efis/ctrlPanel/rightVor/" + side + "/anim";
    // Outer rings of the baro and mins knobs, -1 or 0.
    const std::string baroMode = "1-sim/ng/baroLarge" + side + "/anim/anim,1-sim/ng/baroLarge" + side + "/anim,1-sim/efis/ctrlPanel/baroLarge" + side + "/anim";
    const std::string minsMode = "1-sim/ng/minsLarge" + side + "/anim/anim,1-sim/ng/minsLarge" + side + "/anim,1-sim/efis/ctrlPanel/minsLarge" + side + "/anim";

    return cache.try_emplace(product->deviceVariant,
                    std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                        {{0, 0}, {"FPV", "1-sim/ng/FPV/" + side + ",1-sim/efis/ctrlPanel/FPV/" + side, PDCDatarefType::TOGGLE_VALUE}},
                        {{1, 1}, {"MTRS", "1-sim/ng/mtrs/" + side + ",1-sim/efis/ctrlPanel/mtrs/" + side, PDCDatarefType::TOGGLE_VALUE}},
                        {{-1, 2}, {"3M VSD", ""}},
                        {{2, 3}, {"WXR", "1-sim/ndpanel/" + panel + "/map1,1-sim/efis/ctrlPanel/" + panel + "/map1", PDCDatarefType::TOGGLE_VALUE}},
                        {{3, 4}, {"STA", ndCmd + "map2_button"}},
                        {{4, 5}, {"WPT", ndCmd + "map5_button"}},
                        {{5, 6}, {"ARPT", ndCmd + "map3_button"}},
                        {{6, 7}, {"DATA", ndCmd + "map4_button"}},
                        {{7, 8}, {"POS", ""}},  // No POS button on the 757/767
                        {{8, 9}, {"TERR", ""}}, // No TERR button on the 757/767
                        // Sign convention taken from the FF767 FCU/EFIS profile: -1 = VOR, 1 = ADF.
                        {{9, 10}, {"LEFT VOR1", vorL, PDCDatarefType::SET_VALUE, -1.0}},
                        {{10, 11}, {"LEFT OFF", vorL, PDCDatarefType::SET_VALUE, 0.0}},
                        {{11, 12}, {"LEFT ADF1", vorL, PDCDatarefType::SET_VALUE, 1.0}},
                        {{12, 13}, {"RIGHT VOR2", vorR, PDCDatarefType::SET_VALUE, -1.0}},
                        {{13, 14}, {"RIGHT OFF", vorR, PDCDatarefType::SET_VALUE, 0.0}},
                        {{14, 15}, {"RIGHT ADF2", vorR, PDCDatarefType::SET_VALUE, 1.0}},
                        {{15, 16}, {"Mins RST", "1-sim/ng/mins/rst/" + side + ",1-sim/efis/ctrlPanel/mins/rst/" + side, PDCDatarefType::TOGGLE_VALUE}},
                        {{16, 17}, {"VOR MAP CTR", "1-sim/ndpanel/" + panel + "/hsiModeButton,1-sim/efis/ctrlPanel/" + panel + "/hsiModeButton", PDCDatarefType::TOGGLE_VALUE}},
                        {{17, 18}, {"RANGE TFC", "1-sim/ng/map/tfc/" + side + ",1-sim/efis/ctrlPanel/map/tfc/" + side, PDCDatarefType::TOGGLE_VALUE}},
                        {{18, 19}, {"Baro STD", "1-sim/ng/baro/std/" + side + ",1-sim/efis/ctrlPanel/baro/std/" + side, PDCDatarefType::TOGGLE_VALUE}},
                        {{-1, 20}, {"3M Range Minus", rangeCmd + "-"}},
                        {{-1, 21}, {"3M Range Plus", rangeCmd + "+"}},
                        {{21, 22}, {"Baro knob left fast", "baroSmall", PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
                        {{22, 23}, {"Baro knob right fast", "baroSmall", PDCDatarefType::ADD_BARO_REPEATING, 10.0}},
                        {{23, 24}, {"Mins RADIO", minsMode, PDCDatarefType::SET_VALUE, 0.0}},
                        {{24, 25}, {"Mins BARO", minsMode, PDCDatarefType::SET_VALUE, -1.0}},
                        {{25, 26}, {"Baro inHg", baroMode, PDCDatarefType::SET_VALUE, 0.0}},
                        {{26, 27}, {"Baro HPA", baroMode, PDCDatarefType::SET_VALUE, -1.0}},
                        {{27, 28}, {"Map APP", modeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{28, 29}, {"Map VOR", modeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
                        {{29, 30}, {"Map MAP", modeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
                        {{30, 31}, {"Map PLN", modeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
                        // The 757/767 selector runs 10-640, so 5 clamps to 10.
                        {{31, -1}, {"3N Map range 5", rangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{32, -1}, {"3N Map range 10", rangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{33, -1}, {"3N Map range 20", rangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
                        {{34, -1}, {"3N Map range 40", rangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
                        {{35, -1}, {"3N Map range 80", rangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
                        {{36, -1}, {"3N Map range 160", rangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
                        {{37, -1}, {"3N Map range 320", rangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 5.0}},
                        {{38, -1}, {"3N Map range 640", rangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 6.0}},
                        {{19, 32}, {"Mins knob left fast", "minsSmall", PDCDatarefType::ADD_MINIMUMS_REPEATING, -10.0}},
                        {{39, 33}, {"Mins knob left slow", "minsSmall", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                        {{40, 34}, {"Mins knob center", ""}},
                        {{41, 35}, {"Mins knob right slow", "minsSmall", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                        {{20, 36}, {"Mins knob right fast", "minsSmall", PDCDatarefType::ADD_MINIMUMS_REPEATING, 10.0}},
                        {{42, 37}, {"Baro knob left slow", "baroSmall", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                        {{43, 38}, {"Baro knob center", ""}},
                        {{44, 39}, {"Baro knob right slow", "baroSmall", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                    })
        .first->second;
}

void FF767PDCProfile::update() {
    float now = XPLMGetElapsedTime();

    for (auto [knob, knobName] : {std::pair<KnobState *, const char *>{&baroKnob, "baroSmall"}, {&minimumsKnob, "minsSmall"}}) {
        char delta = knob->fastDelta != 0 ? knob->fastDelta : knob->slowDelta;
        if (delta != 0 && now - knob->lastCommandTime >= 0.1f) {
            knob->lastCommandTime = now;
            turnKnob(knobName, delta);
        }
    }
}

void FF767PDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING || button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        KnobState &knob = button->datarefType == PDCDatarefType::ADD_BARO_REPEATING ? baroKnob : minimumsKnob;
        char value = static_cast<char>(button->value);
        bool isFast = value == 10 || value == -10;
        float now = XPLMGetElapsedTime();

        if (phase == xplm_CommandEnd) {
            if (isFast) {
                knob.fastDelta = 0;
                knob.fastReleaseTime = now;
                knob.lastCommandTime = now + 1.0f;
            } else {
                knob.slowDelta = 0;
            }
        } else if (isFast) {
            knob.fastDelta = value;
            knob.lastCommandTime = now + 1.0f;
            turnKnob(button->dataref.c_str(), value);
        } else {
            knob.slowDelta = value;
            knob.lastCommandTime = now + 1.0f;
            // Skip the ±1 while the fast detent is engaged or being released through.
            if (knob.fastDelta == 0 && now - knob.fastReleaseTime > 0.3f) {
                turnKnob(button->dataref.c_str(), value);
            }
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::TOGGLE_VALUE) {
        // The aircraft keeps the latched state in every listed copy; the first one is read back.
        auto datarefs = SplitDatarefs(button->dataref);
        float newValue = datarefManager->get<float>(datarefs[0].c_str()) > 0.5f ? 0.0f : 1.0f;
        for (const auto &dataref : datarefs) {
            datarefManager->set<float>(dataref.c_str(), newValue);
        }

    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE_USING_COMMANDS) {
        // Format: "position_dataref,decrease_cmd,increase_cmd"
        auto parts = SplitDatarefs(button->dataref);
        if (parts.size() != 3) {
            return;
        }

        int current = static_cast<int>(std::lround(datarefManager->get<float>(parts[0].c_str())));
        int target = static_cast<int>(button->value);

        for (int i = current; i < target; i++) {
            datarefManager->executeCommand(parts[2].c_str());
        }
        for (int i = current; i > target; i--) {
            datarefManager->executeCommand(parts[1].c_str());
        }

    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE) {
        for (const auto &dataref : SplitDatarefs(button->dataref)) {
            datarefManager->set<float>(dataref.c_str(), button->value);
        }

    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

void FF767PDCProfile::turnKnob(const char *knobName, char delta) {
    if (delta == 0) {
        return;
    }

    // Inner knob of the baro or mins selector; one click in the cockpit moves both copies by 5.
    const std::string name = std::string(knobName) + (product->isCaptainSide() ? "L" : "R");
    auto datarefManager = Dataref::getInstance();
    for (const std::string &dataref : {"1-sim/ng/" + name + "/anim", "1-sim/efis/ctrlPanel/" + name + "/anim"}) {
        float current = datarefManager->get<float>(dataref.c_str());
        datarefManager->set<float>(dataref.c_str(), current + delta * 5.0f);
    }
}

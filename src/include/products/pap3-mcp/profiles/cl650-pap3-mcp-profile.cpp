#include "cl650-pap3-mcp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <XPLMUtilities.h>

namespace {
    // The CL650 has no MCP displays, only annunciator lamps. Each lamp is
    // driven by two bulbs that flicker slightly around 1.0 when lit and sit at
    // exactly 0 when dark.
    constexpr float kLampLitThreshold = 0.1f;

    struct LampLed {
            PAP3MCPLed led;
            const char *lamp1;
            const char *lamp2;
    };

    const LampLed kLampLeds[] = {
        {PAP3MCPLed::SPEED, "CL650/lamps/glareshield/FCP/turb_1", "CL650/lamps/glareshield/FCP/turb_2"},
        {PAP3MCPLed::VNAV, "CL650/lamps/glareshield/FCP/vnav_1", "CL650/lamps/glareshield/FCP/vnav_2"},
        {PAP3MCPLed::LVL_CHG, "CL650/lamps/glareshield/FCP/flc_1", "CL650/lamps/glareshield/FCP/flc_2"},
        {PAP3MCPLed::HDG_SEL, "CL650/lamps/glareshield/FCP/hdg_1", "CL650/lamps/glareshield/FCP/hdg_2"},
        {PAP3MCPLed::LNAV, "CL650/lamps/glareshield/FCP/nav_1", "CL650/lamps/glareshield/FCP/nav_2"},
        {PAP3MCPLed::VORLOC, "CL650/lamps/glareshield/FCP/bc_1", "CL650/lamps/glareshield/FCP/bc_2"},
        {PAP3MCPLed::APP, "CL650/lamps/glareshield/FCP/appr_1", "CL650/lamps/glareshield/FCP/appr_2"},
        {PAP3MCPLed::ALT_HLD, "CL650/lamps/glareshield/FCP/alt_1", "CL650/lamps/glareshield/FCP/alt_2"},
        {PAP3MCPLed::VS, "CL650/lamps/glareshield/FCP/vs_1", "CL650/lamps/glareshield/FCP/vs_2"},
        {PAP3MCPLed::CMD_A, "CL650/lamps/glareshield/FCP/ap_eng_1", "CL650/lamps/glareshield/FCP/ap_eng_2"},
        {PAP3MCPLed::CMD_B, "CL650/lamps/glareshield/FCP/ap_eng_1", "CL650/lamps/glareshield/FCP/ap_eng_2"},
        {PAP3MCPLed::CWS_A, "CL650/lamps/glareshield/FCP/xfr_1", "CL650/lamps/glareshield/FCP/xfr_2"},
        {PAP3MCPLed::CWS_B, "CL650/lamps/glareshield/FCP/xfr_1", "CL650/lamps/glareshield/FCP/xfr_2"},
        {PAP3MCPLed::AT_ARM, "CL650/lamps/glareshield/ats_L", "CL650/lamps/glareshield/ats_R"},
    };

    const char *kATSLamp = "CL650/lamps/glareshield/ats_L";
    const char *kHalfBankLamp = "CL650/lamps/glareshield/FCP/half_bank_1";
} // namespace

CL650PAP3MCPProfile::CL650PAP3MCPProfile(ProductPAP3MCP *product) :
    PAP3MCPAircraftProfile(product) {
    product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 0);
    product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, 255);
    product->setLedBrightness(PAP3MCPLed::N1, 0);
    product->setLedBrightness(PAP3MCPLed::MA_CAPT, 0);
    product->setLedBrightness(PAP3MCPLed::MA_FO, 0);

    Dataref::getInstance()->monitorExistingDataref<float>("CL650/lamps/integ/A1CA_fcp", [product](float brightness) {
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255));
    },
        this);

    for (const auto &entry : kLampLeds) {
        for (const char *lamp : {entry.lamp1, entry.lamp2}) {
            Dataref::getInstance()->monitorExistingDataref<float>(lamp, [this](float) {
                updateLeds();
            },
                this);
        }
    }

    // A/T disengaged in the sim: drop the magnetic A/T ARM switch
    Dataref::getInstance()->monitorExistingDataref<float>(kATSLamp, [product](float brightness) {
        if (brightness <= kLampLitThreshold) {
            product->setATSolenoid(false);
        }
    },
        this);

    updateLeds();
}

bool CL650PAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("CL650/CDU/1/screen/text_line0");
}

const std::vector<std::string> &CL650PAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {};
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &CL650PAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Held for as long as the key is, so the cockpit button moves with it
        {0, {"N1", "CL650/glareshield/N2_SYNC", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {1, {"SPEED", "CL650/FCP/turb_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {2, {"VNAV", "CL650/FCP/vnav_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {3, {"LVL CHG", "CL650/FCP/flc_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {4, {"HDG SEL", "CL650/FCP/hdg_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"LNAV", "CL650/FCP/nav_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {6, {"VOR LOC", "CL650/FCP/bc_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {7, {"APP", "CL650/FCP/appr_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {8, {"ALT HLD", "CL650/FCP/alt_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {9, {"V/S", "CL650/FCP/vs_mode", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {10, {"CMD A", "CL650/FCP/ap_eng", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {11, {"CWS A", "CL650/FCP/xfr", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {12, {"CMD B", "CL650/FCP/ap_eng", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {13, {"CWS B", "CL650/FCP/xfr", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {14, {"C/O", "CL650/FCP/ias_mach", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {15, {"SPD INTV", "CL650/FCP/hdg_sync", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {16, {"ALT INTV", "CL650/FCP/alt_cancel", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},

        // Encoder button-press events
        {17, {"CRS CAPT DEC", "CL650/FCP/crs1_down"}},
        {18, {"CRS CAPT INC", "CL650/FCP/crs1_up"}},
        {19, {"SPD DEC", "CL650/FCP/speed_down"}},
        {20, {"SPD INC", "CL650/FCP/speed_up"}},
        {21, {"HDG DEC", "CL650/FCP/hdg_down"}},
        {22, {"HDG INC", "CL650/FCP/hdg_up"}},
        {23, {"ALT DEC", "CL650/FCP/alt_down"}},
        {24, {"ALT INC", "CL650/FCP/alt_up"}},
        {25, {"CRS FO DEC", "CL650/FCP/crs2_down"}},
        {26, {"CRS FO INC", "CL650/FCP/crs2_up"}},
        {38, {"VS DEC", "CL650/FCP/vs_pitch_down"}},
        {39, {"VS INC", "CL650/FCP/vs_pitch_up"}},
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &CL650PAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "CL650/FCP/crs1_up", "CL650/FCP/crs1_down"},
        {1, "SPD", "CL650/FCP/speed_up", "CL650/FCP/speed_down"},
        {2, "HDG", "CL650/FCP/hdg_up", "CL650/FCP/hdg_down"},
        {3, "ALT", "CL650/FCP/alt_up", "CL650/FCP/alt_down"},
        {4, "V/S", "CL650/FCP/vs_pitch_up", "CL650/FCP/vs_pitch_down"},
        {5, "CRS FO", "CL650/FCP/crs2_up", "CL650/FCP/crs2_down"},
    };
    return encoders;
}

void CL650PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    data.displayEnabled = false;
}

void CL650PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void CL650PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));

    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void CL650PAP3MCPProfile::handleBankAngleSwitch(uint8_t switchByte) {
    // The CL650 only has 1/2 BANK: the 10° and 15° positions turn it on, 20° to 30° turn it off.
    // The aircraft also engages it by itself at altitude, so only act when the switch moves
    // between the two halves, and not on the position reported when the profile loads.
    int halfBank = (switchByte & (0x02 | 0x04)) != 0 ? 1 : 0;
    bool firstReport = hwHalfBank < 0;
    if (halfBank == hwHalfBank) {
        return;
    }

    hwHalfBank = halfBank;
    if (!firstReport) {
        syncToggle(isLampLit(kHalfBankLamp), halfBank == 1, "CL650/FCP/half_bank_mode");
    }
}

void CL650PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT/FO: the CL650 FD buttons are momentary toggles with no readable
    // state, so every change of the hardware switch toggles the FD.
    if (byteOffset == 0x04 && bitMask == 0x08) {
        if (state != hwFDCaptOn) {
            hwFDCaptOn = state;
            pressButton("CL650/FCP/fd1");
        }
        return;
    }

    if (byteOffset == 0x04 && bitMask == 0x20) {
        if (state != hwFDFoOn) {
            hwFDFoOn = state;
            pressButton("CL650/FCP/fd2");
        }
        return;
    }

    // A/T ARM: bit 0x01 = ARMED line, bit 0x02 = DISARMED line
    if (byteOffset == 0x06 && (bitMask == 0x01 || bitMask == 0x02)) {
        if (state) {
            bool armed = bitMask == 0x01;
            product->setATSolenoid(armed);
            syncToggle(isLampLit(kATSLamp), armed, "CL650/glareshield/ATS");
        }
        return;
    }

    // AP DISCONNECT bar: 0x04/0x80 = bar up, 0x05/0x01 = bar down.
    // CL650/FCP/ap_disc is the backup disconnect lever, 1 when pulled.
    if ((byteOffset == 0x04 && bitMask == 0x80) || (byteOffset == 0x05 && bitMask == 0x01)) {
        if (state) {
            bool disconnected = byteOffset == 0x05;
            bool leverPulled = Dataref::getInstance()->get<float>("CL650/FCP/ap_disc") > 0.5f;
            if (leverPulled != disconnected) {
                Dataref::getInstance()->executeCommand("CL650/FCP/ap_disc");
            }
        }
        return;
    }
}

bool CL650PAP3MCPProfile::isLampLit(const char *lamp) {
    return Dataref::getInstance()->getCached<float>(lamp) > kLampLitThreshold;
}

void CL650PAP3MCPProfile::updateLeds() {
    for (const auto &entry : kLampLeds) {
        bool lit = isLampLit(entry.lamp1) || isLampLit(entry.lamp2);

        auto it = ledStates.find(entry.led);
        if (it != ledStates.end() && it->second == lit) {
            continue;
        }

        ledStates[entry.led] = lit;
        product->setLedBrightness(entry.led, lit ? 1 : 0);
    }
}

void CL650PAP3MCPProfile::syncToggle(bool simOn, bool hwOn, const char *toggleCmd) {
    if (simOn != hwOn) {
        pressButton(toggleCmd);
    }
}

void CL650PAP3MCPProfile::pressButton(const char *command) {
    // A latching hardware switch has no release to follow, so hold the
    // command about as long as a click in the cockpit to animate the button.
    // No owner, so the release still runs if the profile unloads meanwhile.
    Dataref::getInstance()->executeCommand(command, xplm_CommandBegin);
    std::string cmd = command;
    AppState::getInstance()->executeAfter(200, nullptr, [cmd]() {
        Dataref::getInstance()->executeCommand(cmd.c_str(), xplm_CommandEnd);
    });
}

#include "ff767-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <XPLMUtilities.h>

namespace {
    // MCP lamp index -> PAP3 LED, from objects/757/ckpt/knobs.obj. HDG SEL, A/T ARM,
    // CWS and MA have no lamp on the 757/767; CMD C (lamp 13) has no PAP3 LED.
    struct LampLed {
            const char *dataref;
            PAP3MCPLed led;
    };

    const LampLed lampLeds[] = {
        {"1-sim/AP/lamp/1", PAP3MCPLed::N1}, // EPR
        {"1-sim/AP/lamp/2", PAP3MCPLed::SPEED},
        {"1-sim/AP/lamp/3", PAP3MCPLed::LNAV},
        {"1-sim/AP/lamp/4", PAP3MCPLed::VNAV},
        {"1-sim/AP/lamp/5", PAP3MCPLed::LVL_CHG}, // FL CH
        {"1-sim/AP/lamp/7", PAP3MCPLed::VS},
        {"1-sim/AP/lamp/8", PAP3MCPLed::ALT_HLD},
        {"1-sim/AP/lamp/10", PAP3MCPLed::VORLOC}, // LOC
        {"1-sim/AP/lamp/11", PAP3MCPLed::APP},
        {"1-sim/AP/lamp/12", PAP3MCPLed::CMD_A}, // CMD L
        {"1-sim/AP/lamp/14", PAP3MCPLed::CMD_B}, // CMD R
    };
}

FF767PAP3MCPProfile::FF767PAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("lights/glareshield1_rhe", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");

        float ratio = std::clamp(brightness, 0.0f, 1.0f);
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, hasPower ? static_cast<uint8_t>(ratio * 255) : 0);
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, hasPower ? 180 : 0);
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, hasPower ? 255 : 0);

        product->forceStateSync();
    },
        this);

    // We abuse the GPU dataref to trigger an update when the UI is closed.
    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/electrical/gpuAvailable", [](bool gpuAvailable) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/autopilot/autopilot_has_power");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power", [](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("lights/glareshield1_rhe");
    },
        this);

    for (const auto &lamp : lampLeds) {
        PAP3MCPLed led = lamp.led;
        Dataref::getInstance()->monitorExistingDataref<float>(lamp.dataref, [this, product, led](float level) {
            product->setLedBrightness(led, level > 0.0f || isTestMode() ? 1 : 0);
        },
            this);
    }

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/testPanel/test1Button", [](int test) {
        for (const auto &lamp : lampLeds) {
            Dataref::getInstance()->executeChangedCallbacksForDataref(lamp.dataref);
        }
    },
        this);
}

bool FF767PAP3MCPProfile::IsEligible() {
    // Same check as the FF767 FCU/EFIS and PDC profiles; the FF777 has 1-sim/output/mcp/ok.
    return Dataref::getInstance()->exists("1-sim/AP/cmd_C_Button") &&
           !Dataref::getInstance()->exists("1-sim/output/mcp/ok");
}

const std::vector<std::string> &FF767PAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit2/autopilot/autopilot_has_power",
        "1-sim/testPanel/test1Button",
        "1-sim/AP/iasmach",
        "1-sim/AP/dig3/spdSetting",
        "1-sim/AP/hdgSetting",
        "1-sim/AP/dig5/altSetting",
        "1-sim/AP/vviSetting",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &FF767PAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Row 1 (byte 0x01)
        {0, {"EPR", "1-sim/command/AP/eprButton_button"}},
        {1, {"SPD", "1-sim/command/AP/spdButton_button"}},
        {2, {"VNAV", "1-sim/comm/AP/vnavButton"}},
        {3, {"FL CH", "1-sim/command/AP/flchButton_button"}},
        {4, {"HDG SEL", "1-sim/command/AP/hdgConfButton_button"}},
        {5, {"LNAV", "1-sim/comm/AP/lnavButton"}},
        {6, {"LOC", "1-sim/comm/AP/locButton"}},
        {7, {"APP", "1-sim/comm/AP/appButton"}},

        // Row 2 (byte 0x02). No CWS on the 757/767; CMD C has no PAP3 button.
        {8, {"ALT HOLD", "1-sim/comm/AP/altHoldButton"}},
        {9, {"V/S", "1-sim/comm/AP/vviButton"}},
        {10, {"CMD L", "1-sim/command/AP/cmd_L_Button_button"}},
        {12, {"CMD R", "1-sim/command/AP/cmd_R_Button_button"}},
        {14, {"IAS/MACH", "1-sim/command/AP/iasmach_button"}},
        {15, {"SPD INTV", "1-sim/command/AP/spdConfButton_button"}},

        // Encoder rotations (exposed as button pairs by the hardware). No MCP course.
        {19, {"SPD DEC", "1-sim/comm/AP/spdDN"}},
        {20, {"SPD INC", "1-sim/comm/AP/spdUP"}},
        {21, {"HDG DEC", "1-sim/comm/AP/hdgDN"}},
        {22, {"HDG INC", "1-sim/comm/AP/hdgUP"}},
        {23, {"ALT DEC", "1-sim/comm/AP/altDN"}},
        {24, {"ALT INC", "1-sim/comm/AP/altUP"}},
        {38, {"VS DEC", "1-sim/comm/AP/vviDN"}},
        {39, {"VS INC", "1-sim/comm/AP/vviUP"}},
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &FF767PAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "", ""},
        {1, "SPD", "1-sim/comm/AP/spdUP", "1-sim/comm/AP/spdDN"},
        {2, "HDG", "1-sim/comm/AP/hdgUP", "1-sim/comm/AP/hdgDN"},
        {3, "ALT", "1-sim/comm/AP/altUP", "1-sim/comm/AP/altDN"},
        {4, "V/S", "1-sim/comm/AP/vviUP", "1-sim/comm/AP/vviDN"},
        {5, "CRS FO", "", ""},
    };
    return encoders;
}

void FF767PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dataref = Dataref::getInstance();

    data.displayEnabled = dataref->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");
    data.displayTest = isTestMode();
    data.showLabels = false;
    data.showCourse = false;

    // Types match the FF767 FCU/EFIS profile, which caches the same datarefs.
    data.spdMach = dataref->getCached<bool>("1-sim/AP/iasmach");
    data.speed = dataref->getCached<float>("1-sim/AP/dig3/spdSetting");
    data.heading = static_cast<int>(dataref->getCached<float>("1-sim/AP/hdgSetting")) % 360;
    data.altitude = static_cast<int>(std::round(dataref->getCached<float>("1-sim/AP/dig5/altSetting") / 100.0f) * 100);
    data.verticalSpeed = std::round(dataref->getCached<float>("1-sim/AP/vviSetting"));
}

void FF767PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void FF767PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0 || encoder->incCmd.empty() || encoder->decCmd.empty()) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));
    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void FF767PAP3MCPProfile::syncSwitch(bool simOn, bool hwOn, const char *toggleCmd) {
    if (simOn != hwOn) {
        Dataref::getInstance()->executeCommand(toggleCmd);
    }
}

// Switch values per FF's checklists: fdNSwitcher 0=ON, atSwitcher 0=ARM, desengageLever 0=UP.
void FF767PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    auto dataref = Dataref::getInstance();

    // FD CAPT: byte 0x04, bit 0x08; HIGH when ON
    if (byteOffset == 0x04 && bitMask == 0x08) {
        syncSwitch(!dataref->get<bool>("1-sim/AP/fd1Switcher"), state, "1-sim/command/AP/fd1Switcher_trigger");
        return;
    }

    // FD FO: byte 0x04, bit 0x20; HIGH when ON
    if (byteOffset == 0x04 && bitMask == 0x20) {
        syncSwitch(!dataref->get<bool>("1-sim/AP/fd2Switcher"), state, "1-sim/command/AP/fd2Switcher_trigger");
        return;
    }

    // A/T ARM: byte 0x06, bit 0x01 = ARMED line, bit 0x02 = DISARMED line
    if (byteOffset == 0x06 && (bitMask == 0x01 || bitMask == 0x02)) {
        if (state) {
            bool armed = bitMask == 0x01;
            product->setATSolenoid(armed);
            syncSwitch(dataref->get<int>("1-sim/AP/atSwitcher") == 0, armed, "1-sim/command/AP/atSwitcher_trigger");
        }
        return;
    }

    // AP DISENGAGE bar: byte 0x04 bit 0x80 rises when DOWN, byte 0x05 bit 0x01 rises when UP
    if ((byteOffset == 0x04 && bitMask == 0x80) || (byteOffset == 0x05 && bitMask == 0x01)) {
        if (state) {
            bool barUp = byteOffset == 0x05;
            syncSwitch(dataref->get<int>("1-sim/AP/desengageLever") == 0, barUp, "1-sim/command/AP/desengageLever_button");
        }
        return;
    }
}

// Positional: the PAP3's five detents map onto the 757's AUTO/10/15/20/25.
void FF767PAP3MCPProfile::handleBankAngleSwitch(uint8_t switchByte) {
    int target = -1;
    if (switchByte & 0x02) {
        target = 0; // AUTO
    } else if (switchByte & 0x04) {
        target = 1; // 10°
    } else if (switchByte & 0x08) {
        target = 2; // 15°
    } else if (switchByte & 0x10) {
        target = 3; // 20°
    } else if (switchByte & 0x20) {
        target = 4; // 25°
    }

    if (target >= 0) {
        Dataref::getInstance()->set<float>("1-sim/AP/bankLimitRotary", static_cast<float>(target));
    }
}

bool FF767PAP3MCPProfile::isTestMode() {
    return Dataref::getInstance()->getCached<int>("1-sim/testPanel/test1Button") == 2;
}

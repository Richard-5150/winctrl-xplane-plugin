#ifndef CL650_PAP3_MCP_PROFILE_H
#define CL650_PAP3_MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <string>
#include <unordered_map>
#include <vector>

class CL650PAP3MCPProfile : public PAP3MCPAircraftProfile {
    private:
        // The lamp datarefs flicker while lit, so only write an LED when it turns on or off
        std::unordered_map<PAP3MCPLed, bool> ledStates;

        // Last known hardware switch positions, so a repeated report of the
        // same position (e.g. after forceStateSync) does not toggle twice.
        bool hwFDCaptOn = false;
        bool hwFDFoOn = false;
        // Bank angle switch half (1 = 10°/15°, 0 = 20°-30°), -1 until first reported
        int hwHalfBank = -1;

        static bool isLampLit(const char *lamp);
        void updateLeds();
        void syncToggle(bool simOn, bool hwOn, const char *toggleCmd);
        void pressButton(const char *command);

    public:
        CL650PAP3MCPProfile(ProductPAP3MCP *product);

        static bool IsEligible();
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const override;
        const std::vector<PAP3MCPEncoderDef> &encoderDefs() const override;
        void updateDisplayData(PAP3MCPDisplayData &data) override;
        void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) override;
        void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) override;

        void handleBankAngleSwitch(uint8_t switchByte) override;
        void handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) override;
};

#endif

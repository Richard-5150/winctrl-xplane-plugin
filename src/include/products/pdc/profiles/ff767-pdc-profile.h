#ifndef FF767_PDC_PROFILE_H
#define FF767_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>

class FF767PDCProfile : public PDCAircraftProfile {
    private:
        // The fast detent also closes the slow contact, so both are tracked
        // separately (same reasoning as the Zibo profile, issue #109).
        struct KnobState {
                char slowDelta = 0;
                char fastDelta = 0;
                float fastReleaseTime = -1.0f;
                float lastCommandTime = 0.0f;
        };

        KnobState baroKnob;
        KnobState minimumsKnob;

        void turnKnob(const char *knobName, char delta);

    public:
        FF767PDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

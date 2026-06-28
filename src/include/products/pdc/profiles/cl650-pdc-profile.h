#ifndef CL650_PDC_PROFILE_H
#define CL650_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>

class CL650PDCProfile : public PDCAircraftProfile {
    private:
        // The fast detent also closes the slow contact, so both are tracked
        // separately (same reasoning as the Zibo profile, issue #109).
        struct KnobState {
                const PDCButtonDef *slow = nullptr;
                const PDCButtonDef *fast = nullptr;
                // Deferred slow step, fired once the fast contact had its chance to close
                const PDCButtonDef *pendingSlow = nullptr;
                bool fastSeen = false;
                bool slowFired = false;
                float lastCommandTime = 0.0f;
                float pendingSlowTime = 0.0f;
                float fastReleaseTime = -1.0f;
                float fastRepeatInterval = 0.1f;
        };

        KnobState baroKnob;
        KnobState menuKnob;
        int lastRangePosition = -1;

        void knobPressed(KnobState &knob, const PDCButtonDef *button, XPLMCommandPhase phase, bool deferSlow);
        void stepRange(const PDCButtonDef *button);

    public:
        CL650PDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

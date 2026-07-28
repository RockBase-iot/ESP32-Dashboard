#pragma once

#include <stdint.h>

enum class ButtonId : uint8_t {
    Boot = 0,
    User = 1,
};

enum class ButtonAction : uint8_t {
    None = 0,
    NextPage,
    PreviousPage,
    SyncCurrent,
    OpenConfig,
    RecoveryAp,
};

class ButtonController {
public:
    ButtonAction update(ButtonId button, bool pressed, uint32_t nowMs);

private:
    struct ButtonState {
        bool stablePressed = false;
        bool candidatePressed = false;
        bool hasCandidate = false;
        uint32_t candidateSinceMs = 0;
        uint32_t pressedSinceMs = 0;
    };

    static constexpr uint32_t kDebounceMs = 40;
    static constexpr uint32_t kLongPressMs = 2000;
    static constexpr uint32_t kRecoveryPressMs = 6000;

    ButtonAction actionForRelease(ButtonId button, uint32_t heldMs) const;
    ButtonState &stateFor(ButtonId button);

    ButtonState _boot;
    ButtonState _user;
};

#include "pending_button_action.h"

namespace {
constexpr uint32_t kPendingButtonActionMagic = 0x524F4B50UL; // "ROKP"
constexpr uint16_t kPendingButtonActionVersion = 1;

bool isValidStoredAction(uint8_t raw) {
    return raw <= static_cast<uint8_t>(ButtonAction::RecoveryAp);
}

void clear(PendingButtonActionState &state) {
    state.magic = 0;
    state.action = 0;
    state.inverseAction = 0;
    state.version = 0;
}
}  // namespace

void storePendingButtonAction(PendingButtonActionState &state, ButtonAction action) {
    const uint8_t raw = static_cast<uint8_t>(action);
    state.magic = kPendingButtonActionMagic;
    state.action = raw;
    state.inverseAction = static_cast<uint8_t>(~raw);
    state.version = kPendingButtonActionVersion;
}

ButtonAction consumePendingButtonAction(PendingButtonActionState &state) {
    const uint8_t raw = state.action;
    const bool valid = state.magic == kPendingButtonActionMagic &&
                       state.version == kPendingButtonActionVersion &&
                       isValidStoredAction(raw) &&
                       state.inverseAction == static_cast<uint8_t>(~raw);
    clear(state);
    if (!valid) {
        return ButtonAction::None;
    }
    return static_cast<ButtonAction>(raw);
}


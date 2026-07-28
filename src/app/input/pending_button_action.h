#pragma once

#include <stdint.h>

#include "button_controller.h"

struct PendingButtonActionState {
    uint32_t magic;
    uint8_t action;
    uint8_t inverseAction;
    uint16_t version;
};

void storePendingButtonAction(PendingButtonActionState &state, ButtonAction action);
ButtonAction consumePendingButtonAction(PendingButtonActionState &state);


#pragma once

#include <stdint.h>

#include "app/input/button_controller.h"
#include "app/page/page_catalog.h"
#include "app/time/focus_clock_model.h"
#include "utils/light_wake.h"

bool applyFocusClockButtonAction(PageId page,
                                 ButtonAction action,
                                 const FocusClockConfig &config,
                                 FocusClockRuntimeState &state,
                                 int64_t nowUtc);
bool focusClockBlocksButtonAction(PageId page,
                                  ButtonAction action,
                                  const FocusClockConfig &config,
                                  const FocusClockRuntimeState &state,
                                  int64_t nowUtc);
uint32_t focusClockNextRefreshMs(const FocusClockConfig &config,
                                 const FocusClockRuntimeState &state,
                                 int64_t nowUtc);
ButtonAction focusClockActionFromLightWake(LightWake wake, uint32_t heldAfterWakeMs);
ButtonAction focusClockActionFromAwakeHold(PageId page,
                                           bool userPressed,
                                           uint32_t heldMs,
                                           bool alreadyHandled);

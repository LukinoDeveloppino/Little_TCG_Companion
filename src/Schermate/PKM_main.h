#pragma once
#include "inc/ui.h"
#include "inc/app_state.h"
#include "inc/timer.h"

// ── Draw ──────────────────────────────────────────────────────────────────────
void screenMainDraw(UI &ui, const AppState &s, bool full = false);
void screenMainRedrawButtons(UI &ui, const AppState &s);

// ── Touch handler ─────────────────────────────────────────────────────────────
bool screenMainHandleTouch(UI &ui, AppState &s, Timer &gameTimer,
                           Point p,
                           const char **popupBig,
                           uint16_t *popupColor);

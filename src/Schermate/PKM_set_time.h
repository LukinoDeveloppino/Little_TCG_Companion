#pragma once
#include "inc/ui.h"
#include "inc/app_state.h"
#include "inc/timer.h"

// ── Draw ──────────────────────────────────────────────────────────────────────
void screenSettingsDraw(UI &ui, const AppState &s, bool full = false);

// ── Touch handler ─────────────────────────────────────────────────────────────
// Modifica state e gameTimer in base al tocco.
void screenSettingsHandleTouch(UI &ui, AppState &s, Timer &gameTimer, Point p);

// ── Calibrazione touch (richiamabile anche da fuori) ─────────────────────────
void runCalibration(UI &ui);
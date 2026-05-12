#pragma once
#include "inc/ui.h"
#include "inc/app_state.h"
#include "inc/timer.h"

// ── Draw ──────────────────────────────────────────────────────────────────────
void screenMainDraw(UI &ui, const AppState &s, bool full = false);
void screenMainRedrawTopBar(UI &ui, const AppState &s);
void screenMainRedrawButtons(UI &ui, const AppState &s);

// ── Touch handler ─────────────────────────────────────────────────────────────
// Modifica state e gameTimer in base al tocco.
// Ritorna true se è stato attivato un popup (caller gestisce il timer popup).
bool screenMainHandleTouch(UI &ui, AppState &s, Timer &gameTimer,
                           int tx, int ty,
                           const char **popupBig, const char **popupLabel,
                           uint16_t *popupColor);
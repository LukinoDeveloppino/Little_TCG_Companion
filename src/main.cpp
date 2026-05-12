#include <Arduino.h>
#include <Preferences.h>
#include "inc/ui.h"
#include "inc/timer.h"
#include "inc/touch.h"
#include "inc/app_state.h"
#include "Schermate/PKM_main.h"
#include "Schermate/PKM_set_time.h"

// ── Globali ───────────────────────────────────────────────────────────────────
UI       ui;
Timer    gameTimer;
AppState state;

uint32_t popupHideAt = 0;
bool     popupActive = false;

static const int CAL_VER = 4;

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  gameTimer.begin(3000);
  state.timerRemaining = 3000;

  ui.begin();
  touchInit();

  // Carica calibrazione salvata (o avvia wizard)
  Preferences prefs;
  prefs.begin("touch_cal", true);
  bool validCal = prefs.isKey("xMin") && prefs.getInt("ver", 0) == CAL_VER;
  if (validCal) {
    int  xMin = prefs.getInt("xMin"), xMax = prefs.getInt("xMax");
    int  yMin = prefs.getInt("yMin"), yMax = prefs.getInt("yMax");
    bool sw   = prefs.getBool("swap", false);
    touchSetCalibration(xMin, xMax, yMin, yMax, sw);
    Serial.printf("[CAL] loaded swap=%d xMin=%d xMax=%d yMin=%d yMax=%d\n",
                  sw, xMin, xMax, yMin, yMax);
  }
  prefs.end();

  if (!validCal) runCalibration(ui);

  screenMainDraw(ui, state, true);
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
  // Aggiorna timer
  gameTimer.update();
  if (state.timerState == TimerState::RUNNING) {
    uint32_t r = gameTimer.remaining();
    if (r != state.timerRemaining) {
      state.timerRemaining = r;
      ui.updateTimer(r);
      if (r == 0) {
        state.timerState = TimerState::IDLE;
        screenMainRedrawButtons(ui, state);
      }
    }
  }

  // Nasconde popup scaduto
  if (popupActive && millis() >= popupHideAt) {
    popupActive = false;
    ui.hidePopup();
    ui.updateTimer(state.timerRemaining); // ripristina l'area coperta dal popup
  }

  // Gestione touch
  Point p;
  if (touchReadMapped(p)) {
    if (state.screen == Screen::MAIN) {
      const char *big = nullptr, *label = nullptr;
      uint16_t    color = C_WHITE;

      bool wantsPopup = screenMainHandleTouch(ui, state, gameTimer,
                                              p,
                                              &big, &label, &color);
      // Navigazione verso settings (lo screen handler aggiorna state.screen)
      if (state.screen == Screen::SETTINGS) {
        screenSettingsDraw(ui, state, true);
      }

      if (wantsPopup && big) {
        ui.showPopup(big, color);
        popupHideAt = millis() + 2200;
        popupActive = true;
      }
    } else {
      screenSettingsHandleTouch(ui, state, gameTimer, p);
    }

    // Attendi rilascio dito + debounce
    while (digitalRead(TP_IRQ) == LOW) delay(5);
    delay(120);
  }
}
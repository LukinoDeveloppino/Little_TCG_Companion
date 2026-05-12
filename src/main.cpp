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

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  gameTimer.begin(3000);
  state.timerRemaining = 3000;

  ui.begin();
  touchInit();

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
  static uint32_t popupHideAt = 0;

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
  if (popupHideAt && millis() >= popupHideAt) {
    popupHideAt = 0;
    ui.hidePopup();
    ui.updateTimer(state.timerRemaining);
  }

  // Gestione touch
  Point p;
  if (touchReadMapped(p)) {
    if (state.screen == Screen::MAIN) {
      const char *big = nullptr;
      uint16_t    color = C_WHITE;

      bool wantsPopup = screenMainHandleTouch(ui, state, gameTimer, p, &big, &color);

      if (state.screen == Screen::SETTINGS)
        screenSettingsDraw(ui, state, true);

      if (wantsPopup && big) {
        ui.showPopup(big, color);
        popupHideAt = millis() + 2200;
      }
    } else {
      screenSettingsHandleTouch(ui, state, gameTimer, p);
    }

    while (digitalRead(TP_IRQ) == LOW) delay(5);
    delay(120);
  }
}

#include "Schermate/PKM_main.h"
#include <Arduino.h>

// ── Layout ────────────────────────────────────────────────────────────────────
static const Rect WIN_J1A      = {  28,   4,  28,  28 };
static const Rect WIN_J1B      = {  60,   4,  28,  28 };
static const Rect WIN_J2A      = { 232,   4,  28,  28 };
static const Rect WIN_J2B      = { 264,   4,  28,  28 };
static const Rect ZONE_TIMER   = {  20,  42, 280, 100 };
static const Rect BTN_TIMER_FULL  = {  10, 152, 300,  36 }; // Start (IDLE)
static const Rect BTN_TIMER_LEFT  = {  10, 152, 145,  36 }; // Pausa / Resume
static const Rect BTN_TIMER_RIGHT = { 165, 152, 145,  36 }; // Reset
static const Rect BTN_COIN     = {  10, 198, 145,  36 };
static const Rect BTN_DICE     = { 165, 198, 145,  36 };

// ── TOP BAR ───────────────────────────────────────────────────────────────────
static void drawTopBar(UI &ui, const AppState &s) {
  TFT_eSPI &tft = ui.tft();
  tft.fillRect(0, 0, 320, 36, C_BG);
  tft.drawFastHLine(0, 36, 320, C_BORDER);
  tft.setFreeFont(&FreeSans9pt7b);

  tft.setTextColor(C_GREEN, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("J1", 8, 18);
  ui.drawWinBox(WIN_J1A, s.wins[0][0], C_GREEN);
  ui.drawWinBox(WIN_J1B, s.wins[0][1], C_GREEN);

  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("BO3", 160, 18);

  ui.drawWinBox(WIN_J2A, s.wins[1][0], C_BLUE);
  ui.drawWinBox(WIN_J2B, s.wins[1][1], C_BLUE);
  tft.setTextColor(C_BLUE, C_BG);
  tft.setTextDatum(MR_DATUM);
  tft.drawString("J2", 312, 18);
}

// ── TIMER + BOTTONI ───────────────────────────────────────────────────────────
static void drawTimerButtons(UI &ui, const AppState &s) {
  ui.tft().fillRect(10, 152, 300, 36, C_BG);
  if (s.timerState == TimerState::IDLE) {
    ui.drawButton(BTN_TIMER_FULL,  "Start", C_GREEN,   C_WHITE);
  } else if (s.timerState == TimerState::RUNNING) {
    ui.drawButton(BTN_TIMER_LEFT,  "Pausa", C_YELLOW,  0x0000);
    ui.drawButton(BTN_TIMER_RIGHT, "Reset", C_SURFACE, C_WHITE);
  } else {
    ui.drawButton(BTN_TIMER_LEFT,  "Start", C_GREEN,   C_WHITE);
    ui.drawButton(BTN_TIMER_RIGHT, "Reset", C_SURFACE, C_WHITE);
  }
}

void screenMainDraw(UI &ui, const AppState &s, bool full) {
  if (full) ui.tft().fillScreen(C_BG);
  drawTopBar(ui, s);
  ui.updateTimer(s.timerRemaining);
  drawTimerButtons(ui, s);
  ui.drawDivider(194);
  ui.drawButton(BTN_COIN, "Lancia moneta", C_SURFACE, C_YELLOW);
  ui.drawButton(BTN_DICE, "Tira dado D6",  C_SURFACE, C_PURPLE);
}

void screenMainRedrawTopBar(UI &ui, const AppState &s) {
  drawTopBar(ui, s);
}

void screenMainRedrawButtons(UI &ui, const AppState &s) {
  drawTimerButtons(ui, s);
}

// ── Touch ─────────────────────────────────────────────────────────────────────

static char s_popupLabel[16];
static const char *s_coinBig[2]  = { "CROCE", "TESTA" };
static const char *s_diceFaces[] = { "", "1", "2", "3", "4", "5", "6" };

bool screenMainHandleTouch(UI &ui, AppState &s, Timer &gameTimer,
                           Point p,
                           const char **popupBig, const char **popupLabel,
                           uint16_t *popupColor) {
  // Win boxes
  if (WIN_J1A.contains(p)) { s.wins[0][0]=!s.wins[0][0]; screenMainRedrawTopBar(ui,s); return false; }
  if (WIN_J1B.contains(p)) { s.wins[0][1]=!s.wins[0][1]; screenMainRedrawTopBar(ui,s); return false; }
  if (WIN_J2A.contains(p)) { s.wins[1][0]=!s.wins[1][0]; screenMainRedrawTopBar(ui,s); return false; }
  if (WIN_J2B.contains(p)) { s.wins[1][1]=!s.wins[1][1]; screenMainRedrawTopBar(ui,s); return false; }

  // Timer area → vai alle impostazioni (solo se fermo)
  if (ZONE_TIMER.contains(p) && s.timerState == TimerState::IDLE) {
    s.pickM  = s.timerRemaining / 60;
    s.pickS  = s.timerRemaining % 60;
    s.screen = Screen::SETTINGS;
    return false;
  }

  if (s.timerState == TimerState::IDLE) {
    if (BTN_TIMER_FULL.contains(p)) {
      gameTimer.start();
      s.timerState = TimerState::RUNNING;
      screenMainRedrawButtons(ui, s);
    }
  } else if (s.timerState == TimerState::RUNNING) {
    if (BTN_TIMER_LEFT.contains(p)) {
      gameTimer.pause();
      s.timerState = TimerState::PAUSED;
      screenMainRedrawButtons(ui, s);
    }
    if (BTN_TIMER_RIGHT.contains(p)) {
      gameTimer.reset();
      s.timerState     = TimerState::IDLE;
      s.timerRemaining = gameTimer.remaining();
      ui.updateTimer(s.timerRemaining);
      screenMainRedrawButtons(ui, s);
    }
  } else {
    if (BTN_TIMER_LEFT.contains(p)) {
      gameTimer.start();
      s.timerState = TimerState::RUNNING;
      screenMainRedrawButtons(ui, s);
    }
    if (BTN_TIMER_RIGHT.contains(p)) {
      gameTimer.reset();
      s.timerState     = TimerState::IDLE;
      s.timerRemaining = gameTimer.remaining();
      ui.updateTimer(s.timerRemaining);
      screenMainRedrawButtons(ui, s);
    }
  }

  // Lancia moneta
  if (BTN_COIN.contains(p)) {
    *popupBig   = s_coinBig[random(2)];
    *popupColor = C_YELLOW;
    return true;
  }

  // Tira dado D6
  if (BTN_DICE.contains(p)) {
    int r = random(1, 7);
    snprintf(s_popupLabel, sizeof(s_popupLabel), "DADO · %d", r);
    *popupBig   = s_diceFaces[r];
    *popupLabel = s_popupLabel;
    *popupColor = C_PURPLE;
    return true;
  }

  return false;
}

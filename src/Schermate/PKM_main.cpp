#include "Schermate/PKM_main.h"
#include <Arduino.h>

static bool hit(int tx, int ty, int x, int y, int w, int h) {
  return tx >= x && tx <= x + w && ty >= y && ty <= y + h;
}

// ── TOP BAR ──────────────────────────────────────────────────────────────────────
static void drawTopBar(UI &ui, const AppState &s) {
  TFT_eSPI &tft = ui.tft();
  tft.fillRect(0, 0, 320, 36, C_BG);
  tft.drawFastHLine(0, 36, 320, C_BORDER);
  tft.setFreeFont(&FreeSans9pt7b);

  tft.setTextColor(C_GREEN, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("J1", 8, 18);
  ui.drawWinBox(28, 4, s.wins[0][0], C_GREEN);
  ui.drawWinBox(60, 4, s.wins[0][1], C_GREEN);

  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("BO3", 160, 18);

  ui.drawWinBox(232, 4, s.wins[1][0], C_BLUE);
  ui.drawWinBox(264, 4, s.wins[1][1], C_BLUE);
  tft.setTextColor(C_BLUE, C_BG);
  tft.setTextDatum(MR_DATUM);
  tft.drawString("J2", 312, 18);
}
// ── TIMER + BOTTONI ──────────────────────────────────────────────────────────────────────
static void drawTimerButtons(UI &ui, const AppState &s) {
  ui.tft().fillRect(10, 152, 300, 36, C_BG);
  if (s.timerState == TimerState::IDLE) {
    ui.drawButton(10, 152, 300, 36, "Start", C_GREEN, C_WHITE);
  } else if (s.timerState == TimerState::RUNNING) {
    ui.drawButton(10,  152, 145, 36, "Pausa", C_YELLOW, 0x0000);
    ui.drawButton(165, 152, 145, 36, "Reset", C_SURFACE, C_WHITE);
  } else {
    ui.drawButton(10,  152, 145, 36, "Start", C_GREEN,   C_WHITE);
    ui.drawButton(165, 152, 145, 36, "Reset", C_SURFACE, C_WHITE);
  }
}


void screenMainDraw(UI &ui, const AppState &s, bool full) {
  if (full) ui.tft().fillScreen(C_BG);
  drawTopBar(ui, s);
  ui.updateTimer(s.timerRemaining);
  drawTimerButtons(ui, s);
  ui.drawDivider(194);
  ui.drawButton(10,  198, 145, 36, "Lancia moneta", C_SURFACE, C_YELLOW);
  ui.drawButton(165, 198, 145, 36, "Tira dado D6",  C_SURFACE, C_PURPLE);
}

void screenMainRedrawTopBar(UI &ui, const AppState &s) {
  drawTopBar(ui, s);
}

void screenMainRedrawButtons(UI &ui, const AppState &s) {
  drawTimerButtons(ui, s);
}

// ── Touch ─────────────────────────────────────────────────────────────────────

// Stringhe statiche per i popup (vita fino al prossimo lancio)
static char s_popupLabel[16];
static const char *s_coinBig[2]  = { "CROCE", "TESTA" };
static const char *s_diceFaces[] = { "", "1", "2", "3", "4", "5", "6" };

bool screenMainHandleTouch(UI &ui, AppState &s, Timer &gameTimer,
                           int tx, int ty,
                           const char **popupBig, const char **popupLabel,
                           uint16_t *popupColor) {
  // Win boxes — ridisegna solo la top bar
  if (hit(tx,ty,28,4,28,28))  { s.wins[0][0]=!s.wins[0][0]; screenMainRedrawTopBar(ui,s); return false; }
  if (hit(tx,ty,60,4,28,28))  { s.wins[0][1]=!s.wins[0][1]; screenMainRedrawTopBar(ui,s); return false; }
  if (hit(tx,ty,232,4,28,28)) { s.wins[1][0]=!s.wins[1][0]; screenMainRedrawTopBar(ui,s); return false; }
  if (hit(tx,ty,264,4,28,28)) { s.wins[1][1]=!s.wins[1][1]; screenMainRedrawTopBar(ui,s); return false; }

  // Timer area → vai alle impostazioni (solo se fermo)
  if (hit(tx,ty,20,42,280,100) && s.timerState == TimerState::IDLE) {
    s.pickM  = s.timerRemaining / 60;
    s.pickS  = s.timerRemaining % 60;
    s.screen = Screen::SETTINGS;
    return false;
  }


  if (s.timerState == TimerState::IDLE) {                  // --- TIMER IN IDLE ----------------------------
    if (hit(tx,ty,10,152,300,36)) {                                                 // --- PREMUTO START ---
      gameTimer.start();
      s.timerState = TimerState::RUNNING;
      screenMainRedrawButtons(ui, s);
    }                                                      //-----------------------------------------------
  } else if (s.timerState == TimerState::RUNNING) {        // --- TIMER RUNNING ----------------------------
    if (hit(tx,ty,10,152,145,36)) {                                                 // --- PREMUTO PAUSA ---
      gameTimer.pause();
      s.timerState = TimerState::PAUSED;
      screenMainRedrawButtons(ui, s);
    }                                                                               // ---------------------
    if (hit(tx,ty,165,152,145,36)) {                                                // --- PREMUTO RESET ---
      gameTimer.reset();
      s.timerState     = TimerState::IDLE;
      s.timerRemaining = gameTimer.remaining();
      ui.updateTimer(s.timerRemaining);
      screenMainRedrawButtons(ui, s);
    }                                                                               // ---------------------
  } else {                                                 // --- TIMER IN PASUA ---------------------------
    if (hit(tx,ty,10,152,145,36)) {                                                 // --- PREMUTO RESUME --
      gameTimer.start();                      
      s.timerState = TimerState::RUNNING;
      screenMainRedrawButtons(ui, s);
    }                                                                               // ---------------------
    if (hit(tx,ty,165,152,145,36)) {                                                // --- PREMUTO RESET ---
      gameTimer.reset();
      s.timerState     = TimerState::IDLE;
      s.timerRemaining = gameTimer.remaining();
      ui.updateTimer(s.timerRemaining);
      screenMainRedrawButtons(ui, s);
    }                                                                               // ---------------------
  }                                                        // ----------------------------------------------

  // Lancia moneta
  if (hit(tx,ty,10,198,145,36)) {                                       // --- PREMUTO TASTO PER LANCIARE LA MONETA
    int h = random(2);
    *popupBig   = s_coinBig[h];
    *popupColor = C_YELLOW;
    return true;
  }

  // Tira dado D6
  if (hit(tx,ty,165,198,145,36)) {
    int r = random(1, 7);
    snprintf(s_popupLabel, sizeof(s_popupLabel), "DADO · %d", r);
    *popupBig   = s_diceFaces[r];
    *popupLabel = s_popupLabel;
    *popupColor = C_PURPLE;
    return true;
  }

  return false;
}
#include "ui.h"

void UI::begin() {
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  _tft.init();
  _tft.setRotation(1);
  _tft.fillScreen(C_BG);
}

void UI::drawButton(int x, int y, int w, int h, const char *label,
                    uint16_t bg, uint16_t fg, int radius) {
  _tft.fillRoundRect(x, y, w, h, radius, bg);
  _tft.drawRoundRect(x, y, w, h, radius, C_BORDER);
  _tft.setTextColor(fg, bg);
  _tft.setTextDatum(MC_DATUM);
  _tft.setTextSize(2);
  _tft.drawString(label, x + w / 2, y + h / 2);
}

void UI::drawWinBox(int x, int y, bool filled, uint16_t color) {
  if (filled) {
    _tft.fillRoundRect(x, y, 28, 28, 6, color);
  } else {
    _tft.fillRoundRect(x, y, 28, 28, 6, C_SURFACE);
    _tft.drawRoundRect(x, y, 28, 28, 6, C_BORDER);
  }
}

void UI::drawDivider(int y) {
  _tft.drawFastHLine(10, y, 300, C_BORDER);
}

void UI::drawTopBar(const AppState &s) {
  _tft.fillRect(0, 0, 320, 36, C_BG);
  _tft.drawFastHLine(0, 36, 320, C_BORDER);
  _tft.setTextSize(1);

  _tft.setTextColor(C_GREEN, C_BG);
  _tft.setTextDatum(ML_DATUM);
  _tft.drawString("J1", 8, 18);
  drawWinBox(28, 4, s.wins[0][0], C_GREEN);
  drawWinBox(60, 4, s.wins[0][1], C_GREEN);

  _tft.setTextColor(C_GRAY, C_BG);
  _tft.setTextDatum(MC_DATUM);
  _tft.drawString("BO3", 160, 18);

  drawWinBox(232, 4, s.wins[1][0], C_BLUE);
  drawWinBox(264, 4, s.wins[1][1], C_BLUE);
  _tft.setTextColor(C_BLUE, C_BG);
  _tft.setTextDatum(MR_DATUM);
  _tft.drawString("J2", 312, 18);
}

void UI::drawTimerDigits(uint32_t sec, uint16_t color) {
  char buf[6];
  snprintf(buf, 6, "%02lu:%02lu", sec / 60, sec % 60);
  _tft.fillRect(20, 42, 280, 100, C_BG);
  _tft.setTextColor(color, C_BG);
  _tft.setTextDatum(MC_DATUM);
  _tft.setFreeFont(&FreeSans24pt7b);
  _tft.drawString(buf, 160, 100);
  _tft.setFreeFont(nullptr);
}

void UI::updateTimer(uint32_t remaining) {
  uint16_t color = C_WHITE;
  if (remaining <= 300)      color = C_RED;
  else if (remaining <= 600) color = C_YELLOW;
  drawTimerDigits(remaining, color);
}

void UI::redrawTopBar(const AppState &s) {
  drawTopBar(s);
}

void UI::redrawTimerButtons(const AppState &s) {
  _tft.fillRect(10, 152, 300, 36, C_BG);
  if (s.timerState == TimerState::IDLE) {
    drawButton(10, 152, 300, 36, "Start", C_GREEN, C_WHITE);
  } else if (s.timerState == TimerState::RUNNING) {
    drawButton(10,  152, 145, 36, "Pausa",  C_YELLOW, 0x0000);
    drawButton(165, 152, 145, 36, "Reset",  C_SURFACE, C_WHITE);
  } else {
    drawButton(10,  152, 145, 36, "Start",  C_GREEN, C_WHITE);
    drawButton(165, 152, 145, 36, "Reset",  C_SURFACE, C_WHITE);
  }
}

void UI::drawMain(const AppState &s, bool full) {
  if (full) _tft.fillScreen(C_BG);
  drawTopBar(s);
  updateTimer(s.timerRemaining);
  redrawTimerButtons(s);
  drawDivider(194);
  drawButton(10,  198, 145, 36, "Lancia moneta", C_SURFACE, C_YELLOW);
  drawButton(165, 198, 145, 36, "Tira dado D6",  C_SURFACE, C_PURPLE);
}

void UI::drawSettings(const AppState &s, bool full) {
  if (full) _tft.fillScreen(C_BG);

  _tft.fillRect(0, 0, 320, 36, C_BG);
  _tft.drawFastHLine(0, 36, 320, C_BORDER);
  _tft.setTextSize(1);
  _tft.setTextColor(C_BLUE, C_BG);
  _tft.setTextDatum(ML_DATUM);
  _tft.drawString("< Indietro", 8, 18);
  _tft.setTextColor(C_WHITE, C_BG);
  _tft.setTextDatum(MC_DATUM);
  _tft.drawString("Durata partita", 160, 18);

  char buf[4];
  snprintf(buf, 4, "%02d", s.pickM);
  drawButton(50, 50, 30, 30, "+", C_SURFACE, C_WHITE, 15);
  drawButton(100, 50, 30, 30, "-", C_SURFACE, C_WHITE, 15);
  _tft.setFreeFont(&FreeSans24pt7b);
  _tft.setTextColor(C_WHITE, C_BG);
  _tft.setTextDatum(MC_DATUM);
  _tft.fillRect(50, 86, 80, 50, C_BG);
  _tft.drawString(buf, 90, 115);
  _tft.setFreeFont(nullptr);
  _tft.setTextColor(C_GRAY, C_BG);
  _tft.setTextSize(1);
  _tft.drawString("min", 90, 145);

  _tft.setFreeFont(&FreeSans24pt7b);
  _tft.setTextColor(C_GRAY, C_BG);
  _tft.setTextDatum(MC_DATUM);
  _tft.drawString(":", 160, 115);
  _tft.setFreeFont(nullptr);

  snprintf(buf, 4, "%02d", s.pickS);
  drawButton(180, 50, 30, 30, "+", C_SURFACE, C_WHITE, 15);
  drawButton(220, 50, 30, 30, "-", C_SURFACE, C_WHITE, 15);
  _tft.setFreeFont(&FreeSans24pt7b);
  _tft.setTextColor(C_WHITE, C_BG);
  _tft.fillRect(170, 86, 80, 50, C_BG);
  _tft.drawString(buf, 210, 115);
  _tft.setFreeFont(nullptr);
  _tft.setTextColor(C_GRAY, C_BG);
  _tft.setTextSize(1);
  _tft.drawString("sec", 210, 145);

  drawButton(95, 158, 130, 36, "Salva", C_GREEN, C_WHITE);

  drawDivider(198);
  drawButton(70, 202, 180, 34, "Calibra touch", C_SURFACE, C_WHITE);
}

void UI::showPopup(const char *big, const char *label, uint16_t color) {
  _tft.fillRoundRect(85, 70, 150, 100, 20, C_SURFACE);
  _tft.drawRoundRect(85, 70, 150, 100, 20, C_BORDER);
  _tft.setTextColor(color, C_SURFACE);
  _tft.setTextDatum(MC_DATUM);
  _tft.setFreeFont(&FreeSans24pt7b);
  _tft.drawString(big, 160, 110);
  _tft.setFreeFont(nullptr);
  _tft.setTextColor(C_GRAY, C_SURFACE);
  _tft.setTextSize(1);
  _tft.drawString(label, 160, 150);
  _popupVisible = true;
}

void UI::hidePopup() {
  if (!_popupVisible) return;
  _popupVisible = false;
}

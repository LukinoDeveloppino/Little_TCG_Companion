#include "inc/ui.h"

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
  _tft.setFreeFont(&FreeSans9pt7b);
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

void UI::drawTimerDigits(uint32_t sec, uint16_t color) {
  char buf[6];
  snprintf(buf, 6, "%02lu:%02lu", sec / 60, sec % 60);
  _tft.fillRect(20, 42, 280, 100, C_BG);
  _tft.setTextColor(color, C_BG);
  _tft.setTextDatum(MC_DATUM);
  _tft.setFreeFont(&FreeSansBold24pt7b);
  _tft.drawString(buf, 160, 100);
}

void UI::updateTimer(uint32_t remaining) {
  uint16_t color = C_WHITE;
  if (remaining <= 300)      color = C_RED;
  else if (remaining <= 600) color = C_YELLOW;
  drawTimerDigits(remaining, color);
}

void UI::showPopup(const char *big, uint16_t color) {
  _tft.fillRoundRect(85, 70, 150, 100, 20, C_SURFACE);
  _tft.drawRoundRect(85, 70, 150, 100, 20, C_BORDER);
  _tft.setTextColor(color, C_SURFACE);
  _tft.setTextDatum(MC_DATUM);
  _tft.setFreeFont(&FreeSansBold18pt7b);
  _tft.drawString(big, 160, 120);
  _popupVisible = true;
}

void UI::hidePopup() {
  if (!_popupVisible) return;
  _popupVisible = false;
}
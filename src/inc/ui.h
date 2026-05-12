#pragma once
#include <TFT_eSPI.h>
#include "app_state.h"
#include "touch.h"

class UI {
public:
  void begin();

  // ── Primitivi grafici ─────────────────────────────────────────────────────
  void drawButton(const Rect &r, const char *label,
                  uint16_t bg, uint16_t fg, int radius = 10);
  void drawWinBox(const Rect &r, bool filled, uint16_t color);
  void drawDivider(int y);

  // ── Timer display ─────────────────────────────────────────────────────────
  void updateTimer(uint32_t remaining);

  // ── Popup ─────────────────────────────────────────────────────────────────
  void showPopup(const char *big, uint16_t color);
  void hidePopup();
  bool isPopupVisible() const { return _popupVisible; }

  // ── Accesso diretto al TFT (usato da screen_* e calibrazione) ────────────
  TFT_eSPI &tft() { return _tft; }

private:
  TFT_eSPI _tft;
  bool     _popupVisible = false;

  void drawTimerDigits(uint32_t sec, uint16_t color);
};
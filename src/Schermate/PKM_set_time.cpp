#include "Schermate/PKM_set_time.h"
#include "Schermate/PKM_main.h"
#include "inc/touch.h"
#include <Preferences.h>
#include <Arduino.h>

// ── Layout ────────────────────────────────────────────────────────────────────
// Minuti centrati a x=90, Secondi centrati a x=230 (simmetrici rispetto a x=160)
static const Rect BTN_BACK      = {   0,   0,  90,  36 };
static const Rect BTN_MIN_PLUS  = {  43,  44,  44,  44 };
static const Rect BTN_MIN_MINUS = {  93,  44,  44,  44 };
static const Rect BTN_SEC_PLUS  = { 183,  44,  44,  44 };
static const Rect BTN_SEC_MINUS = { 233,  44,  44,  44 };
static const Rect BTN_SALVA     = {  95, 162, 130,  36 };
static const Rect BTN_CALIBRA   = {  70, 206, 180,  30 };

// ── Draw ──────────────────────────────────────────────────────────────────────

void screenSettingsDraw(UI &ui, const AppState &s, bool full) {
  TFT_eSPI &tft = ui.tft();
  if (full) tft.fillScreen(C_BG);

  // Header
  tft.fillRect(0, 0, 320, 36, C_BG);
  tft.drawFastHLine(0, 36, 320, C_BORDER);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(C_BLUE, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("< Indietro", 8, 18);
  tft.setTextColor(C_WHITE, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Durata partita", 160, 18);

  char buf[4];

  // Minuti
  snprintf(buf, 4, "%02d", s.pickM);
  ui.drawButton(BTN_MIN_PLUS,  "+", C_SURFACE, C_WHITE, 22);
  ui.drawButton(BTN_MIN_MINUS, "-", C_SURFACE, C_WHITE, 22);
  tft.fillRect(43, 92, 94, 44, C_BG);
  tft.setFreeFont(&FreeSans18pt7b);
  tft.setTextColor(C_WHITE, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(buf, 90, 116);

  // Separatore ":"
  tft.setTextColor(C_GRAY, C_BG);
  tft.drawString(":", 160, 116);

  // Secondi — setFreeFont obbligatorio: drawButton l'ha resettato a 9pt
  snprintf(buf, 4, "%02d", s.pickS);
  ui.drawButton(BTN_SEC_PLUS,  "+", C_SURFACE, C_WHITE, 22);
  ui.drawButton(BTN_SEC_MINUS, "-", C_SURFACE, C_WHITE, 22);
  tft.fillRect(183, 92, 94, 44, C_BG);
  tft.setFreeFont(&FreeSans18pt7b);
  tft.setTextColor(C_WHITE, C_BG);
  tft.drawString(buf, 230, 116);

  // Etichette unità
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("min",  90, 148);
  tft.drawString("sec", 230, 148);

  ui.drawButton(BTN_SALVA,   "Salva",         C_GREEN,   C_WHITE);
  ui.drawDivider(202);
  ui.drawButton(BTN_CALIBRA, "Calibra touch", C_SURFACE, C_WHITE);
}

// ── Calibrazione ─────────────────────────────────────────────────────────────

static void drawCrosshair(TFT_eSPI &tft, int x, int y) {
  tft.drawLine(x - 20, y, x + 20, y, TFT_RED);
  tft.drawLine(x, y - 20, x, y + 20, TFT_RED);
  tft.fillCircle(x, y, 4, TFT_RED);
}

static bool calWaitTouch(uint16_t &outRx, uint16_t &outRy) {
  uint32_t deadline = millis() + 15000;
  while (digitalRead(TP_IRQ) != LOW) {
    if (millis() > deadline) return false;
    delay(10);
  }
  delay(60);

  uint32_t sx = 0, sy = 0;
  int n = 0;
  while (digitalRead(TP_IRQ) == LOW) {
    uint16_t rx, ry;
    if (touchRead(&rx, &ry)) { sx += rx; sy += ry; n++; }
    delay(20);
  }
  delay(100);

  if (n < 3) return false;
  outRx = sx / n;
  outRy = sy / n;
  Serial.printf("[CAL] raw rx=%u ry=%u (n=%d)\n", outRx, outRy, n);
  return true;
}

void runCalibration(UI &ui) {
  TFT_eSPI &tft = ui.tft();

  static const struct { int sx, sy; const char *label; } T[5] = {
    {  20,  20, "Alto SX"  },
    { 300,  20, "Alto DX"  },
    { 300, 220, "Basso DX" },
    {  20, 220, "Basso SX" },
    { 160, 120, "Centro"   },
  };
  uint16_t rx[5] = {}, ry[5] = {};

  for (int i = 0; i < 5; ) {
    tft.fillScreen(C_BG);
    tft.setTextColor(C_WHITE, C_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString("Calibrazione touch", 160, 110);
    char buf[24];
    snprintf(buf, 24, "%d/5  %s", i + 1, T[i].label);
    tft.drawString(buf, 160, 135);
    drawCrosshair(tft, T[i].sx, T[i].sy);

    if (!calWaitTouch(rx[i], ry[i])) {
      tft.setTextColor(C_RED, C_BG);
      tft.drawString("Tieni il dito fermo - riprova", 160, 160);
      delay(1500);
      continue;
    }
    tft.fillCircle(T[i].sx, T[i].sy, 10, C_GREEN);
    delay(400);
    i++;
  }

  int dRx_h = abs((int)rx[1] - (int)rx[0]);
  int dRy_h = abs((int)ry[1] - (int)ry[0]);
  bool swapAxes = dRy_h > dRx_h;
  Serial.printf("[CAL] dRx_h=%d dRy_h=%d swap=%d\n", dRx_h, dRy_h, swapAxes);

  uint16_t *xS = swapAxes ? ry : rx;
  uint16_t *yS = swapAxes ? rx : ry;

  float leftRaw   = ((int)xS[0] + (int)xS[3]) / 2.0f;
  float rightRaw  = ((int)xS[1] + (int)xS[2]) / 2.0f;
  float topRaw    = ((int)yS[0] + (int)yS[1]) / 2.0f;
  float bottomRaw = ((int)yS[2] + (int)yS[3]) / 2.0f;

  float kx = (rightRaw - leftRaw)  / (300 - 20);
  float ky = (bottomRaw - topRaw)  / (220 - 20);

  if (fabsf(kx) < 0.5f || fabsf(ky) < 0.5f) {
    tft.fillScreen(C_BG);
    tft.setTextColor(C_RED, C_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString("Dati non validi - riprova", 160, 120);
    Serial.printf("[CAL] INVALID kx=%.2f ky=%.2f\n", kx, ky);
    delay(2000);
    runCalibration(ui);
    return;
  }

  int xMin = (int)(leftRaw  - 20 * kx);
  int xMax = (int)(xMin + 320 * kx);
  int yMin = (int)(topRaw   - 20 * ky);
  int yMax = (int)(yMin + 240 * ky);

  float cx = (xS[4] - xMin) * 320.0f / (xMax - xMin);
  float cy = (yS[4] - yMin) * 240.0f / (yMax - yMin);
  Serial.printf("[CAL] swap=%d xMin=%d xMax=%d yMin=%d yMax=%d\n", swapAxes, xMin, xMax, yMin, yMax);
  Serial.printf("[CAL] center: got(%.0f,%.0f) expected(160,120) err(%.0f,%.0f)\n",
                cx, cy, cx - 160.0f, cy - 120.0f);

  touchSetCalibration(xMin, xMax, yMin, yMax, swapAxes);

  Preferences prefs;
  prefs.begin("touch_cal", false);
  prefs.putInt("xMin", xMin);  prefs.putInt("xMax", xMax);
  prefs.putInt("yMin", yMin);  prefs.putInt("yMax", yMax);
  prefs.putBool("swap", swapAxes);
  prefs.putInt("ver",  CAL_VER);
  prefs.end();

  tft.fillScreen(C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_GREEN, C_BG);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.drawString("Calibrazione OK!", 160, 112);
  tft.setTextColor(C_GRAY, C_BG);
  char buf[32];
  snprintf(buf, 32, "errore centro: X%+.0f  Y%+.0f px", cx - 160.0f, cy - 120.0f);
  tft.drawString(buf, 160, 140);
  delay(2500);
}

// ── Touch ─────────────────────────────────────────────────────────────────────

void screenSettingsHandleTouch(UI &ui, AppState &s, Timer &gameTimer, Point p) {
  if (BTN_BACK.contains(p)) {
    s.screen = Screen::MAIN;
    screenMainDraw(ui, s, true);
    return;
  }

  if (BTN_MIN_PLUS.contains(p))  { s.pickM = min(99, s.pickM+1);  screenSettingsDraw(ui,s,false); return; }
  if (BTN_MIN_MINUS.contains(p)) { s.pickM = max(0,  s.pickM-1);  screenSettingsDraw(ui,s,false); return; }
  if (BTN_SEC_PLUS.contains(p))  { s.pickS = min(50, s.pickS+10); screenSettingsDraw(ui,s,false); return; }
  if (BTN_SEC_MINUS.contains(p)) { s.pickS = max(0,  s.pickS-10); screenSettingsDraw(ui,s,false); return; }

  if (BTN_SALVA.contains(p)) {
    uint32_t newSec  = s.pickM * 60 + s.pickS;
    s.timerRemaining = newSec;
    gameTimer.begin(newSec);
    s.screen = Screen::MAIN;
    screenMainDraw(ui, s, true);
    return;
  }

  if (BTN_CALIBRA.contains(p)) {
    runCalibration(ui);
    screenSettingsDraw(ui, s, true);
  }
}

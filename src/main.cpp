#include <Arduino.h>
#include <Preferences.h>
#include "ui.h"
#include "timer.h"
#include "touch.h"

UI       ui;
Timer    gameTimer;
AppState state;

uint32_t popupHideAt = 0;
bool     popupActive = false;

// ── Calibration ───────────────────────────────────────────────────────────────

static const int CAL_VER = 4;

static void drawCrosshair(TFT_eSPI &tft, int x, int y) {
  tft.drawLine(x - 20, y, x + 20, y, TFT_RED);
  tft.drawLine(x, y - 20, x, y + 20, TFT_RED);
  tft.fillCircle(x, y, 4, TFT_RED);
}

// Reads while the finger is held down; returns false if lifted too fast or timeout.
static bool calWaitTouch(uint16_t &outRx, uint16_t &outRy) {
  uint32_t deadline = millis() + 15000;
  while (digitalRead(TP_IRQ) != LOW) {
    if (millis() > deadline) return false;
    delay(10);
  }
  delay(60); // debounce

  uint32_t sx = 0, sy = 0;
  int n = 0;
  while (digitalRead(TP_IRQ) == LOW) {
    uint16_t rx, ry;
    if (touchRead(&rx, &ry)) { sx += rx; sy += ry; n++; }
    delay(20);
  }
  delay(100); // clean release

  if (n < 3) return false; // finger lifted too fast
  outRx = sx / n;
  outRy = sy / n;
  Serial.printf("[CAL] raw rx=%u ry=%u (n=%d)\n", outRx, outRy, n);
  return true;
}

static void runCalibration() {
  TFT_eSPI &tft = ui.tft();

  // 5 points: 4 corners (clockwise from TL) + center
  // TL(0)→TR(1) = horizontal move  → axis detection
  // TL(0)+BL(3) = left edge average, TR(1)+BR(2) = right edge average → X calibration
  // TL(0)+TR(1) = top edge average,  BR(2)+BL(3) = bottom edge average → Y calibration
  // C(4) = independent verification
  static const struct { int sx, sy; const char *label; } T[5] = {
    { 20,  20,  "Alto SX"  },
    {300,  20,  "Alto DX"  },
    {300, 220,  "Basso DX" },
    { 20, 220,  "Basso SX" },
    {160, 120,  "Centro"   },
  };
  uint16_t rx[5] = {}, ry[5] = {};

  for (int i = 0; i < 5; ) {
    tft.fillScreen(C_BG);
    tft.setTextColor(C_WHITE, C_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Calibrazione touch", 160, 110);
    tft.setTextSize(1);
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

  // TL→TR: horizontal move → whichever sensor axis varies more IS screen X
  int dRx_h = abs((int)rx[1] - (int)rx[0]);
  int dRy_h = abs((int)ry[1] - (int)ry[0]);
  bool swapAxes = dRy_h > dRx_h;
  Serial.printf("[CAL] dRx_h=%d dRy_h=%d swap=%d\n", dRx_h, dRy_h, swapAxes);

  uint16_t *xS = swapAxes ? ry : rx;
  uint16_t *yS = swapAxes ? rx : ry;

  // Average opposite corners for each edge → more robust than a single point
  float leftRaw   = ((int)xS[0] + (int)xS[3]) / 2.0f;  // TL + BL  → screen x=20
  float rightRaw  = ((int)xS[1] + (int)xS[2]) / 2.0f;  // TR + BR  → screen x=300
  float topRaw    = ((int)yS[0] + (int)yS[1]) / 2.0f;  // TL + TR  → screen y=20
  float bottomRaw = ((int)yS[2] + (int)yS[3]) / 2.0f;  // BR + BL  → screen y=220

  float kx = (rightRaw - leftRaw)  / (300 - 20);
  float ky = (bottomRaw - topRaw)  / (220 - 20);

  if (fabsf(kx) < 0.5f || fabsf(ky) < 0.5f) {
    tft.fillScreen(C_BG);
    tft.setTextColor(C_RED, C_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("Dati non validi - riprova", 160, 120);
    Serial.printf("[CAL] INVALID kx=%.2f ky=%.2f\n", kx, ky);
    delay(2000);
    runCalibration();
    return;
  }

  int xMin = (int)(leftRaw  - 20 * kx);
  int xMax = (int)(xMin + 320 * kx);
  int yMin = (int)(topRaw   - 20 * ky);
  int yMax = (int)(yMin + 240 * ky);

  // Center verification (independent point, not used in fit)
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
  tft.setTextColor(C_GREEN, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("Calibrazione OK!", 160, 112);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_BG);
  char buf[32];
  snprintf(buf, 32, "errore centro: X%+.0f  Y%+.0f px", cx - 160.0f, cy - 120.0f);
  tft.drawString(buf, 160, 140);
  delay(2500);
}

// ── Hit test ──────────────────────────────────────────────────────────────────

bool hit(int tx, int ty, int x, int y, int w, int h) {
  return tx >= x && tx <= x + w && ty >= y && ty <= y + h;
}

void handleMain(int tx, int ty) {
  // Win boxes: only top bar needs updating
  if (hit(tx,ty,28,4,28,28))  { state.wins[0][0]=!state.wins[0][0]; ui.redrawTopBar(state); return; }
  if (hit(tx,ty,60,4,28,28))  { state.wins[0][1]=!state.wins[0][1]; ui.redrawTopBar(state); return; }
  if (hit(tx,ty,232,4,28,28)) { state.wins[1][0]=!state.wins[1][0]; ui.redrawTopBar(state); return; }
  if (hit(tx,ty,264,4,28,28)) { state.wins[1][1]=!state.wins[1][1]; ui.redrawTopBar(state); return; }

  // Tap on timer area → settings (full screen change)
  if (hit(tx,ty,20,42,280,100) && state.timerState == TimerState::IDLE) {
    state.pickM = state.timerRemaining / 60;
    state.pickS = state.timerRemaining % 60;
    state.screen = Screen::SETTINGS;
    ui.drawSettings(state, true);
    return;
  }

  // Timer buttons: only the button row needs updating
  if (state.timerState == TimerState::IDLE) {
    if (hit(tx,ty,10,152,300,36)) {
      gameTimer.start();
      state.timerState = TimerState::RUNNING;
      ui.redrawTimerButtons(state);
    }
  } else if (state.timerState == TimerState::RUNNING) {
    if (hit(tx,ty,10,152,145,36)) {
      gameTimer.pause();
      state.timerState = TimerState::PAUSED;
      ui.redrawTimerButtons(state);
    }
    if (hit(tx,ty,165,152,145,36)) {
      gameTimer.reset();
      state.timerState = TimerState::IDLE;
      state.timerRemaining = gameTimer.remaining();
      ui.updateTimer(state.timerRemaining);
      ui.redrawTimerButtons(state);
    }
  } else { // PAUSED
    if (hit(tx,ty,10,152,145,36)) {
      gameTimer.start();
      state.timerState = TimerState::RUNNING;
      ui.redrawTimerButtons(state);
    }
    if (hit(tx,ty,165,152,145,36)) {
      gameTimer.reset();
      state.timerState = TimerState::IDLE;
      state.timerRemaining = gameTimer.remaining();
      ui.updateTimer(state.timerRemaining);
      ui.redrawTimerButtons(state);
    }
  }

  if (hit(tx,ty,10,198,145,36)) {
    bool h = random(2);
    ui.showPopup(h ? "T" : "C", h ? "TESTA" : "CROCE", C_YELLOW);
    popupHideAt = millis() + 2200; popupActive = true;
    return;
  }
  if (hit(tx,ty,165,198,145,36)) {
    int r = random(1, 7);
    const char *faces[] = {"","1","2","3","4","5","6"};
    char label[12]; snprintf(label, 12, "DADO · %d", r);
    ui.showPopup(faces[r], label, C_PURPLE);
    popupHideAt = millis() + 2200; popupActive = true;
    return;
  }
}

void handleSettings(int tx, int ty) {
  if (hit(tx,ty,0,0,90,36)) { state.screen = Screen::MAIN; ui.drawMain(state,true); return; }
  if (hit(tx,ty,50,50,30,30))  { state.pickM = min(99, state.pickM+1);  ui.drawSettings(state,false); return; }
  if (hit(tx,ty,100,50,30,30)) { state.pickM = max(0,  state.pickM-1);  ui.drawSettings(state,false); return; }
  if (hit(tx,ty,180,50,30,30)) { state.pickS = min(50, state.pickS+10); ui.drawSettings(state,false); return; }
  if (hit(tx,ty,220,50,30,30)) { state.pickS = max(0,  state.pickS-10); ui.drawSettings(state,false); return; }
  if (hit(tx,ty,95,158,130,36)) {
    uint32_t newSec = state.pickM * 60 + state.pickS;
    state.timerCustom = newSec;
    gameTimer.begin(newSec);
    state.timerRemaining = newSec;
    state.screen = Screen::MAIN;
    ui.drawMain(state, true);
    return;
  }
  if (hit(tx,ty,70,202,180,34)) {
    runCalibration();
    ui.drawSettings(state, true);
  }
}

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
    int xMin = prefs.getInt("xMin"), xMax = prefs.getInt("xMax");
    int yMin = prefs.getInt("yMin"), yMax = prefs.getInt("yMax");
    bool sw  = prefs.getBool("swap", false);
    touchSetCalibration(xMin, xMax, yMin, yMax, sw);
    Serial.printf("[CAL] loaded swap=%d xMin=%d xMax=%d yMin=%d yMax=%d\n",
                  sw, xMin, xMax, yMin, yMax);
  }
  prefs.end();
  if (!validCal) runCalibration();

  ui.drawMain(state, true);
}

void loop() {
  gameTimer.update();
  if (state.timerState == TimerState::RUNNING) {
    uint32_t r = gameTimer.remaining();
    if (r != state.timerRemaining) {
      state.timerRemaining = r;
      ui.updateTimer(r);
      if (r == 0) {
        state.timerState = TimerState::IDLE;
        ui.redrawTimerButtons(state);
      }
    }
  }

  if (popupActive && millis() >= popupHideAt) {
    popupActive = false;
    ui.hidePopup();
    ui.updateTimer(state.timerRemaining); // restore only the area the popup covered
  }

  Point p;
  if (touchReadMapped(p)) {
    if (state.screen == Screen::MAIN) handleMain(p.x, p.y);
    else                               handleSettings(p.x, p.y);
    while (digitalRead(TP_IRQ) == LOW) delay(5); // wait for finger release
    delay(120);                                    // debounce
  }
}

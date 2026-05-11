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

static void drawCrosshair(TFT_eSPI &tft, int x, int y) {
  tft.drawLine(x - 20, y, x + 20, y, TFT_RED);
  tft.drawLine(x, y - 20, x, y + 20, TFT_RED);
  tft.fillCircle(x, y, 4, TFT_RED);
}

static void calWaitRelease() {
  while (digitalRead(TP_IRQ) == LOW) delay(10);
  delay(100);
}

static void calWaitTouch(uint16_t &outRx, uint16_t &outRy) {
  while (digitalRead(TP_IRQ) != LOW) delay(10);
  delay(50);
  uint32_t sx = 0, sy = 0; int n = 0;
  for (int i = 0; i < 8; i++) {
    uint16_t rx, ry;
    if (touchRead(&rx, &ry)) { sx += rx; sy += ry; n++; }
    delay(20);
  }
  if (n > 0) { outRx = sx / n; outRy = sy / n; }
}

static void runCalibration() {
  TFT_eSPI &tft = ui.tft();
  struct { int sx, sy; uint16_t rx, ry; } pts[2] = {
    {20,  20,  0, 0},
    {300, 220, 0, 0}
  };

  for (int i = 0; i < 2; i++) {
    tft.fillScreen(C_BG);
    tft.setTextColor(C_WHITE, C_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Calibrazione touch", 160, 100);
    tft.setTextSize(1);
    char buf[20];
    snprintf(buf, 20, "Tocca il punto %d/2", i + 1);
    tft.drawString(buf, 160, 125);
    drawCrosshair(tft, pts[i].sx, pts[i].sy);

    calWaitTouch(pts[i].rx, pts[i].ry);
    tft.fillCircle(pts[i].sx, pts[i].sy, 10, C_GREEN);
    delay(400);
    calWaitRelease();
  }

  // Extrapolate xMin/xMax so that map(rx, xMin, xMax, 0, 320) = sx
  float kx = (float)(pts[1].rx - pts[0].rx) / (pts[1].sx - pts[0].sx);
  int xMin = (int)(pts[0].rx - pts[0].sx * kx);
  int xMax = (int)(xMin + 320 * kx);

  float ky = (float)(pts[1].ry - pts[0].ry) / (pts[1].sy - pts[0].sy);
  int yMin = (int)(pts[0].ry - pts[0].sy * ky);
  int yMax = (int)(yMin + 240 * ky);

  touchSetCalibration(xMin, xMax, yMin, yMax);

  Preferences prefs;
  prefs.begin("touch_cal", false);
  prefs.putInt("xMin", xMin); prefs.putInt("xMax", xMax);
  prefs.putInt("yMin", yMin); prefs.putInt("yMax", yMax);
  prefs.end();

  tft.fillScreen(C_BG);
  tft.setTextColor(C_GREEN, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("Calibrazione OK!", 160, 120);
  delay(1200);
}

// ── Hit test ──────────────────────────────────────────────────────────────────

bool hit(int tx, int ty, int x, int y, int w, int h) {
  return tx >= x && tx <= x + w && ty >= y && ty <= y + h;
}

void handleMain(int tx, int ty) {
  if (hit(tx,ty,28,4,28,28))  { state.wins[0][0]=!state.wins[0][0]; ui.drawMain(state,true); return; }
  if (hit(tx,ty,60,4,28,28))  { state.wins[0][1]=!state.wins[0][1]; ui.drawMain(state,true); return; }
  if (hit(tx,ty,232,4,28,28)) { state.wins[1][0]=!state.wins[1][0]; ui.drawMain(state,true); return; }
  if (hit(tx,ty,264,4,28,28)) { state.wins[1][1]=!state.wins[1][1]; ui.drawMain(state,true); return; }

  if (hit(tx,ty,20,42,280,100) && state.timerState == TimerState::IDLE) {
    state.pickM = state.timerRemaining / 60;
    state.pickS = state.timerRemaining % 60;
    state.screen = Screen::SETTINGS;
    ui.drawSettings(state, true);
    return;
  }

  if (state.timerState == TimerState::IDLE) {
    if (hit(tx,ty,10,152,300,36)) {
      gameTimer.start();
      state.timerState = TimerState::RUNNING;
      ui.drawMain(state, true);
    }
  } else if (state.timerState == TimerState::RUNNING) {
    if (hit(tx,ty,10,152,145,36))  { gameTimer.pause(); state.timerState = TimerState::PAUSED; ui.drawMain(state,true); }
    if (hit(tx,ty,165,152,145,36)) { gameTimer.reset(); state.timerState = TimerState::IDLE;
                                      state.timerRemaining = gameTimer.remaining(); ui.drawMain(state,true); }
  } else {
    if (hit(tx,ty,10,152,145,36))  { gameTimer.start(); state.timerState = TimerState::RUNNING; ui.drawMain(state,true); }
    if (hit(tx,ty,165,152,145,36)) { gameTimer.reset(); state.timerState = TimerState::IDLE;
                                      state.timerRemaining = gameTimer.remaining(); ui.drawMain(state,true); }
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
  if (prefs.isKey("xMin")) {
    touchSetCalibration(
      prefs.getInt("xMin", 200), prefs.getInt("xMax", 3800),
      prefs.getInt("yMin", 200), prefs.getInt("yMax", 3800)
    );
    prefs.end();
  } else {
    prefs.end();
    runCalibration();
  }

  ui.drawMain(state, true);
}

void loop() {
  gameTimer.update();
  if (state.timerState == TimerState::RUNNING) {
    uint32_t r = gameTimer.remaining();
    if (r != state.timerRemaining) {
      state.timerRemaining = r;
      ui.updateTimer(r);
      if (r == 0) { state.timerState = TimerState::IDLE; ui.drawMain(state, true); }
    }
  }

  if (popupActive && millis() >= popupHideAt) {
    popupActive = false;
    ui.hidePopup();
    ui.drawMain(state, true);
  }

  Point p;
  if (touchReadMapped(p)) {
    delay(50);
    if (state.screen == Screen::MAIN) handleMain(p.x, p.y);
    else                               handleSettings(p.x, p.y);
  }
}

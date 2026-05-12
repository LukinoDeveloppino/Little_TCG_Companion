
#pragma once
#include <Arduino.h>

// ── Colori RGB565 ─────────────────────────────────────────────────────────────
#define C_BG      0x1082
#define C_SURFACE 0x2945
#define C_BORDER  0x39C7
#define C_WHITE   0xFFFF
#define C_GRAY    0x630C
#define C_GREEN   0x1DAB
#define C_BLUE    0x0543
#define C_YELLOW  0xFEC8
#define C_RED     0xFA28
#define C_PURPLE  0xBDF6

static const int CAL_VER = 4;

// ── Enumerazioni ──────────────────────────────────────────────────────────────
enum class Screen     { MAIN, SETTINGS };
enum class TimerState { IDLE, RUNNING, PAUSED };

// ── Stato globale dell'applicazione ──────────────────────────────────────────
struct AppState {
  Screen     screen         = Screen::MAIN;
  TimerState timerState     = TimerState::IDLE;
  bool       wins[2][2]     = {};
  uint32_t   timerRemaining = 3000;
  int        pickM = 50, pickS = 0;
};
#pragma once
#include <TFT_eSPI.h>

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

enum class Screen      { MAIN, SETTINGS };
enum class TimerState  { IDLE, RUNNING, PAUSED };

struct AppState {
  Screen     screen         = Screen::MAIN;
  TimerState timerState     = TimerState::IDLE;
  bool       wins[2][2]     = {};
  uint32_t   timerRemaining = 3000;
  uint32_t   timerCustom    = 3000;
  int        pickM = 50, pickS = 0;
};

class UI {
public:
  void begin();
  void drawMain(const AppState &s, bool full = false);
  void drawSettings(const AppState &s, bool full = false);
  void showPopup(const char *big, const char *label, uint16_t color);
  void hidePopup();
  void updateTimer(uint32_t remaining);
  TFT_eSPI &tft() { return _tft; }

private:
  TFT_eSPI _tft;
  bool _popupVisible = false;

  void drawTopBar(const AppState &s);
  void drawTimerDigits(uint32_t sec, uint16_t color);
  void drawButton(int x, int y, int w, int h, const char *label,
                  uint16_t bg, uint16_t fg, int radius = 10);
  void drawWinBox(int x, int y, bool filled, uint16_t color);
  void drawDivider(int y);
};

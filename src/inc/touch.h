#pragma once
#include <Arduino.h>

#define TP_SCK  25
#define TP_MISO 39
#define TP_MOSI 32
#define TP_CS   33
#define TP_IRQ  36

struct Point { int x; int y; };

struct Rect {
  int x, y, w, h;
  bool contains(const Point &p) const {
    return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
  }
};

void touchInit();
bool touchRead(uint16_t *x, uint16_t *y);
bool touchReadMapped(Point &p);
void touchSetCalibration(int xMin, int xMax, int yMin, int yMax, bool swapAxes = false);
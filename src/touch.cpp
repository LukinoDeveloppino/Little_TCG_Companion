#include <Arduino.h>
#include <touch.h>



void touchInit() {
  pinMode(TP_SCK,  OUTPUT); digitalWrite(TP_SCK,  LOW);
  pinMode(TP_MOSI, OUTPUT); digitalWrite(TP_MOSI, LOW);
  pinMode(TP_CS,   OUTPUT); digitalWrite(TP_CS,   HIGH);
  pinMode(TP_MISO, INPUT);
  pinMode(TP_IRQ,  INPUT);
}

uint16_t xptRead(uint8_t cmd) {
  uint16_t result = 0;
  digitalWrite(TP_CS, LOW);
  delayMicroseconds(1);

  for (int i = 7; i >= 0; i--) {
    digitalWrite(TP_SCK, LOW);
    digitalWrite(TP_MOSI, (cmd >> i) & 1 ? HIGH : LOW);
    digitalWrite(TP_SCK, HIGH);
  }

  for (int i = 15; i >= 0; i--) {
    digitalWrite(TP_SCK, LOW);
    digitalWrite(TP_SCK, HIGH);
    if (digitalRead(TP_MISO)) result |= (1 << i);
  }

  digitalWrite(TP_SCK, LOW);
  digitalWrite(TP_CS, HIGH);
  return result >> 3;
}

bool touchRead(uint16_t *x, uint16_t *y) {
  if (digitalRead(TP_IRQ) != LOW) return false;

  uint16_t z = xptRead(0xB0);
  if (z < 100) return false;

  uint32_t sx = 0, sy = 0;
  for (int i = 0; i < 4; i++) {
    sx += xptRead(0xD0);
    sy += xptRead(0x90);
  }
  *x = sx / 4;
  *y = sy / 4;
  return true;
}

static int _xMin = 200, _xMax = 3800, _yMin = 200, _yMax = 3800;

void touchSetCalibration(int xMin, int xMax, int yMin, int yMax) {
  _xMin = xMin; _xMax = xMax; _yMin = yMin; _yMax = yMax;
}

bool touchReadMapped(Point &p) {
  uint16_t rx, ry;
  if (!touchRead(&rx, &ry)) return false;
  p.x = constrain((int)map(rx, _xMin, _xMax, 0, 320), 0, 319);
  p.y = constrain((int)map(ry, _yMin, _yMax, 0, 240), 0, 239);
  return true;
}
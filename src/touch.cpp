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
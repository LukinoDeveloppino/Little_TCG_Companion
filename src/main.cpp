#include <Arduino.h>
#include <TFT_eSPI.h>
#include <touch.h>

TFT_eSPI tft;


void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Tocca lo schermo...");

  touchInit();
  Serial.println("Ready");
}

void loop() {
  uint16_t x, y;
  if (touchRead(&x, &y)) {
    Serial.printf("X=%4d  Y=%4d\n", x, y);
    tft.fillRect(0, 50, 320, 30, TFT_BLACK);
    tft.setCursor(10, 55);
    tft.printf("X=%4d  Y=%4d", x, y);
    delay(80);
  }
}

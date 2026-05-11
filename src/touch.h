#include <Arduino.h>

#define TP_SCK  25
#define TP_MISO 39
#define TP_MOSI 32
#define TP_CS   33
#define TP_IRQ  36

void touchInit();
uint16_t xptRead(uint8_t cmd);
bool touchRead(uint16_t *x, uint16_t *y);
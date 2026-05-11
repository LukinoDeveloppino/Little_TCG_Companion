#pragma once
#include <Arduino.h>

class Timer {
public:
  void begin(uint32_t seconds);
  void start();
  void pause();
  void reset();
  void update();
  bool isRunning() const { return _running; }
  uint32_t remaining() const { return _remaining; }

private:
  uint32_t _total = 3000;
  uint32_t _remaining = 3000;
  uint32_t _lastTick = 0;
  bool _running = false;
};

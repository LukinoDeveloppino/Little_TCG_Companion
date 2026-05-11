#include "timer.h"

void Timer::begin(uint32_t seconds) {
  _total = seconds;
  _remaining = seconds;
  _running = false;
}

void Timer::start() {
  if (!_running && _remaining > 0) {
    _lastTick = millis();
    _running = true;
  }
}

void Timer::pause() { _running = false; }

void Timer::reset() {
  _running = false;
  _remaining = _total;
}

void Timer::update() {
  if (!_running) return;
  uint32_t now = millis();
  if (now - _lastTick >= 1000) {
    _lastTick += 1000;
    if (_remaining > 0) _remaining--;
    else _running = false;
  }
}

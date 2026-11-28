// Порт USB Serial/JTAG (HWCDC ядра) для core/: вход команд, выход, часы, мьютекс, куча под образ.
#pragma once
#include <Arduino.h>

#include "core/io.h"

class UsbInput final : public core::Input {
 public:
  size_t read(uint8_t *out, size_t max, uint32_t timeout_ms) override {
    uint32_t t0 = millis();
    for (;;) {
      int n = Serial.available();
      if (n > 0) return Serial.read(out, (size_t)n < max ? (size_t)n : max);
      if (millis() - t0 >= timeout_ms) return 0;
      delay(1);
    }
  }
};

class ClockEsp final : public core::Clock {
 public:
  uint32_t now_ms() override { return millis(); }
  void sleep_ms(uint32_t ms) override { delay(ms); }
};

class MutexLock final : public core::Lock {
 public:
  void begin() { m_ = xSemaphoreCreateMutex(); }
  void lock() override { xSemaphoreTake(m_, portMAX_DELAY); }
  void unlock() override { xSemaphoreGive(m_); }

 private:
  SemaphoreHandle_t m_ = nullptr;
};

class HeapMemory final : public core::Memory {
 public:
  uint8_t *alloc(size_t len) override { return (uint8_t *)malloc(len); }
  void release(uint8_t *p) override { free(p); }
  size_t largest() override { return ESP.getMaxAllocHeap(); }
  size_t free_total() override { return ESP.getFreeHeap(); }
};

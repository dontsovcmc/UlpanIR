// core::Debug поверх ch32fun funprog (port/funprog_glue.c).
#pragma once
#include "core/debug.h"

class DebugRvswd final : public core::Debug {
 public:
  void begin();  // SWDIO и SWCLK — вход без подтяжки
  bool attach(core::ChipId &id) override;
  bool erase(uint32_t addr, uint32_t len, core::Area area) override;
  bool write(uint32_t addr, const uint8_t *data, uint32_t len) override;
  bool verify(uint32_t addr, const uint8_t *data, uint32_t len) override;
  void reset() override;
  void detach() override;
};

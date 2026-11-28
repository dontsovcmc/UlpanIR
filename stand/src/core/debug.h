// 2-проводная отладка CH592F (RVSWD), как её видит core/: сеанс, стирание, запись, сверка, перезапуск.
// Реализация — port/debug_rvswd поверх ch32fun funprog (design §4).
#pragma once
#include <stdint.h>

namespace core {

// Семейства — как enum RiscVChip в funprog_rvswdio.h.
constexpr uint8_t FAMILY_CH59X = 0x0b;

// Основная flash с адреса 0 и DataFlash, где пульт хранит настройки (docs/ble_protocol.md).
constexpr uint32_t DATAFLASH_ADDR = 0x70000;
constexpr uint32_t DATAFLASH_SIZE = 32 * 1024;
constexpr uint32_t FLASH_BLOCK = 256;  // сектор записи и стирания CH59x

enum class Area : uint8_t { Program, DataFlash };

struct ChipId {
  uint8_t family;
  uint8_t uid[8];
};

class Debug {
 public:
  // Подключиться, остановить ядро, узнать семейство и UID, поднять частоту. false — нет ответа.
  virtual bool attach(ChipId &id) = 0;
  virtual bool erase(uint32_t addr, uint32_t len, Area area) = 0;
  // len кратна FLASH_BLOCK.
  virtual bool write(uint32_t addr, const uint8_t *data, uint32_t len) = 0;
  virtual bool verify(uint32_t addr, const uint8_t *data, uint32_t len) = 0;
  // Перезапустить МК и отпустить SWDIO/SWCLK.
  virtual void reset() = 0;
  // Отпустить SWDIO/SWCLK без перезапуска (после неудачного attach).
  virtual void detach() = 0;

 protected:
  ~Debug() = default;
};

}  // namespace core

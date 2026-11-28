// Линии кнопок J1.1 и J1.3: только 0 или вход (design §7). Ноги знает только port/lines_gpio.
#pragma once
#include <stdint.h>

namespace core {

enum : uint8_t { BTN_A = 1, BTN_B = 2 };

class Lines {
 public:
  // Вывести в 0 узлы из mask одновременно (одной записью в регистр).
  virtual void press(uint8_t mask) = 0;
  // Узлы кнопок — вход без подтяжки.
  virtual void release_buttons() = 0;
  // Все линии J1, в том числе SWDIO и SWCLK, — вход без подтяжки.
  virtual void release_all() = 0;

 protected:
  ~Lines() = default;
};

}  // namespace core

// core::Lines на GPIO C6: узлы кнопок только 0 или вход (design §7).
#pragma once
#include "core/lines.h"

class LinesGpio final : public core::Lines {
 public:
  void begin();
  void press(uint8_t mask) override;
  void release_buttons() override;
  void release_all() override;
};

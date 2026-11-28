// Приём лога пульта с J1.8: UART1 115200 8N1, только RX, без подтяжки (design §7).
#pragma once
#include <stddef.h>
#include <stdint.h>

class UartLog {
 public:
  void begin();
  size_t read(uint8_t *out, size_t max);
};

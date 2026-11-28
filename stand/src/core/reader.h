// Строки команд и сырые байты образа `#flash` из одного входа: байты образа, пришедшие одним куском с
// командой, не теряются (design §5).
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "core/io.h"

namespace core {

class Reader {
 public:
  static constexpr size_t MAX_LINE = 64;
  enum class Got : uint8_t { Line, TooLong, Timeout };

  explicit Reader(Input &in) : in_(in) {}

  // Строка без `\n` и `\r` в out[MAX_LINE + 1]. TooLong — строка длиннее MAX_LINE, отброшена до `\n`.
  // Timeout — за timeout_ms вход замолчал, начатая строка сохраняется до следующего вызова.
  Got line(char *out, uint32_t timeout_ms);
  // Ровно n байт, если вход не молчал дольше idle_ms; иначе — сколько успело прийти.
  size_t bytes(uint8_t *out, size_t n, uint32_t idle_ms);

 private:
  bool fill(uint32_t timeout_ms);

  Input &in_;
  uint8_t buf_[256];
  size_t pos_ = 0, len_ = 0;
  char line_[MAX_LINE];
  size_t line_len_ = 0;
  bool too_long_ = false;
};

}  // namespace core

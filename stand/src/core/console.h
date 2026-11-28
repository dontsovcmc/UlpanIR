// Один поток в USB: лог пульта как есть и строки стенда `# …`, всегда с новой строки (design §3,
// требование stand-port «Лог пульта в порту стенда»). Пишут две задачи — мьютекс из port/.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "core/io.h"

namespace core {

using Sink = void (*)(void *ctx, const uint8_t *data, size_t len);

class Console {
 public:
  Console(Sink sink, void *ctx, Lock *lock = nullptr);

  // Байты лога пульта — как есть.
  void log(const uint8_t *data, size_t len);
  // Строка стенда: `# ` + текст + `\n`. Если строка лога не кончилась — сначала `\n`.
  void reply(const char *fmt, ...) __attribute__((format(printf, 2, 3)));

 private:
  void emit(const uint8_t *data, size_t len);

  Sink sink_;
  void *ctx_;
  Lock *lock_;
  bool mid_line_ = false;
};

}  // namespace core

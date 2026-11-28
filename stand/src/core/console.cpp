#include "core/console.h"

#include <stdarg.h>
#include <stdio.h>

namespace core {

namespace {

class Guard {
 public:
  explicit Guard(Lock *l) : l_(l) {
    if (l_) l_->lock();
  }
  ~Guard() {
    if (l_) l_->unlock();
  }

 private:
  Lock *l_;
};

}  // namespace

Console::Console(Sink sink, void *ctx, Lock *lock) : sink_(sink), ctx_(ctx), lock_(lock) {}

void Console::log(const uint8_t *data, size_t len) {
  if (len == 0) return;
  Guard g(lock_);
  sink_(ctx_, data, len);
  mid_line_ = data[len - 1] != '\n';
}

void Console::reply(const char *fmt, ...) {
  char buf[192];
  buf[0] = '#';
  buf[1] = ' ';
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf + 2, sizeof buf - 3, fmt, ap);
  va_end(ap);
  size_t text = n < 0 ? 0 : (size_t)n;
  if (text > sizeof buf - 4) text = sizeof buf - 4;  // обрезано vsnprintf
  size_t len = 2 + text;
  buf[len++] = '\n';
  Guard g(lock_);
  if (mid_line_) sink_(ctx_, (const uint8_t *)"\n", 1);
  sink_(ctx_, (const uint8_t *)buf, len);
  mid_line_ = false;
}

}  // namespace core

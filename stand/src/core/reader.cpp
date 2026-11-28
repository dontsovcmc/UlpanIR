#include "core/reader.h"

#include <string.h>

namespace core {

bool Reader::fill(uint32_t timeout_ms) {
  pos_ = 0;
  len_ = in_.read(buf_, sizeof buf_, timeout_ms);
  return len_ > 0;
}

Reader::Got Reader::line(char *out, uint32_t timeout_ms) {
  for (;;) {
    if (pos_ == len_ && !fill(timeout_ms)) return Got::Timeout;
    while (pos_ < len_) {
      char c = (char)buf_[pos_++];
      if (c == '\n') {
        bool too_long = too_long_;
        size_t n = line_len_;
        too_long_ = false;
        line_len_ = 0;
        if (too_long) return Got::TooLong;
        memcpy(out, line_, n);
        out[n] = 0;
        return Got::Line;
      }
      if (c == '\r' || too_long_) continue;
      if (line_len_ == MAX_LINE) {
        too_long_ = true;
        continue;
      }
      line_[line_len_++] = c;
    }
  }
}

size_t Reader::bytes(uint8_t *out, size_t n, uint32_t idle_ms) {
  size_t got = 0;
  size_t have = len_ - pos_;
  if (have) {
    size_t k = have < n ? have : n;
    memcpy(out, buf_ + pos_, k);
    pos_ += k;
    got = k;
  }
  while (got < n) {
    size_t k = in_.read(out + got, n - got, idle_ms);
    if (k == 0) break;
    got += k;
  }
  return got;
}

}  // namespace core

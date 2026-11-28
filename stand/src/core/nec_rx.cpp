#include "core/nec_rx.h"

#include <stdio.h>

namespace core {

namespace {

// Длительности NEC в мкс — как в кодировщике пульта firmware/src/nec.c.
constexpr uint32_t LEAD_MARK = 9000;
constexpr uint32_t LEAD_SPACE = 4500;
constexpr uint32_t REPEAT_SPACE = 2250;
constexpr uint32_t BIT_MARK = 562;
constexpr uint32_t ZERO_SPACE = 562;
constexpr uint32_t ONE_SPACE = 1687;

// ±25 %
bool near(uint32_t v, uint32_t ref) { return v * 4 >= ref * 3 && v * 4 <= ref * 5; }

}  // namespace

IrFrame nec_decode(const uint32_t *d, size_t n) {
  IrFrame f{};
  f.kind = IrFrame::Unparsed;
  f.pulses = (uint16_t)((n + 1) / 2);
  if (n < 3 || !near(d[0], LEAD_MARK)) return f;

  // Пауза после последней метки, если приёмник её отдал, не считается.
  if ((n == 3 || n == 4) && near(d[1], REPEAT_SPACE) && near(d[2], BIT_MARK)) {
    f.kind = IrFrame::Repeat;
    return f;
  }
  if ((n != 67 && n != 68) || !near(d[1], LEAD_SPACE) || !near(d[66], BIT_MARK)) return f;

  uint8_t b[4] = {0, 0, 0, 0};
  for (int k = 0; k < 32; k++) {
    uint32_t mark = d[2 + 2 * k], space = d[3 + 2 * k];
    if (!near(mark, BIT_MARK)) return f;
    if (near(space, ONE_SPACE))
      b[k / 8] |= (uint8_t)(1u << (k % 8));
    else if (!near(space, ZERO_SPACE))
      return f;
  }
  if (b[3] != (uint8_t)~b[2]) return f;
  f.kind = IrFrame::Nec;
  f.command = b[2];
  f.addr16 = b[1] != (uint8_t)~b[0];
  f.address = f.addr16 ? (uint16_t)(b[0] | (b[1] << 8)) : b[0];
  return f;
}

size_t ir_filter(uint32_t *d, size_t n, uint32_t min_us) {
  size_t w = 0;
  for (size_t i = 0; i < n; i++) {
    // Помеха внутри: она и следующая длительность (противоположного уровня) прибавляются к предыдущей.
    if (d[i] < min_us && w > 0 && i + 1 < n) {
      d[w - 1] += d[i] + d[i + 1];
      i++;
      continue;
    }
    d[w++] = d[i];
  }
  return w;
}

void ir_format(const IrFrame &f, uint32_t t_ms, char *out, size_t cap) {
  unsigned long t = t_ms;
  switch (f.kind) {
    case IrFrame::Nec:
      if (f.addr16)
        snprintf(out, cap, "ir: nec 0x%04x 0x%02x t=%lu", f.address, f.command, t);
      else
        snprintf(out, cap, "ir: nec 0x%02x 0x%02x t=%lu", f.address, f.command, t);
      break;
    case IrFrame::Repeat:
      snprintf(out, cap, "ir: повтор t=%lu", t);
      break;
    default:
      snprintf(out, cap, "ir: неразобрано %u t=%lu", f.pulses, t);
  }
}

}  // namespace core

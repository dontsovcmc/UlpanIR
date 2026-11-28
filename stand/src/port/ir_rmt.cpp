#include "port/ir_rmt.h"

#include <Arduino.h>

#include "core/nec_rx.h"
#include "pins.h"

namespace {

constexpr uint32_t TICK_HZ = 1000000;  // 1 мкс
constexpr uint16_t END_TICKS = 12000;  // 12 мс тишины — конец кадра
// Аппаратный фильтр RMT — не длиннее 255 тактов источника 80 МГц (около 3 мкс); помехи до 100 мкс
// склеивает core::ir_filter.
constexpr uint8_t HW_FILTER_TICKS = 3;
constexpr uint32_t GLITCH_US = 100;

// Кадр NEC — 34 символа; запас на помехи.
rmt_data_t symbols[96];
size_t count;
uint32_t dur[2 * 96];

}  // namespace

// Гипотеза: rmtRead/rmtReadAsync ядра 3.x отдаёт целый кадр NEC (около 68 мс) одним чтением (design §8).
bool IrRmt::begin() {
  ok_ = rmtInit(PIN_IR, RMT_RX_MODE, RMT_MEM_NUM_BLOCKS_2, TICK_HZ) && rmtSetRxMaxThreshold(PIN_IR, END_TICKS) &&
        rmtSetRxMinThreshold(PIN_IR, HW_FILTER_TICKS);
  if (ok_) start();
  return ok_;
}

void IrRmt::start() {
  count = RMT_SYMBOLS_OF(symbols);
  rmtReadAsync(PIN_IR, symbols, &count);
}

// Выход приёмника активен нулём: метка — уровень 0.
void IrRmt::poll(core::IrGate &gate, core::Console &out) {
  if (!ok_ || !rmtReceiveCompleted(PIN_IR)) return;
  uint32_t now = millis();
  size_t n = 0;
  for (size_t i = 0; i < count; i++) {
    const rmt_data_t &s = symbols[i];
    if (s.duration0 == 0) break;
    if (!(n == 0 && s.level0 == 1)) dur[n++] = s.duration0;
    if (s.duration1 == 0) break;
    dur[n++] = s.duration1;
  }
  start();
  if (!gate.enabled() || n == 0) return;
  n = core::ir_filter(dur, n, GLITCH_US);
  char line[64];
  core::ir_format(core::nec_decode(dur, n), gate.since(now), line, sizeof line);
  out.reply("%s", line);
}

#include "port/adc_vbat.h"

#include <Arduino.h>
#include <driver/gpio.h>

#include "pins.h"

namespace {

// Настройка канала АЦП драйвером ESP-IDF снимает подтяжки ноги — вернуть после каждого замера.
void pull_down() { gpio_pulldown_en((gpio_num_t)PIN_VBAT_SW); }

}  // namespace

void AdcVbat::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_VBAT_SW, ADC_11db);  // в ESP-IDF 5 — ADC_ATTEN_DB_12
  (void)analogReadMilliVolts(PIN_VBAT_SW);
  pull_down();
}

int AdcVbat::read_mv() {
  constexpr int N = 8;
  int sum = 0;
  for (int i = 0; i < N; i++) sum += analogReadMilliVolts(PIN_VBAT_SW);
  pull_down();
  return sum / N;
}

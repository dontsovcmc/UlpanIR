#include "port/lines_gpio.h"

#include <driver/gpio.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>

#include "pins.h"
#include "port/funprog_glue.h"

namespace {

constexpr uint32_t BIT_A = 1u << PIN_BTN_A;
constexpr uint32_t BIT_B = 1u << PIN_BTN_B;

uint32_t bits(uint8_t mask) { return (mask & core::BTN_A ? BIT_A : 0) | (mask & core::BTN_B ? BIT_B : 0); }

}  // namespace

// Вход без подтяжки; в регистре выхода заранее 0, сигнал ноги — бит GPIO_OUT_REG (выбор 128 в матрице):
// «нажать» — только включить выход, в 1 нога не выводится никогда.
void LinesGpio::begin() {
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = (1ULL << PIN_BTN_A) | (1ULL << PIN_BTN_B);
  cfg.mode = GPIO_MODE_INPUT;
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&cfg);
  REG_WRITE(GPIO_OUT_W1TC_REG, BIT_A | BIT_B);
  REG_WRITE(GPIO_FUNC0_OUT_SEL_CFG_REG + PIN_BTN_A * 4, SIG_GPIO_OUT_IDX);
  REG_WRITE(GPIO_FUNC0_OUT_SEL_CFG_REG + PIN_BTN_B * 4, SIG_GPIO_OUT_IDX);
}

// Обе линии одной записью в регистр: пульт видит «обе сразу» (design §7).
void LinesGpio::press(uint8_t mask) {
  REG_WRITE(GPIO_OUT_W1TC_REG, BIT_A | BIT_B);
  REG_WRITE(GPIO_ENABLE_W1TS_REG, bits(mask));
}

void LinesGpio::release_buttons() { REG_WRITE(GPIO_ENABLE_W1TC_REG, BIT_A | BIT_B); }

void LinesGpio::release_all() {
  release_buttons();
  fp_detach();
}

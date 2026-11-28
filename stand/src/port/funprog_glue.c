// ch32fun funprog одной единицей трансляции (порядок include — vendor/.../funprog/README.md) и сеанс
// RVSWD на выводах SWCLK/SWDIO стенда.
#include "port/funprog_glue.h"

#include <driver/gpio.h>
#include <soc/gpio_sig_map.h>
#include <string.h>

#include "pins.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "funprog_config.h"
#include "funprog_port.h"
#include "funprog_rvswdio.h"
#include "port/bitbang_rvswd_c6.h"
#include "funprog_ch5xx.h"
#pragma GCC diagnostic pop

static struct SWIOState st;

#define MASK_C (1u << PIN_SWCLK)
#define MASK_D (1u << PIN_SWDIO)

// Выход ноги — бит GPIO_OUT_REG, включение — бит GPIO_ENABLE_REG (выбор сигнала 128 в матрице GPIO).
static void route_gpio_out(int pin) { REG_WRITE(GPIO_FUNC0_OUT_SEL_CFG_REG + pin * 4, SIG_GPIO_OUT_IDX); }

static void float_pins(void) {
  RV_OE_CLR(MASK_C | MASK_D);
  gpio_config_t c = {
      .pin_bit_mask = (1ULL << PIN_SWCLK) | (1ULL << PIN_SWDIO),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&c);
}

static void session_pins(void) {
  float_pins();
  route_gpio_out(PIN_SWCLK);
  route_gpio_out(PIN_SWDIO);
  // Подтяжка SWDIO — только на время сеанса: вне него она подпитывала бы выключенный пульт (design §4).
  gpio_pullup_en((gpio_num_t)PIN_SWDIO);
  RV_OUT_SET(MASK_C | MASK_D);
  RV_OE_SET(MASK_C);
}

int fp_attach(uint8_t *family, uint8_t uid[8]) {
  session_pins();
  memset(&st, 0, sizeof st);
  st.pinmaskC = (int)MASK_C;
  st.pinmaskD = (int)MASK_D;
  ResetInternalProgrammingState(&st);
  uint8_t reply[8];
  int r = DetermineChipTypeAndSectorInfo(&st, reply);
  if (r) return r;
  if (st.target_chip_type == CHIP_UNKNOWN) return -1;
  *family = (uint8_t)st.target_chip_type;
  memset(uid, 0, 8);
  if (st.target_chip_type == CHIP_CH59x) return ch5xx_read_uuid(&st, uid);
  return 0;
}

int fp_erase(uint32_t addr, uint32_t len, int dataflash) {
  return CH5xxErase(&st, addr, len, dataflash ? EEPROM_AREA : PROGRAM_AREA);
}

// Быстрая запись блобом в ОЗУ МК — как minichlink с программатором funprog (pgm-esp32s2-ch32xx.c).
int fp_write(uint32_t addr, const uint8_t *data, uint32_t len) {
  int r = ch5xx_write_flash_using_microblob(&st, addr, (uint8_t *)data, len);
  ch5xx_microblob_end(&st);
  return r;
}

// Блоб занимал регистры ядра: программу сверки funprog загружает заново.
int fp_verify(uint32_t addr, const uint8_t *data, uint32_t len) {
  st.statetag = STTAG("XXXX");
  return ch5xx_verify_data(&st, addr, (uint8_t *)data, len);
}

// HALT_MODE_REBOOT из minichlink.c (DefaultHaltMode): остановить, перезапустить, отпустить ядро.
void fp_reset(void) {
  MCFWriteReg32(&st, DMCONTROL, 0x80000001);
  MCFWriteReg32(&st, DMCONTROL, 0x80000001);
  MCFWriteReg32(&st, DMCONTROL, 0x80000003);
  MCFWriteReg32(&st, DMCONTROL, 0x40000001);
  DELAY_US(10000);
  float_pins();
}

void fp_detach(void) { float_pins(); }

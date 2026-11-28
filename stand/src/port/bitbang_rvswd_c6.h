// Биты 2-проводной отладки WCH (RVSWD) регистрами GPIO ESP32-C6 — для funprog (design §4).
// Перенос bitbang_rvswdio_esp32s2.h из cnlohr/esp32s2-cookbook (ch32v003programmer, MIT): та же
// последовательность бит, вместо внешних резисторов подтяжки — встроенная подтяжка SWDIO на время сеанса.
// SWCLK ведёт стенд, поэтому точные тайминги не нужны; полупериод — в тактах ЦП (160 МГц).
// Однопроводный SWIO (CH32V003) не нужен: его функции — пустые, чтение отвечает ошибкой.
#ifndef BITBANG_RVSWD_C6_H
#define BITBANG_RVSWD_C6_H

#include <esp_cpu.h>
#include <soc/gpio_reg.h>

#define RV_OUT_SET(m) REG_WRITE(GPIO_OUT_W1TS_REG, (m))
#define RV_OUT_CLR(m) REG_WRITE(GPIO_OUT_W1TC_REG, (m))
#define RV_OE_SET(m) REG_WRITE(GPIO_ENABLE_W1TS_REG, (m))
#define RV_OE_CLR(m) REG_WRITE(GPIO_ENABLE_W1TC_REG, (m))
#define RV_IN() REG_READ(GPIO_IN_REG)

static inline void IRAM PrecDelay(int cycles) {
  uint32_t start = esp_cpu_get_cycle_count();
  while ((int)(esp_cpu_get_cycle_count() - start) < cycles) {
  }
}

static inline void Send1BitSWIO(struct SWIOState *state) { (void)state; }
static inline void Send0BitSWIO(struct SWIOState *state) { (void)state; }
static inline int ReadBitSWIO(struct SWIOState *state) {
  (void)state;
  return 2;
}

static inline void SendBitRVSWD(struct SWIOState *state, int val) {
  int t1 = state->t1coeff;
  RV_OUT_CLR(state->pinmaskC);
  if (val)
    RV_OUT_SET(state->pinmaskD);
  else
    RV_OUT_CLR(state->pinmaskD);
  RV_OE_SET(state->pinmaskD);
  PrecDelay(t1);
  RV_OUT_SET(state->pinmaskC);
  PrecDelay(t1);
}

static inline int ReadBitRVSWD(struct SWIOState *state) {
  int t1 = state->t1coeff;
  RV_OE_CLR(state->pinmaskD);  // SWDIO отпущен: держит МК или встроенная подтяжка
  RV_OUT_CLR(state->pinmaskC);
  PrecDelay(t1);
  int r = !!(RV_IN() & (uint32_t)state->pinmaskD);
  RV_OUT_SET(state->pinmaskC);
  PrecDelay(t1);
  return r;
}

static void rvswd_stop(struct SWIOState *state) {
  int t1 = state->t1coeff;
  RV_OUT_CLR(state->pinmaskC);
  PrecDelay(t1);
  RV_OUT_CLR(state->pinmaskD);
  RV_OE_SET(state->pinmaskD);
  PrecDelay(t1);
  RV_OUT_SET(state->pinmaskC);
  PrecDelay(t1);
  RV_OUT_SET(state->pinmaskD);
}

static void rvswd_start(struct SWIOState *state) {
  RV_OUT_SET(state->pinmaskC);
  RV_OE_SET(state->pinmaskC);
  RV_OUT_SET(state->pinmaskD);
  RV_OE_SET(state->pinmaskD);
  RV_OUT_CLR(state->pinmaskD);
  PrecDelay(state->t1coeff);
}

static void MCFWriteReg32(struct SWIOState *state, uint8_t command, uint32_t value) {
  if (state->opmode != 2) return;
  rvswd_start(state);
  int parity = 1;
  for (uint32_t mask = 1 << 6; mask; mask >>= 1) {
    int v = !!(command & mask);
    parity ^= v;
    SendBitRVSWD(state, v);
  }
  SendBitRVSWD(state, 1);  // запись
  SendBitRVSWD(state, parity);
  ReadBitRVSWD(state);
  ReadBitRVSWD(state);
  ReadBitRVSWD(state);
  SendBitRVSWD(state, 0);
  SendBitRVSWD(state, 0);

  parity = 0;
  for (uint32_t mask = 1u << 31; mask; mask >>= 1) {
    int v = !!(value & mask);
    parity ^= v;
    SendBitRVSWD(state, v);
  }
  SendBitRVSWD(state, parity);
  ReadBitRVSWD(state);
  ReadBitRVSWD(state);
  ReadBitRVSWD(state);
  SendBitRVSWD(state, 1);
  SendBitRVSWD(state, 0);
  rvswd_stop(state);
  esp_rom_delay_us(8);  // в образце: «Sometimes 2 is too short»
}

// 0 — прочитано, иначе ошибка (нет ответа или чётность не сошлась).
static int MCFReadReg32(struct SWIOState *state, uint8_t command, uint32_t *value) {
  if (state->opmode != 2) return -2;
  rvswd_start(state);
  int parity = 0;
  for (uint32_t mask = 1 << 6; mask; mask >>= 1) {
    int v = !!(command & mask);
    parity ^= v;
    SendBitRVSWD(state, v);
  }
  SendBitRVSWD(state, 0);  // чтение
  SendBitRVSWD(state, parity);
  ReadBitRVSWD(state);
  ReadBitRVSWD(state);
  ReadBitRVSWD(state);
  SendBitRVSWD(state, 0);
  SendBitRVSWD(state, 0);

  uint32_t rval = 0;
  parity = 0;
  for (int i = 0; i < 32; i++) {
    rval <<= 1;
    if (ReadBitRVSWD(state)) {
      rval |= 1;
      parity ^= 1;
    }
  }
  *value = rval;
  int bad = ReadBitRVSWD(state) != parity;
  ReadBitRVSWD(state);
  ReadBitRVSWD(state);
  ReadBitRVSWD(state);
  SendBitRVSWD(state, 1);
  SendBitRVSWD(state, 0);
  rvswd_stop(state);
  esp_rom_delay_us(8);
  return bad ? -23 : 0;
}

static inline void ConfigureIOForRVSWIO(struct SWIOState *state) { (void)state; }

static inline void ConfigureIOForRVSWD(struct SWIOState *state) {
  RV_OUT_SET(state->pinmaskC);
  RV_OE_SET(state->pinmaskC);
}

// Гипотеза: полупериоды 0,25/0,5/2 мкс (40/80/320 тактов при 160 МГц) CH592 принимает; если нет —
// увеличить (design Risks «RVSWD с C6 не работает»). funprog ставит MEDIUM для CH59x.
static void SetProtocolSpeed(struct SWIOState *state, enum ProtocolSpeed speed) {
  state->protocol_speed = speed;
  switch (speed) {
    case PROTOCOL_SPEED_LOW: state->t1coeff = 320; break;
    case PROTOCOL_SPEED_MEDIUM: state->t1coeff = 80; break;
    default: state->t1coeff = 40;
  }
}

#endif

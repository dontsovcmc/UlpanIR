// Настройки ch32fun funprog для стенда: только CH5xx по RVSWD (design §4). Образец —
// vendor/ch32fun/minichlink/funprog (funprog_config_example.h в upstream).
#ifndef FUNPROG_CONFIG_H
#define FUNPROG_CONFIG_H

#define DEBUG_PRINT 0
#define OLD_SWD_SUPPORT 0  // старый SWD нужен CH32V103, CH571/3, CH582/3; у CH592 — новый RVSWD
#define CH5xx_SUPPORT 1
#define CH5xx_EEPROM 0
#define CH5xx_OPTIONS 0
#define CH5xx_UNLOCK 0
#define UPDI_SUPPORT 0
#define MCO_SUPPORT 0

#endif

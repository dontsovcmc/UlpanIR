// Платформенная часть funprog на ESP32-C6: задержки и атрибут функций в IRAM.
#ifndef FUNPROG_PORT_H
#define FUNPROG_PORT_H

#include <esp_attr.h>
#include <esp_rom_sys.h>

#define FUN_PRINTF_DEBUG(format, ...)
#define STATUS_LED_ON
#define STATUS_LED_OFF
#define DELAY_US(n) esp_rom_delay_us(n)
#define DELAY_MS(n) esp_rom_delay_us((n) * 1000)
#define IRAM IRAM_ATTR

#endif

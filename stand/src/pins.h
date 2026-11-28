// Выводы ESP32-C6 Super Mini к гребёнке J1 пульта. Источник правды — таблица в stand/README.md (design §7).
// Вне команд все — вход без подтяжки. Не заняты: GPIO8, GPIO9, GPIO15 (стяжки и светодиоды Super Mini),
// GPIO12, GPIO13 (USB), GPIO16, GPIO17 (UART0, GPIO16 с самого сброса держит 1).
#pragma once

#define PIN_BTN_A 1     // J1.3 BTNA_N, кнопка A: только 0 или вход
#define PIN_BTN_B 2     // J1.1 BTNB_N, кнопка B: только 0 или вход
#define PIN_SWCLK 18    // J1.7 SWCLK (PB15): выход только во время сеанса отладки
#define PIN_SWDIO 19    // J1.6 SWDIO (PB14): встроенная подтяжка только во время сеанса
#define PIN_LOG_RX 4    // J1.8 TX пульта (PA9): приём лога UART1 115200 8N1, без подтяжки
#define PIN_VBAT_SW 0   // J1.5 VBAT_SW: АЦП, встроенная подтяжка к земле разряжает C4
#define PIN_IR 3        // выход ИК-приёмника 38 кГц: RMT RX

/* Developer log: UART1 TX (PA9, J1 "TX"), 115200 8N1. Business events and error reasons go here;
 * the BLE protocol itself only says "ok" or "rejected". Built with LOG=1 (default); LOG=0 removes it. */
#ifndef LOG_H
#define LOG_H
#include "settings.h"

#ifdef DEBUG
#include <stdio.h>
#define LOG(...)            \
    do {                    \
        printf(__VA_ARGS__); \
        printf("\r\n");      \
    } while (0)
void log_init(void);
void log_settings(const char *tag, const settings_t *s);
#else
#define LOG(...) \
    do {         \
    } while (0)
static inline void log_init(void) {}
static inline void log_settings(const char *tag, const settings_t *s) {
    (void)tag;
    (void)s;
}
#endif
#endif

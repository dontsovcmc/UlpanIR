#include "log.h"
#ifdef DEBUG
#include "CH59x_common.h"

void log_init(void) {
    GPIOA_SetBits(GPIO_Pin_9); /* TXD1 idles high */
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit(); /* 115200 8N1; printf goes here through _write in CH59x_sys.c (DEBUG=Debug_UART1) */
}

static void log_button(const char *name, const button_code_t *c) {
    if (c->assigned)
        printf(" %s=%s0x%0*X/0x%02X", name, c->extended ? "ext:" : "", c->extended ? 4 : 2, c->address, c->command);
    else
        printf(" %s=none", name);
}

void log_settings(const char *tag, const settings_t *s) {
    printf("%s:", tag);
    log_button("A", &s->a);
    log_button("B", &s->b);
    printf(" repeat=%u/s %s\r\n", s->repeat_hz, s->repeat_mode == REPEAT_NEC_CODE ? "nec-repeat" : "full-frame");
}
#endif

#ifdef DEBUG
/* newlib stdio wants these; only stdout over UART is used (_write in CH59x_sys.c). */
#include <sys/stat.h>
int _close(int fd) { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int off, int whence) { (void)fd; (void)off; (void)whence; return 0; }
int _read(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; return 0; }
#endif

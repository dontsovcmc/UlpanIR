// Тонкий C-интерфейс к ch32fun funprog для port/debug_rvswd. 0 — успех.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int fp_attach(uint8_t *family, uint8_t uid[8]);
int fp_erase(uint32_t addr, uint32_t len, int dataflash);
int fp_write(uint32_t addr, const uint8_t *data, uint32_t len);
int fp_verify(uint32_t addr, const uint8_t *data, uint32_t len);
void fp_reset(void);
void fp_detach(void);

#ifdef __cplusplus
}
#endif

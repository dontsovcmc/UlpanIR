/* Settings in CH592 DataFlash: two slots, see docs/ble_protocol.md "Хранение во флеше". */
#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H
#include "settings.h"

/* Reads both slots and returns the newest valid block, or defaults on a clean chip. */
void settings_load(settings_t *s);
/* Writes into the slot that is not the current best, verifies it; SETTINGS_OK or SETTINGS_ERR_FLASH.
 * On failure the previous settings stay in flash and stay the best slot. */
int settings_save(const settings_t *s);
#endif

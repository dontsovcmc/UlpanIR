/* Settings in CH592 DataFlash: one record with CRC, see docs/ble_protocol.md "Хранение во флеше". */
#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H
#include "settings.h"

/* Reads the record; defaults (nothing assigned) on a clean chip or a damaged record. */
void settings_load(settings_t *s);
/* Erases, writes and verifies the record; SETTINGS_OK or SETTINGS_ERR_FLASH. A write interrupted by power
 * loss leaves a record with a bad CRC: the remote starts unconfigured and is configured again. */
int settings_save(const settings_t *s);
#endif

/* Settings in CH592 DataFlash: one record with CRC, see docs/ble_protocol.md "Хранение во флеше". */
#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H
#include "settings.h"

/* Reads the record; false and defaults (nothing assigned) on a clean chip or a damaged record. */
bool settings_load(settings_t *s);
/* Erases and writes the record; NULL on success, else the reason. A write interrupted by power
 * loss leaves a record with a bad CRC: the remote starts unconfigured and is configured again. */
const char *settings_save(const settings_t *s);
#endif

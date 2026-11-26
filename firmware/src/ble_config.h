/* BLE configuration mode: advertise "IR-Remote-XXXX", serve docs/ble_protocol.md, switch off after
 * 60 s without commands. Never returns. *settings is the working copy, updated on a successful write. */
#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H
#include "settings.h"
void ble_config_run(settings_t *settings) __attribute__((noreturn));
#endif

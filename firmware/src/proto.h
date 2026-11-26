/* BLE configuration protocol v1 (docs/ble_protocol.md): characteristic logic without the BLE stack.
 * The GATT callbacks in ble_config.c call these; any reason becomes one ATT error and a log line. */
#ifndef PROTO_H
#define PROTO_H
#include "settings.h"

#define PROTO_VERSION    1
#define PROTO_STATUS_LEN 6
#define PROTO_NAME_LEN   14 /* "IR-Remote-XXXX" */

typedef enum {
    PROTO_UUID_SERVICE = 1,
    PROTO_UUID_CONFIG = 2,
    PROTO_UUID_STATUS = 3,
    PROTO_UUID_PING = 4,
    PROTO_UUID_TEST = 5,
} proto_uuid_t;

/* 128-bit UUID d5e8000N-b2f6-4b31-98c6-7923e11860fc, little-endian byte order of the BLE stack. */
void proto_uuid(proto_uuid_t which, uint8_t out[16]);

/* config write: validate, save, then update *work; NULL on success, else the reason (for the log);
 * on error *work stays as it was. */
const char *proto_config_write(const uint8_t *buf, size_t len, settings_t *work,
                               const char *(*save)(const settings_t *));
void proto_config_read(const settings_t *work, uint8_t out[SETTINGS_BLOCK_LEN]);
void proto_status(uint16_t battery_mv, uint8_t out[PROTO_STATUS_LEN]);
/* test write: 1 byte (button) or 12 bytes (button + block v1); fills *code to send, stores nothing. */
const char *proto_test_write(const uint8_t *buf, size_t len, const settings_t *work, button_code_t *code);
/* Advertised name from the BLE address (stored least significant byte first). */
void proto_adv_name(const uint8_t mac[6], char out[PROTO_NAME_LEN + 1]);
/* Internal VBAT channel at -12 dB gain -> millivolts: V = (adc / 512 - 3) * 1.05 V. */
uint16_t proto_vbat_mv(uint16_t adc);
#endif

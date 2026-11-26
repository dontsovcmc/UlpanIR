/* Remote settings: block v1 (BLE characteristic `config`) and its DataFlash record (one copy + CRC).
 * Format and error codes: docs/ble_protocol.md. Pure logic, no hardware access. */
#ifndef SETTINGS_H
#define SETTINGS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SETTINGS_VERSION    1
#define SETTINGS_BLOCK_LEN  11
#define SETTINGS_RECORD_LEN 13 /* block + crc u16 */
#define SETTINGS_REPEAT_HZ_MIN 1
#define SETTINGS_REPEAT_HZ_MAX 10

/* Checks return NULL when all is well, or a short reason for the log ("repeat_hz not in 1..10").
 * Over BLE any reason becomes one ATT error: the page itself tells the user what is wrong. */

typedef enum { REPEAT_FULL_FRAME = 0, REPEAT_NEC_CODE = 1 } repeat_mode_t;

typedef struct {
    bool assigned;
    bool extended; /* 16-bit NEC address */
    uint16_t address;
    uint8_t command;
} button_code_t;

typedef struct {
    button_code_t a, b;
    uint8_t repeat_hz;
    repeat_mode_t repeat_mode;
} settings_t;

void settings_defaults(settings_t *s);
/* Validates and decodes a block; NULL if valid, else the reason, and *out is left untouched. */
const char *settings_parse(const uint8_t *buf, size_t len, settings_t *out);
void settings_serialize(const settings_t *s, uint8_t out[SETTINGS_BLOCK_LEN]);

uint16_t settings_crc16(const uint8_t *data, size_t len); /* CRC-16/CCITT-FALSE */
void settings_record_pack(const settings_t *s, uint8_t out[SETTINGS_RECORD_LEN]);
/* True and *out = stored settings if the CRC matches and the block is valid; otherwise false and defaults
 * (clean chip, or a write interrupted by pulling the battery: the remote is simply configured again). */
bool settings_record_unpack(const uint8_t in[SETTINGS_RECORD_LEN], settings_t *out);
#endif

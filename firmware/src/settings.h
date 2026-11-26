/* Remote settings: block v1 (BLE characteristic `config`) and the two-slot DataFlash record.
 * Format and error codes: docs/ble_protocol.md. Pure logic, no hardware access. */
#ifndef SETTINGS_H
#define SETTINGS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SETTINGS_VERSION    1
#define SETTINGS_BLOCK_LEN  11
#define SETTINGS_SLOT_LEN   17 /* counter u32 + block + crc u16 */
#define SETTINGS_REPEAT_HZ_MIN 1
#define SETTINGS_REPEAT_HZ_MAX 10

/* Results; non-zero values are the ATT application error codes sent to the page. */
enum {
    SETTINGS_OK = 0,
    SETTINGS_ERR_LENGTH = 0x80,
    SETTINGS_ERR_VERSION = 0x81,
    SETTINGS_ERR_FLAGS = 0x82,
    SETTINGS_ERR_ADDRESS = 0x83,
    SETTINGS_ERR_REPEAT_HZ = 0x84,
    SETTINGS_ERR_REPEAT_MODE = 0x85,
    SETTINGS_ERR_BUTTON = 0x86,
    SETTINGS_ERR_NOT_ASSIGNED = 0x87,
    SETTINGS_ERR_FLASH = 0x88,
    SETTINGS_ERR_BUSY = 0x89,
};

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
/* Validates and decodes a block; on error returns the code and leaves *out untouched. */
int settings_parse(const uint8_t *buf, size_t len, settings_t *out);
void settings_serialize(const settings_t *s, uint8_t out[SETTINGS_BLOCK_LEN]);

uint16_t settings_crc16(const uint8_t *data, size_t len); /* CRC-16/CCITT-FALSE */
void settings_slot_pack(uint32_t counter, const settings_t *s, uint8_t out[SETTINGS_SLOT_LEN]);
/* Picks the valid slot with the larger counter: returns 0 or 1 and fills *out / *counter;
 * returns -1 with defaults and counter 0 when neither slot is valid. */
int settings_pick(const uint8_t slot0[SETTINGS_SLOT_LEN], const uint8_t slot1[SETTINGS_SLOT_LEN],
                  settings_t *out, uint32_t *counter);
/* Slot to write next, given the result of settings_pick: never overwrite the current best. */
int settings_next_slot(int best);
#endif

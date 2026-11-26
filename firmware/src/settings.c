#include "settings.h"
#include <string.h>

#define FLAG_ASSIGNED 0x01
#define FLAG_EXTENDED 0x02

void settings_defaults(settings_t *s) {
    memset(s, 0, sizeof(*s));
    s->repeat_hz = 3;
    s->repeat_mode = REPEAT_FULL_FRAME;
}

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

/* Button record: flags u8, address u16, command u8. */
static int check_address(const uint8_t *p) {
    if ((p[0] & FLAG_ASSIGNED) && !(p[0] & FLAG_EXTENDED) && rd16(p + 1) > 0xFF) return SETTINGS_ERR_ADDRESS;
    return SETTINGS_OK;
}

static button_code_t decode_button(const uint8_t *p) {
    button_code_t b = {(p[0] & FLAG_ASSIGNED) != 0, (p[0] & FLAG_EXTENDED) != 0, rd16(p + 1), p[3]};
    return b;
}

static void encode_button(const button_code_t *b, uint8_t *p) {
    p[0] = (uint8_t)((b->assigned ? FLAG_ASSIGNED : 0) | (b->extended ? FLAG_EXTENDED : 0));
    wr16(p + 1, b->address);
    p[3] = b->command;
}

int settings_parse(const uint8_t *buf, size_t len, settings_t *out) {
    /* Checks in the order of docs/ble_protocol.md: the first failing one is returned. */
    if (len != SETTINGS_BLOCK_LEN) return SETTINGS_ERR_LENGTH;
    if (buf[0] != SETTINGS_VERSION) return SETTINGS_ERR_VERSION;
    if ((buf[1] | buf[5]) & ~(FLAG_ASSIGNED | FLAG_EXTENDED)) return SETTINGS_ERR_FLAGS;
    int err = check_address(buf + 1);
    if (err == SETTINGS_OK) err = check_address(buf + 5);
    if (err != SETTINGS_OK) return err;
    if (buf[9] < SETTINGS_REPEAT_HZ_MIN || buf[9] > SETTINGS_REPEAT_HZ_MAX) return SETTINGS_ERR_REPEAT_HZ;
    if (buf[10] > REPEAT_NEC_CODE) return SETTINGS_ERR_REPEAT_MODE;

    out->a = decode_button(buf + 1);
    out->b = decode_button(buf + 5);
    out->repeat_hz = buf[9];
    out->repeat_mode = (repeat_mode_t)buf[10];
    return SETTINGS_OK;
}

void settings_serialize(const settings_t *s, uint8_t out[SETTINGS_BLOCK_LEN]) {
    out[0] = SETTINGS_VERSION;
    encode_button(&s->a, out + 1);
    encode_button(&s->b, out + 5);
    out[9] = s->repeat_hz;
    out[10] = (uint8_t)s->repeat_mode;
}

uint16_t settings_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= (uint16_t)(*data++ << 8);
        for (int i = 0; i < 8; i++) crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}

void settings_slot_pack(uint32_t counter, const settings_t *s, uint8_t out[SETTINGS_SLOT_LEN]) {
    for (int i = 0; i < 4; i++) out[i] = (uint8_t)(counter >> (8 * i));
    settings_serialize(s, out + 4);
    wr16(out + 15, settings_crc16(out, 15));
}

/* Valid slot: CRC matches, counter is not the erased value, block passes validation. */
static bool slot_read(const uint8_t *p, settings_t *s, uint32_t *counter) {
    if (settings_crc16(p, 15) != rd16(p + 15)) return false;
    uint32_t c = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    if (c == 0xFFFFFFFFu) return false;
    if (settings_parse(p + 4, SETTINGS_BLOCK_LEN, s) != SETTINGS_OK) return false;
    *counter = c;
    return true;
}

int settings_pick(const uint8_t slot0[SETTINGS_SLOT_LEN], const uint8_t slot1[SETTINGS_SLOT_LEN],
                  settings_t *out, uint32_t *counter) {
    settings_t s0, s1;
    uint32_t c0 = 0, c1 = 0;
    bool v0 = slot_read(slot0, &s0, &c0);
    bool v1 = slot_read(slot1, &s1, &c1);
    if (v0 && (!v1 || c0 >= c1)) {
        *out = s0;
        *counter = c0;
        return 0;
    }
    if (v1) {
        *out = s1;
        *counter = c1;
        return 1;
    }
    settings_defaults(out);
    *counter = 0;
    return -1;
}

int settings_next_slot(int best) { return best == 0 ? 1 : 0; }

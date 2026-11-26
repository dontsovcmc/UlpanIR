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

/* Button record: flags u8, address u16, command u8. 8-bit address must fit in a byte. */
static bool address_ok(const uint8_t *p) {
    return !(p[0] & FLAG_ASSIGNED) || (p[0] & FLAG_EXTENDED) || rd16(p + 1) <= 0xFF;
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
    if (len != SETTINGS_BLOCK_LEN || buf[0] != SETTINGS_VERSION) return SETTINGS_ERR_INVALID;
    if ((buf[1] | buf[5]) & ~(FLAG_ASSIGNED | FLAG_EXTENDED)) return SETTINGS_ERR_INVALID;
    if (!address_ok(buf + 1) || !address_ok(buf + 5)) return SETTINGS_ERR_INVALID;
    if (buf[9] < SETTINGS_REPEAT_HZ_MIN || buf[9] > SETTINGS_REPEAT_HZ_MAX) return SETTINGS_ERR_INVALID;
    if (buf[10] > REPEAT_NEC_CODE) return SETTINGS_ERR_INVALID;

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

void settings_record_pack(const settings_t *s, uint8_t out[SETTINGS_RECORD_LEN]) {
    settings_serialize(s, out);
    wr16(out + SETTINGS_BLOCK_LEN, settings_crc16(out, SETTINGS_BLOCK_LEN));
}

bool settings_record_unpack(const uint8_t in[SETTINGS_RECORD_LEN], settings_t *out) {
    if (settings_crc16(in, SETTINGS_BLOCK_LEN) == rd16(in + SETTINGS_BLOCK_LEN) &&
        settings_parse(in, SETTINGS_BLOCK_LEN, out) == SETTINGS_OK)
        return true;
    settings_defaults(out);
    return false;
}

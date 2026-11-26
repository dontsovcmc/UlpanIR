#include "proto.h"
#include "config.h"
#include <string.h>

/* d5e80001-b2f6-4b31-98c6-7923e11860fc, little-endian; bytes 12-13 carry the 0x000N of the first group. */
static const uint8_t UUID_BASE_LE[16] = {0xfc, 0x60, 0x18, 0xe1, 0x23, 0x79, 0xc6, 0x98,
                                         0x31, 0x4b, 0xf6, 0xb2, 0x00, 0x00, 0xe8, 0xd5};

void proto_uuid(proto_uuid_t which, uint8_t out[16]) {
    memcpy(out, UUID_BASE_LE, 16);
    out[12] = (uint8_t)which;
}

int proto_config_write(const uint8_t *buf, size_t len, settings_t *work, int (*save)(const settings_t *)) {
    settings_t s;
    int err = settings_parse(buf, len, &s);
    if (err != SETTINGS_OK) return err;
    err = save(&s);
    if (err != SETTINGS_OK) return err;
    *work = s;
    return SETTINGS_OK;
}

void proto_config_read(const settings_t *work, uint8_t out[SETTINGS_BLOCK_LEN]) { settings_serialize(work, out); }

void proto_status(uint16_t battery_mv, uint8_t out[PROTO_STATUS_LEN]) {
    out[0] = PROTO_VERSION;
    out[1] = FW_VERSION_MAJOR;
    out[2] = FW_VERSION_MINOR;
    out[3] = FW_VERSION_PATCH;
    out[4] = (uint8_t)battery_mv;
    out[5] = (uint8_t)(battery_mv >> 8);
}

int proto_test_write(const uint8_t *buf, size_t len, const settings_t *work, button_code_t *code) {
    settings_t passed;
    const settings_t *src = work;
    if (len != 1 && len != 1 + SETTINGS_BLOCK_LEN) return SETTINGS_ERR_LENGTH;
    if (len == 1 + SETTINGS_BLOCK_LEN) {
        int err = settings_parse(buf + 1, SETTINGS_BLOCK_LEN, &passed);
        if (err != SETTINGS_OK) return err;
        src = &passed;
    }
    if (buf[0] > 1) return SETTINGS_ERR_BUTTON;
    const button_code_t *c = buf[0] == 0 ? &src->a : &src->b;
    if (!c->assigned) return SETTINGS_ERR_NOT_ASSIGNED;
    *code = *c;
    return SETTINGS_OK;
}

void proto_adv_name(const uint8_t mac[6], char out[PROTO_NAME_LEN + 1]) {
    static const char hex[] = "0123456789ABCDEF";
    memcpy(out, "IR-Remote-", 10);
    out[10] = hex[mac[1] >> 4];
    out[11] = hex[mac[1] & 15];
    out[12] = hex[mac[0] >> 4];
    out[13] = hex[mac[0] & 15];
    out[14] = 0;
}

uint16_t proto_vbat_mv(uint16_t adc) {
    uint32_t v = (uint32_t)adc * 1050u / 512u;
    return v > 3150u ? (uint16_t)(v - 3150u) : 0;
}

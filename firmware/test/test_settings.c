/* Host tests for the settings block v1 and the flash record (one copy + CRC) (docs/ble_protocol.md). */
#include "settings.h"
#include <stdio.h>
#include <string.h>

static int fails;
#define CHECK(c)                                                    \
    do {                                                            \
        if (!(c)) {                                                 \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);    \
            fails++;                                                \
        }                                                           \
    } while (0)

/* Example from docs/ble_protocol.md: A = 0x32/0x02, B = 0x32/0x03, 3 frames/s, full frame. */
static const uint8_t EXAMPLE[SETTINGS_BLOCK_LEN] = {0x01, 0x01, 0x32, 0x00, 0x02, 0x01, 0x32, 0x00, 0x03, 0x03, 0x00};

static void test_defaults(void) {
    settings_t s;
    settings_defaults(&s);
    CHECK(!s.a.assigned && !s.b.assigned);
    CHECK(s.repeat_hz == 3);
    CHECK(s.repeat_mode == REPEAT_FULL_FRAME);
}

static void test_parse_example(void) {
    settings_t s;
    CHECK(settings_parse(EXAMPLE, sizeof(EXAMPLE), &s) == SETTINGS_OK);
    CHECK(s.a.assigned && !s.a.extended && s.a.address == 0x32 && s.a.command == 0x02);
    CHECK(s.b.assigned && s.b.address == 0x32 && s.b.command == 0x03);
    CHECK(s.repeat_hz == 3 && s.repeat_mode == REPEAT_FULL_FRAME);
}

static void test_roundtrip_extended(void) {
    settings_t s, t;
    settings_defaults(&s);
    s.a = (button_code_t){.assigned = true, .extended = true, .address = 0xBEEF, .command = 0x7A};
    s.repeat_hz = 10;
    s.repeat_mode = REPEAT_NEC_CODE;
    uint8_t buf[SETTINGS_BLOCK_LEN];
    settings_serialize(&s, buf);
    CHECK(buf[1] == 0x03 && buf[2] == 0xEF && buf[3] == 0xBE && buf[4] == 0x7A);
    CHECK(settings_parse(buf, sizeof(buf), &t) == SETTINGS_OK);
    CHECK(memcmp(&s, &t, sizeof(s)) == 0);
}

static void test_serialize_example(void) {
    settings_t s;
    settings_parse(EXAMPLE, sizeof(EXAMPLE), &s);
    uint8_t buf[SETTINGS_BLOCK_LEN];
    settings_serialize(&s, buf);
    CHECK(memcmp(buf, EXAMPLE, sizeof(buf)) == 0);
}

static int parse_mod(int off, uint8_t val) {
    uint8_t b[SETTINGS_BLOCK_LEN];
    memcpy(b, EXAMPLE, sizeof(b));
    b[off] = val;
    settings_t s;
    return settings_parse(b, sizeof(b), &s);
}

static void test_validation(void) {
    settings_t s;
    settings_defaults(&s);
    settings_t before = s;
    CHECK(settings_parse(EXAMPLE, 10, &s) == SETTINGS_ERR_LENGTH);
    CHECK(settings_parse(EXAMPLE, 12, &s) == SETTINGS_ERR_LENGTH);
    CHECK(memcmp(&s, &before, sizeof(s)) == 0); /* rejected: output untouched */
    CHECK(parse_mod(0, 2) == SETTINGS_ERR_VERSION);
    CHECK(parse_mod(1, 0x05) == SETTINGS_ERR_FLAGS);
    CHECK(parse_mod(5, 0x81) == SETTINGS_ERR_FLAGS);
    CHECK(parse_mod(3, 0x01) == SETTINGS_ERR_ADDRESS); /* A: address 0x0132 without 16-bit flag */
    CHECK(parse_mod(9, 0) == SETTINGS_ERR_REPEAT_HZ);
    CHECK(parse_mod(9, 11) == SETTINGS_ERR_REPEAT_HZ);
    CHECK(parse_mod(9, 50) == SETTINGS_ERR_REPEAT_HZ); /* spec scenario: 50 frames/s rejected */
    CHECK(parse_mod(9, 1) == SETTINGS_OK);
    CHECK(parse_mod(9, 10) == SETTINGS_OK);
    CHECK(parse_mod(10, 2) == SETTINGS_ERR_REPEAT_MODE);
    /* unassigned button: address and command are not checked */
    uint8_t b[SETTINGS_BLOCK_LEN];
    memcpy(b, EXAMPLE, sizeof(b));
    b[5] = 0x00;
    b[7] = 0x12;
    CHECK(settings_parse(b, sizeof(b), &s) == SETTINGS_OK && !s.b.assigned);
    /* checks run in the documented order: version before repeat_hz */
    memcpy(b, EXAMPLE, sizeof(b));
    b[0] = 9;
    b[9] = 0;
    CHECK(settings_parse(b, sizeof(b), &s) == SETTINGS_ERR_VERSION);
}

static void test_crc(void) {
    /* CRC-16/CCITT-FALSE check value */
    CHECK(settings_crc16((const uint8_t *)"123456789", 9) == 0x29B1);
}

static void test_record(void) {
    settings_t s, out;
    settings_parse(EXAMPLE, sizeof(EXAMPLE), &s);
    uint8_t rec[SETTINGS_RECORD_LEN], erased[SETTINGS_RECORD_LEN];
    memset(erased, 0xFF, sizeof(erased));

    /* clean flash: not valid, defaults (nothing assigned) */
    CHECK(!settings_record_unpack(erased, &out));
    CHECK(!out.a.assigned && !out.b.assigned && out.repeat_hz == 3);

    /* layout: block, then CRC LE over the block */
    settings_record_pack(&s, rec);
    CHECK(memcmp(rec, EXAMPLE, SETTINGS_BLOCK_LEN) == 0);
    uint16_t c = settings_crc16(rec, SETTINGS_BLOCK_LEN);
    CHECK(rec[11] == (uint8_t)c && rec[12] == (uint8_t)(c >> 8));
    CHECK(settings_record_unpack(rec, &out) && memcmp(&out, &s, sizeof(s)) == 0);

    /* interrupted write: the tail is still erased -> clean remote, never a mix of old and new codes */
    uint8_t torn[SETTINGS_RECORD_LEN];
    memcpy(torn, rec, sizeof(torn));
    memset(torn + 5, 0xFF, SETTINGS_RECORD_LEN - 5);
    CHECK(!settings_record_unpack(torn, &out) && !out.a.assigned && !out.b.assigned);

    /* single corrupted bit */
    memcpy(torn, rec, sizeof(torn));
    torn[3] ^= 0x01;
    CHECK(!settings_record_unpack(torn, &out) && !out.a.assigned);

    /* matching CRC but a block this firmware does not accept (future version) */
    memcpy(torn, rec, sizeof(torn));
    torn[0] = 2;
    c = settings_crc16(torn, SETTINGS_BLOCK_LEN);
    torn[11] = (uint8_t)c;
    torn[12] = (uint8_t)(c >> 8);
    CHECK(!settings_record_unpack(torn, &out) && !out.a.assigned);
}

int main(void) {
    test_defaults();
    test_parse_example();
    test_roundtrip_extended();
    test_serialize_example();
    test_validation();
    test_crc();
    test_record();
    if (fails) {
        printf("test_settings: %d failure(s)\n", fails);
        return 1;
    }
    printf("test_settings: ok\n");
    return 0;
}

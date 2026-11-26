/* Host tests: BLE characteristic logic of docs/ble_protocol.md (config, status, test, name, VBAT). */
#include "proto.h"
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

static const uint8_t EXAMPLE[SETTINGS_BLOCK_LEN] = {0x01, 0x01, 0x32, 0x00, 0x02, 0x01, 0x32, 0x00, 0x03, 0x03, 0x00};

static int save_calls, save_result;
static settings_t saved;
static int fake_save(const settings_t *s) {
    save_calls++;
    saved = *s;
    return save_result;
}

static void test_uuid_le(void) {
    /* d5e80001-b2f6-4b31-98c6-7923e11860fc, little-endian as the stack stores it */
    static const uint8_t want[16] = {0xfc, 0x60, 0x18, 0xe1, 0x23, 0x79, 0xc6, 0x98,
                                     0x31, 0x4b, 0xf6, 0xb2, 0x01, 0x00, 0xe8, 0xd5};
    uint8_t u[16];
    proto_uuid(PROTO_UUID_SERVICE, u);
    CHECK(memcmp(u, want, 16) == 0);
    proto_uuid(PROTO_UUID_TEST, u);
    CHECK(u[12] == 0x05 && u[13] == 0x00 && u[15] == 0xd5);
}

static void test_config_write_ok(void) {
    settings_t work;
    settings_defaults(&work);
    save_calls = 0;
    save_result = SETTINGS_OK;
    CHECK(proto_config_write(EXAMPLE, sizeof(EXAMPLE), &work, fake_save) == SETTINGS_OK);
    CHECK(save_calls == 1 && saved.a.command == 0x02 && work.a.assigned && work.b.command == 0x03);
}

static void test_config_write_invalid_keeps_settings(void) {
    settings_t work;
    settings_defaults(&work);
    settings_t before = work;
    uint8_t b[SETTINGS_BLOCK_LEN];
    memcpy(b, EXAMPLE, sizeof(b));
    b[9] = 50; /* spec: 50 frames/s is rejected */
    save_calls = 0;
    CHECK(proto_config_write(b, sizeof(b), &work, fake_save) == SETTINGS_ERR_INVALID);
    CHECK(save_calls == 0 && memcmp(&work, &before, sizeof(work)) == 0);
}

static void test_config_write_flash_error_keeps_settings(void) {
    settings_t work;
    settings_defaults(&work);
    settings_t before = work;
    save_result = SETTINGS_ERR_FLASH;
    CHECK(proto_config_write(EXAMPLE, sizeof(EXAMPLE), &work, fake_save) == SETTINGS_ERR_FLASH);
    CHECK(memcmp(&work, &before, sizeof(work)) == 0);
    save_result = SETTINGS_OK;
}

static void test_config_read(void) {
    settings_t work;
    settings_parse(EXAMPLE, sizeof(EXAMPLE), &work);
    uint8_t out[SETTINGS_BLOCK_LEN];
    proto_config_read(&work, out);
    CHECK(memcmp(out, EXAMPLE, sizeof(out)) == 0);
}

static void test_status(void) {
    uint8_t out[PROTO_STATUS_LEN];
    proto_status(2873, out);
    CHECK(out[0] == 1 && out[1] == 0 && out[2] == 2 && out[3] == 0); /* proto 1, fw 0.2.0 */
    CHECK(out[4] == (2873 & 0xFF) && out[5] == (2873 >> 8));
}

static void test_test_write(void) {
    settings_t work;
    settings_defaults(&work); /* nothing saved yet */
    button_code_t code;
    /* 1 byte: current working settings -> A not assigned */
    uint8_t one[1] = {0};
    CHECK(proto_test_write(one, 1, &work, &code) == SETTINGS_ERR_INVALID);
    /* 12 bytes: code from the page's unsaved block */
    uint8_t twelve[12];
    twelve[0] = 1;
    memcpy(twelve + 1, EXAMPLE, SETTINGS_BLOCK_LEN);
    CHECK(proto_test_write(twelve, 12, &work, &code) == SETTINGS_OK);
    CHECK(code.assigned && code.address == 0x32 && code.command == 0x03);
    CHECK(!work.a.assigned); /* nothing stored */
    /* errors */
    twelve[0] = 2;
    CHECK(proto_test_write(twelve, 12, &work, &code) == SETTINGS_ERR_INVALID);
    CHECK(proto_test_write(twelve, 5, &work, &code) == SETTINGS_ERR_INVALID);
    twelve[0] = 0;
    twelve[10] = 0; /* repeat_hz 0 in the passed block */
    CHECK(proto_test_write(twelve, 12, &work, &code) == SETTINGS_ERR_INVALID);
}

static void test_adv_name(void) {
    /* MAC 11:22:33:44:AB:CD stored LSB first: last 4 hex digits "ABCD" */
    const uint8_t mac[6] = {0xCD, 0xAB, 0x44, 0x33, 0x22, 0x11};
    char name[PROTO_NAME_LEN + 1];
    proto_adv_name(mac, name);
    CHECK(strcmp(name, "IR-Remote-ABCD") == 0);
}

static void test_vbat_mv(void) {
    /* WCH PDF_DataLogger BAT.h: 2755 -> 2.5 V, 2706 -> 2.4 V, 2658 -> 2.3 V, 2024 -> 1.0 V (-12 dB gain) */
    CHECK(proto_vbat_mv(2755) >= 2495 && proto_vbat_mv(2755) <= 2505);
    CHECK(proto_vbat_mv(2706) >= 2395 && proto_vbat_mv(2706) <= 2405);
    CHECK(proto_vbat_mv(2658) >= 2295 && proto_vbat_mv(2658) <= 2305);
    CHECK(proto_vbat_mv(2024) >= 995 && proto_vbat_mv(2024) <= 1005);
    CHECK(proto_vbat_mv(0) == 0); /* below the range: clamp, no wrap-around */
}

int main(void) {
    test_uuid_le();
    test_config_write_ok();
    test_config_write_invalid_keeps_settings();
    test_config_write_flash_error_keeps_settings();
    test_config_read();
    test_status();
    test_test_write();
    test_adv_name();
    test_vbat_mv();
    if (fails) {
        printf("test_proto: %d failure(s)\n", fails);
        return 1;
    }
    printf("test_proto: ok\n");
    return 0;
}

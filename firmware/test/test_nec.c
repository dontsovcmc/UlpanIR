/* Host unit tests for the NEC encoder (no hardware). Build: make -C firmware test */
#include <stdio.h>
#include <string.h>
#include "nec.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

/* durations are in half-microseconds: 562.5 us = 1125 */
#define T_LEAD_MARK 18000
#define T_LEAD_SPACE 9000
#define T_REP_SPACE 4500
#define T_BIT_MARK 1125
#define T_ZERO 1125
#define T_ONE 3375

static void test_bytes_standard(void) {
    uint8_t b[4];
    nec_bytes(0x32, 0x02, false, b);
    CHECK(b[0] == 0x32 && b[1] == 0xCD && b[2] == 0x02 && b[3] == 0xFD);
}

static void test_bytes_extended(void) {
    uint8_t b[4];
    nec_bytes(0x1234, 0x02, true, b);
    CHECK(b[0] == 0x34 && b[1] == 0x12 && b[2] == 0x02 && b[3] == 0xFD);
}

static void test_frame_layout(void) {
    nec_frame_t f;
    CHECK(nec_encode(0x32, 0x02, false, &f) == 0);
    CHECK(f.count == 67);
    CHECK(f.half_us[0] == T_LEAD_MARK && f.half_us[1] == T_LEAD_SPACE);
    /* byte 0 = 0x32 = 0b00110010, LSB first: 0 1 0 0 1 1 0 0 */
    const int bits0[8] = {0, 1, 0, 0, 1, 1, 0, 0};
    for (int i = 0; i < 8; i++) {
        CHECK(f.half_us[2 + 2 * i] == T_BIT_MARK);
        CHECK(f.half_us[3 + 2 * i] == (bits0[i] ? T_ONE : T_ZERO));
    }
    CHECK(f.half_us[66] == T_BIT_MARK); /* stop mark */
}

static void test_frame_all_bits_match_bytes(void) {
    nec_frame_t f;
    uint8_t b[4];
    nec_encode(0x5A, 0xC3, false, &f);
    nec_bytes(0x5A, 0xC3, false, b);
    for (int k = 0; k < 32; k++) {
        int bit = (b[k / 8] >> (k % 8)) & 1;
        CHECK(f.half_us[3 + 2 * k] == (bit ? T_ONE : T_ZERO));
    }
}

static void test_frame_total_length(void) {
    /* 9 + 4.5 + 16 ones/16 zeros (equal count for any standard NEC) = 67.5 ms, + stop mark 0.5625 ms */
    nec_frame_t f;
    nec_encode(0x00, 0x00, false, &f);
    uint32_t sum = 0;
    for (int i = 0; i < f.count; i++) sum += f.half_us[i];
    CHECK(sum == 136125);
}

static void test_repeat_frame(void) {
    nec_frame_t f;
    CHECK(nec_encode_repeat(&f) == 0);
    CHECK(f.count == 3);
    CHECK(f.half_us[0] == T_LEAD_MARK && f.half_us[1] == T_REP_SPACE && f.half_us[2] == T_BIT_MARK);
}

static void test_null_output_rejected(void) {
    CHECK(nec_encode(0x32, 0x02, false, NULL) != 0);
    CHECK(nec_encode_repeat(NULL) != 0);
}

int main(void) {
    test_bytes_standard();
    test_bytes_extended();
    test_frame_layout();
    test_frame_all_bits_match_bytes();
    test_frame_total_length();
    test_repeat_frame();
    test_null_output_rejected();
    printf(fails ? "%d FAILED\n" : "all nec tests passed\n", fails);
    return fails != 0;
}

#include "nec.h"
#include <stddef.h>

/* NEC timings in half-microseconds */
#define LEAD_MARK 18000  /* 9 ms */
#define LEAD_SPACE 9000  /* 4.5 ms */
#define REP_SPACE 4500   /* 2.25 ms */
#define BIT_MARK 1125    /* 562.5 us */
#define ZERO_SPACE 1125  /* 562.5 us */
#define ONE_SPACE 3375   /* 1687.5 us */

void nec_bytes(uint16_t address, uint8_t command, bool extended, uint8_t out[4]) {
    out[0] = (uint8_t)(address & 0xFF);
    out[1] = extended ? (uint8_t)(address >> 8) : (uint8_t)~out[0];
    out[2] = command;
    out[3] = (uint8_t)~command;
}

int nec_encode(uint16_t address, uint8_t command, bool extended, nec_frame_t *out) {
    if (out == NULL) return -1;
    uint8_t b[4];
    nec_bytes(address, command, extended, b);
    uint8_t n = 0;
    out->half_us[n++] = LEAD_MARK;
    out->half_us[n++] = LEAD_SPACE;
    for (int k = 0; k < 32; k++) {
        out->half_us[n++] = BIT_MARK;
        out->half_us[n++] = ((b[k / 8] >> (k % 8)) & 1) ? ONE_SPACE : ZERO_SPACE;
    }
    out->half_us[n++] = BIT_MARK;
    out->count = n;
    return 0;
}

int nec_encode_repeat(nec_frame_t *out) {
    if (out == NULL) return -1;
    out->half_us[0] = LEAD_MARK;
    out->half_us[1] = REP_SPACE;
    out->half_us[2] = BIT_MARK;
    out->count = 3;
    return 0;
}

#include <stdio.h>
#include "ir_timing.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void test_carrier_38khz_at_60mhz(void) {
    ir_carrier_t c = ir_carrier_ticks(60000000UL, 38000UL);
    double f = 60e6 / c.period;
    CHECK(f > 38000 * 0.99 && f < 38000 * 1.01); /* spec: 38 kHz +-1 % */
    double duty = (double)c.active / c.period;
    CHECK(duty > 0.32 && duty < 0.345);           /* spec: duty 1/3 */
}

static void test_half_us_to_ticks(void) {
    CHECK(ir_half_us_to_ticks(60000000UL, 1125) == 33750);   /* 562.5 us at 60 MHz */
    CHECK(ir_half_us_to_ticks(60000000UL, 18000) == 540000); /* 9 ms */
    CHECK(ir_half_us_to_ticks(32000000UL, 1125) == 18000);   /* 562.5 us at 32 MHz */
}

static void test_longest_segment_fits_26bit_timer(void) {
    CHECK(ir_half_us_to_ticks(60000000UL, 18000) < (1UL << 26));
}

int main(void) {
    test_carrier_38khz_at_60mhz();
    test_half_us_to_ticks();
    test_longest_segment_fits_26bit_timer();
    printf(fails ? "%d FAILED\n" : "all ir_timing tests passed\n", fails);
    return fails != 0;
}

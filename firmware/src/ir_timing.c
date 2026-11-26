#include "ir_timing.h"

ir_carrier_t ir_carrier_ticks(uint32_t fsys_hz, uint32_t carrier_hz) {
    ir_carrier_t c;
    c.period = (fsys_hz + carrier_hz / 2) / carrier_hz; /* rounded */
    c.active = (c.period + 1) / 3;                      /* duty 1/3 */
    return c;
}

uint32_t ir_half_us_to_ticks(uint32_t fsys_hz, uint16_t half_us) {
    return (uint32_t)(((uint64_t)fsys_hz * half_us) / 2000000ULL);
}

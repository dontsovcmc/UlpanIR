/* Timer tick arithmetic for the IR carrier and NEC envelope (pure, host-testable). */
#ifndef IR_TIMING_H
#define IR_TIMING_H
#include <stdint.h>
typedef struct { uint32_t period; uint32_t active; } ir_carrier_t;
/* PWM period and active width (duty 1/3) in timer ticks for carrier_hz at fsys_hz. */
ir_carrier_t ir_carrier_ticks(uint32_t fsys_hz, uint32_t carrier_hz);
/* Duration in half-microseconds -> timer ticks at fsys_hz. */
uint32_t ir_half_us_to_ticks(uint32_t fsys_hz, uint16_t half_us);
#endif

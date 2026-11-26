/* NEC IR frame encoder: pure, hardware independent.
 * A frame is a sequence of durations, alternating mark (carrier on) and space,
 * starting with a mark. Units: half-microseconds (562.5 us = 1125). */
#ifndef NEC_H
#define NEC_H
#include <stdbool.h>
#include <stdint.h>

#define NEC_MAX_DURATIONS 67 /* lead mark+space, 32 bits x (mark+space), stop mark */

typedef struct {
    uint16_t half_us[NEC_MAX_DURATIONS];
    uint8_t count;
} nec_frame_t;

/* Four bytes on air. Standard: addr, ~addr, cmd, ~cmd (address low 8 bits).
 * Extended: addr low byte, addr high byte, cmd, ~cmd. */
void nec_bytes(uint16_t address, uint8_t command, bool extended, uint8_t out[4]);

/* Full frame. Returns 0 on success, -1 on NULL output. */
int nec_encode(uint16_t address, uint8_t command, bool extended, nec_frame_t *out);

/* Standard NEC repeat: 9 ms mark, 2.25 ms space, 562.5 us mark. */
int nec_encode_repeat(nec_frame_t *out);

#endif

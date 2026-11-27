/* Power latch decisions (spec add-power-latch): pure logic, called every 1 ms. Hardware independent.
 * Button released (no button down for debounce_ms) and the frame in progress finished -> switch off.
 * Low battery: before switching off the indicator fades out once. A new press keeps or restores power. */
#ifndef POWER_H
#define POWER_H
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PWR_EV_NONE = 0,
    PWR_EV_OFF,  /* release the latch (HOLD low) */
    PWR_EV_FADE, /* low battery: run the fade-out, then call power_fade_done() */
    PWR_EV_ON,   /* a button was pressed again after OFF: hold the latch again */
} power_event_t;

typedef struct {
    uint8_t debounce_ms;
    bool low_batt;
    uint32_t last_down; /* last tick with any button down */
    bool off;           /* latch released */
    bool faded;         /* fade-out already shown since the last press */
} power_t;

void power_init(power_t *p, uint8_t debounce_ms, bool low_batt);
/* any_down: raw state of the buttons; ir_busy: a frame is still being sent. */
power_event_t power_tick(power_t *p, uint32_t now_ms, bool any_down, bool ir_busy);
/* Fade-out finished (completed = true) or was cut short by a new press (completed = false). */
power_event_t power_fade_done(power_t *p, bool completed);

/* Indicator brightness during the fade-out, 1000 (full) at t = 0 down to 0 at t >= total_ms;
 * quadratic, so the eye sees an even fade. */
uint16_t power_fade_level(uint32_t t_ms, uint32_t total_ms);

/* Battery below this at power-on -> fade-out after release (spec "Сигнал севшей батарейки"). */
#define LOW_BATT_MV 2400
#define FADE_MS     2000
#endif

#include "power.h"
#include <string.h>

void power_init(power_t *p, uint8_t debounce_ms, bool low_batt) {
    memset(p, 0, sizeof(*p));
    p->debounce_ms = debounce_ms;
    p->low_batt = low_batt;
}

power_event_t power_tick(power_t *p, uint32_t now_ms, bool any_down, bool ir_busy) {
    if (any_down) {
        p->last_down = now_ms;
        p->faded = false;
        if (p->off) {
            p->off = false;
            return PWR_EV_ON;
        }
        return PWR_EV_NONE;
    }
    if (p->off || ir_busy || now_ms - p->last_down < p->debounce_ms) return PWR_EV_NONE;
    if (p->low_batt && !p->faded) {
        p->faded = true;
        return PWR_EV_FADE;
    }
    p->off = true;
    return PWR_EV_OFF;
}

power_event_t power_fade_done(power_t *p, bool completed) {
    if (!completed) {
        p->faded = false; /* fade again after the next release */
        return PWR_EV_NONE;
    }
    p->off = true;
    return PWR_EV_OFF;
}

uint16_t power_fade_level(uint32_t t_ms, uint32_t total_ms) {
    if (t_ms >= total_ms) return 0;
    uint32_t r = total_ms - t_ms; /* remaining */
    return (uint16_t)((uint64_t)1000 * r * r / ((uint64_t)total_ms * total_ms));
}

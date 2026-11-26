#include "dispatch.h"

uint16_t dispatch_period_ms(uint8_t repeat_hz) {
    if (repeat_hz == 0) repeat_hz = 1; /* settings_parse never lets 0 through; guard anyway */
    return (uint16_t)((1000u + repeat_hz / 2u) / repeat_hz);
}

void dispatch_reset(dispatch_t *d) { d->unassigned_shown = false; }

void dispatch_released(dispatch_t *d) { d->unassigned_shown = false; }

out_t dispatch_action(dispatch_t *d, const settings_t *s, action_t a) {
    out_t o = {OUT_NONE, {0}};
    if (a.kind == ACT_NONE || a.button == BTN_NONE) return o;
    const button_code_t *code = (a.button == BTN_A) ? &s->a : &s->b;
    if (!code->assigned) {
        /* no IR at all; one slow blink series per press, repeats while held are swallowed */
        if (!d->unassigned_shown) {
            d->unassigned_shown = true;
            o.kind = OUT_UNASSIGNED;
        }
        return o;
    }
    if (a.kind == ACT_SEND_REPEAT) {
        o.kind = OUT_REPEAT;
    } else {
        o.kind = OUT_FRAME;
        o.code = *code;
    }
    return o;
}

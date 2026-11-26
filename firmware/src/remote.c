#include "remote.h"
#include <string.h>

void remote_init(remote_t *r, const remote_cfg_t *cfg) {
    memset(r, 0, sizeof(*r));
    r->cfg = *cfg;
}

static void debounce(bool raw, bool *last_raw, bool *deb, uint32_t *changed, uint32_t now, uint8_t ms) {
    if (raw != *last_raw) {
        *last_raw = raw;
        *changed = now;
    }
    if (*last_raw != *deb && now - *changed >= ms) *deb = *last_raw;
}

action_t remote_tick(remote_t *r, uint32_t now_ms, bool a_down, bool b_down) {
    action_t act = {ACT_NONE, BTN_NONE};
    debounce(a_down, &r->raw_a, &r->deb_a, &r->changed_a, now_ms, r->cfg.debounce_ms);
    debounce(b_down, &r->raw_b, &r->deb_b, &r->changed_b, now_ms, r->cfg.debounce_ms);

    /* Both buttons: never IR (spec remote-config); held config_hold_ms -> config mode. */
    if (r->deb_a && r->deb_b) {
        if (!r->both) {
            r->both = true;
            r->both_since = now_ms;
            r->config_sent = false;
            if (r->active != BTN_NONE) act.kind = ACT_STOP;
            r->active = BTN_NONE;
        }
        if (!r->config_sent && r->cfg.config_hold_ms && now_ms - r->both_since >= r->cfg.config_hold_ms) {
            r->config_sent = true;
            act.kind = ACT_CONFIG;
        }
        return act;
    }
    if (r->both) {
        r->both = false;
        r->wait_release = r->deb_a || r->deb_b; /* the remaining button must be released first */
        return act;
    }

    if (r->wait_release) {
        if (!r->deb_a && !r->deb_b) r->wait_release = false;
        return act;
    }
    if (r->active == BTN_NONE) {
        if (r->deb_a == r->deb_b) return act; /* nothing pressed, or both: not a send gesture */
        r->active = r->deb_a ? BTN_A : BTN_B;
        r->last_start = now_ms;
        act.kind = ACT_SEND_FRAME;
        act.button = r->active;
        return act;
    }

    bool held = (r->active == BTN_A) ? r->deb_a : r->deb_b;
    if (!held) {
        r->active = BTN_NONE;
        r->wait_release = r->deb_a || r->deb_b; /* new press only after both released */
        return act;
    }
    if (now_ms - r->last_start >= r->cfg.period_ms) {
        r->last_start = now_ms;
        act.kind = r->cfg.nec_repeat ? ACT_SEND_REPEAT : ACT_SEND_FRAME;
        act.button = r->active;
    }
    return act;
}

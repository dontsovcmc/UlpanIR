/* Button handling and frame scheduling: pure state machine, called every 1 ms.
 * Hardware independent; the caller reads buttons and performs the returned action. */
#ifndef REMOTE_H
#define REMOTE_H
#include <stdbool.h>
#include <stdint.h>

typedef enum { BTN_NONE = 0, BTN_A, BTN_B } button_t;
typedef enum { ACT_NONE = 0, ACT_SEND_FRAME, ACT_SEND_REPEAT } action_kind_t;
typedef struct { action_kind_t kind; button_t button; } action_t;

typedef struct {
    uint16_t period_ms;  /* repeat period while held (333 = 3 frames/s) */
    bool nec_repeat;     /* false: repeat the full frame; true: NEC repeat code */
    uint8_t debounce_ms; /* level must be stable this long */
} remote_cfg_t;

typedef struct {
    remote_cfg_t cfg;
    bool raw_a, raw_b, deb_a, deb_b;
    uint32_t changed_a, changed_b;
    button_t active;
    bool wait_release; /* sequence ended: ignore buttons until both are released */
    uint32_t last_start;
} remote_t;

void remote_init(remote_t *r, const remote_cfg_t *cfg);
action_t remote_tick(remote_t *r, uint32_t now_ms, bool a_down, bool b_down);
#endif

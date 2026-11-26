/* UlpanIR: button A/B -> NEC frame with repeats while held; codes from DataFlash settings. */
#include "board.h"
#include "config.h"
#include "dispatch.h"
#include "ir_tx.h"
#include "nec.h"
#include "remote.h"
#include "settings_store.h"

static nec_frame_t frame;

static void send(const out_t *o) {
    if (o->kind == OUT_REPEAT) {
        nec_encode_repeat(&frame);
    } else {
        nec_encode(o->code.address, o->code.command, o->code.extended, &frame);
    }
    ir_tx_start(&frame);
}

/* Button without a code: no IR, slow blink 3 times (500 ms on, 500 ms off).
 * With the power latch (add-power-latch) the remote switches off after this. */
static void blink_unassigned(void) {
    for (int i = 0; i < 3; i++) {
        led_set(1);
        mDelaymS(500);
        led_set(0);
        mDelaymS(500);
    }
}

int main(void) {
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    board_init();
    ir_tx_init();

    settings_t settings;
    settings_load(&settings);
#if defined(TEST_CODES) && TEST_CODES
    if (!settings.a.assigned && !settings.b.assigned) {
        settings.a = (button_code_t){true, false, TEST_CODE_A_ADDRESS, TEST_CODE_A_COMMAND};
        settings.b = (button_code_t){true, false, TEST_CODE_B_ADDRESS, TEST_CODE_B_COMMAND};
    }
#endif

    const remote_cfg_t cfg = {dispatch_period_ms(settings.repeat_hz), settings.repeat_mode == REPEAT_NEC_CODE,
                              DEBOUNCE_MS};
    remote_t remote;
    remote_init(&remote, &cfg);
    dispatch_t disp;
    dispatch_reset(&disp);

    uint32_t now = 0;
    for (;;) {
        mDelaymS(1);
        now++;
        action_t a = remote_tick(&remote, now, btn_a_down(), btn_b_down());
        if (remote.active == BTN_NONE) dispatch_released(&disp);
        out_t o = dispatch_action(&disp, &settings, a);
        if (o.kind == OUT_UNASSIGNED) {
            blink_unassigned();
            now += 3000; /* keep the debounce and repeat clocks consistent with the time spent blinking */
        } else if (o.kind != OUT_NONE && !ir_tx_busy()) {
            /* a frame lasts <= 68 ms, below the shortest 100 ms period: busy here means a missed tick */
            send(&o);
        }
    }
}

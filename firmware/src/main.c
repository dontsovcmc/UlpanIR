/* UlpanIR: button A/B -> NEC frame with repeats while held (add-ir-nec-transmit). */
#include "board.h"
#include "config.h"
#include "ir_tx.h"
#include "nec.h"
#include "remote.h"

static nec_frame_t frame;

static void send(const action_t *a) {
    if (a->kind == ACT_SEND_REPEAT) {
        nec_encode_repeat(&frame);
    } else if (a->button == BTN_A) {
        nec_encode(CODE_A_ADDRESS, CODE_A_COMMAND, CODE_EXTENDED, &frame);
    } else {
        nec_encode(CODE_B_ADDRESS, CODE_B_COMMAND, CODE_EXTENDED, &frame);
    }
    ir_tx_start(&frame);
}

int main(void) {
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    board_init();
    ir_tx_init();

    static const remote_cfg_t cfg = {REPEAT_PERIOD_MS, REPEAT_NEC_CODE, DEBOUNCE_MS};
    remote_t remote;
    remote_init(&remote, &cfg);

    uint32_t now = 0;
    for (;;) {
        mDelaymS(1);
        now++;
        action_t a = remote_tick(&remote, now, btn_a_down(), btn_b_down());
        /* a frame lasts <= 68 ms, far below the 333 ms period: busy here means a missed tick */
        if (a.kind != ACT_NONE && !ir_tx_busy()) send(&a);
    }
}

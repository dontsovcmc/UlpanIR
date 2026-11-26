/* Toolchain smoke test (add-ir-nec-transmit 1.2): the board LED blinks at 1 Hz. */
#include "board.h"

int main(void) {
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    board_init();
    for (;;) {
        led_set(1);
        mDelaymS(500);
        led_set(0);
        mDelaymS(500);
    }
}

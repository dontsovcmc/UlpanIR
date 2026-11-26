#include "ir_tx.h"
#include "board.h"
#include "config.h"
#include "ir_timing.h"

static uint32_t seg_ticks[NEC_MAX_DURATIONS];
static volatile uint8_t seg_count, seg_idx;
static volatile bool busy;

static inline void carrier(bool on) {
    if (on) TMR1_PWMEnable(); else TMR1_PWMDisable(); /* disabled: pin falls back to GPIO low */
}

void ir_tx_init(void) {
    if (IR_PIN_REMAP) GPIOPinRemap(ENABLE, RB_PIN_TMR1);
    PIN_CLR(IR_PORT_B, IR_PIN);
    PIN_OUT_PP(IR_PORT_B, IR_PIN);

    ir_carrier_t c = ir_carrier_ticks(FREQ_SYS, IR_CARRIER_HZ);
    TMR1_PWMInit(High_Level, PWM_Times_1);
    TMR1_PWMCycleCfg(c.period);
    TMR1_PWMActDataWidth(c.active);
    carrier(false);
    TMR1_Enable(); /* carrier runs continuously, output is gated */

    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    PFIC_EnableIRQ(TMR0_IRQn);
}

bool ir_tx_busy(void) { return busy; }

bool ir_tx_start(const nec_frame_t *f) {
    if (busy || f == 0 || f->count == 0) return false;
    for (uint8_t i = 0; i < f->count; i++) seg_ticks[i] = ir_half_us_to_ticks(FREQ_SYS, f->half_us[i]);
    seg_count = f->count;
    seg_idx = 0;
    busy = true;
    led_set(1);
    carrier(true); /* segment 0 is a mark */
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    TMR0_TimerInit(seg_ticks[0]);
    return true;
}

__INTERRUPT __HIGH_CODE void TMR0_IRQHandler(void) {
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    uint8_t i = seg_idx + 1;
    if (i < seg_count) {
        R32_TMR0_CNT_END = seg_ticks[i];
        carrier((i & 1) == 0); /* even segments are marks */
        seg_idx = i;
    } else {
        carrier(false);
        TMR0_Disable();
        led_set(0);
        busy = false;
    }
}

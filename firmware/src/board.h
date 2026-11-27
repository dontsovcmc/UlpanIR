/* Pin map. Select with -DBOARD_ULPANIR (our PCB, hardware/README.md §1) or -DBOARD_WEACT
 * (WeAct CH592F Core Board, docs/ref_WeAct_CH592F_CoreBoard_sch.pdf). */
#ifndef BOARD_H
#define BOARD_H
#include "CH59x_common.h"

#if defined(BOARD_ULPANIR)
/* IR carrier: TMR1 PWM on PA10 -> R4 -> gate of Q3 */
#define IR_PIN_REMAP     0              /* TMR1 on PA10 */
#define IR_PORT_B        0
#define IR_PIN           GPIO_Pin_10
/* Indicator LED3: PB4 -> R5 -> LED anode, active high */
#define LED_PORT_B       1
#define LED_PIN          GPIO_Pin_4
#define LED_ACTIVE_HIGH  1
/* Buttons via Schottky to MCU, internal pull-up, pressed = low */
#define BTN_A_PORT_B     0
#define BTN_A_PIN        GPIO_Pin_4     /* PA4 */
#define BTN_B_PORT_B     1
#define BTN_B_PIN        GPIO_Pin_7     /* PB7 */
/* Power latch hold: PA15 -> gate of Q2; high keeps the board powered */
#define HAS_HOLD         1
#define HOLD_PIN         GPIO_Pin_15    /* PA15 */

#elif defined(BOARD_WEACT)
/* PA10/PA11 carry the 32.768 kHz crystal on this board: use the TMR1 alternate pin PB10 */
#define IR_PIN_REMAP     1
#define IR_PORT_B        1
#define IR_PIN           GPIO_Pin_10
/* Blue LED D2: VDD33 -> R1 -> LED -> PA8, active low */
#define LED_PORT_B       0
#define LED_PIN          GPIO_Pin_8
#define LED_ACTIVE_HIGH  0
/* Button A: BOOT key K2 on PB22 to GND; button B: external key on PA4 to GND */
#define BTN_A_PORT_B     1
#define BTN_A_PIN        GPIO_Pin_22
#define BTN_B_PORT_B     0
#define BTN_B_PIN        GPIO_Pin_4
#define HAS_HOLD         0

#else
#error "define BOARD_ULPANIR or BOARD_WEACT"
#endif

#define PIN_OUT_PP(portb, pin) do { if (portb) GPIOB_ModeCfg(pin, GPIO_ModeOut_PP_5mA); else GPIOA_ModeCfg(pin, GPIO_ModeOut_PP_5mA); } while (0)
#define PIN_IN_PU(portb, pin)  do { if (portb) GPIOB_ModeCfg(pin, GPIO_ModeIN_PU); else GPIOA_ModeCfg(pin, GPIO_ModeIN_PU); } while (0)
#define PIN_SET(portb, pin)    do { if (portb) GPIOB_SetBits(pin); else GPIOA_SetBits(pin); } while (0)
#define PIN_CLR(portb, pin)    do { if (portb) GPIOB_ResetBits(pin); else GPIOA_ResetBits(pin); } while (0)
#define PIN_READ(portb, pin)   ((portb) ? (GPIOB_ReadPortPin(pin) != 0) : (GPIOA_ReadPortPin(pin) != 0))

static inline void led_set(int on) {
    if ((on != 0) == (LED_ACTIVE_HIGH != 0)) PIN_SET(LED_PORT_B, LED_PIN); else PIN_CLR(LED_PORT_B, LED_PIN);
}
static inline int btn_a_down(void) { return !PIN_READ(BTN_A_PORT_B, BTN_A_PIN); }
/* Power latch: 1 keeps the board powered, 0 releases it (the board goes dark unless a button is down).
 * No-op on the dev board. */
static inline void power_hold(int on) {
#if HAS_HOLD
    if (on) GPIOA_SetBits(HOLD_PIN); else GPIOA_ResetBits(HOLD_PIN);
#else
    (void)on;
#endif
}
static inline int btn_b_down(void) { return !PIN_READ(BTN_B_PORT_B, BTN_B_PIN); }

static inline void board_init(void) {
#if HAS_HOLD
    GPIOA_SetBits(HOLD_PIN); /* keep the latch closed as early as possible */
    GPIOA_ModeCfg(HOLD_PIN, GPIO_ModeOut_PP_5mA);
#endif
    led_set(0);
    PIN_OUT_PP(LED_PORT_B, LED_PIN);
    PIN_IN_PU(BTN_A_PORT_B, BTN_A_PIN);
    PIN_IN_PU(BTN_B_PORT_B, BTN_B_PIN);
}
#endif

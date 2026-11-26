/* IR output: 38 kHz carrier (TMR1 PWM, duty 1/3) gated by the NEC envelope (TMR0 interrupts).
 * The indicator LED is on exactly while a frame is being sent. */
#ifndef IR_TX_H
#define IR_TX_H
#include <stdbool.h>
#include "nec.h"
void ir_tx_init(void);
/* Starts sending; returns false if a frame is still in progress. *f must stay valid until done. */
bool ir_tx_start(const nec_frame_t *f);
bool ir_tx_busy(void);
#endif

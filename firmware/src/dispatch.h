/* What to output for a button action given the stored settings (add-remote-config).
 * Pure logic: the caller sends the frame, the repeat code or, for a button without a code, the same
 * indicator flash with no IR (one flash per would-be frame). */
#ifndef DISPATCH_H
#define DISPATCH_H
#include "remote.h"
#include "settings.h"

typedef enum { OUT_NONE = 0, OUT_FRAME, OUT_REPEAT, OUT_UNASSIGNED } out_kind_t;
typedef struct {
    out_kind_t kind;
    button_code_t code; /* valid for OUT_FRAME */
} out_t;

/* Repeat period for repeat_hz frames per second, rounded to the nearest ms. */
uint16_t dispatch_period_ms(uint8_t repeat_hz);
out_t dispatch_action(const settings_t *s, action_t a);
#endif

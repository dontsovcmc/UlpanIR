/* Host tests: button action + settings -> what the firmware outputs (add-remote-config 2.2). */
#include "dispatch.h"
#include <stdio.h>

static int fails;
#define CHECK(c)                                                    \
    do {                                                            \
        if (!(c)) {                                                 \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);    \
            fails++;                                                \
        }                                                           \
    } while (0)

static settings_t configured(void) {
    settings_t s;
    settings_defaults(&s);
    s.a = (button_code_t){true, false, 0x32, 0x02};
    s.b = (button_code_t){true, true, 0x1234, 0x03};
    return s;
}

static void test_period(void) {
    CHECK(dispatch_period_ms(3) == 333);
    CHECK(dispatch_period_ms(1) == 1000);
    CHECK(dispatch_period_ms(10) == 100);
    CHECK(dispatch_period_ms(7) == 143);
}

static void test_assigned(void) {
    settings_t s = configured();
    out_t o = dispatch_action(&s, (action_t){ACT_SEND_FRAME, BTN_A});
    CHECK(o.kind == OUT_FRAME && o.code.address == 0x32 && o.code.command == 0x02 && !o.code.extended);
    o = dispatch_action(&s, (action_t){ACT_SEND_FRAME, BTN_B});
    CHECK(o.kind == OUT_FRAME && o.code.address == 0x1234 && o.code.extended);
    o = dispatch_action(&s, (action_t){ACT_SEND_REPEAT, BTN_B});
    CHECK(o.kind == OUT_REPEAT);
    o = dispatch_action(&s, (action_t){ACT_NONE, BTN_NONE});
    CHECK(o.kind == OUT_NONE);
}

static void test_unassigned_blinks_per_frame(void) {
    settings_t s;
    settings_defaults(&s); /* clean board: nothing assigned */
    /* every would-be frame gives one blink and no IR: a 100 ms press blinks once, holding blinks at the repeat rate */
    CHECK(dispatch_action(&s, (action_t){ACT_SEND_FRAME, BTN_A}).kind == OUT_UNASSIGNED);
    CHECK(dispatch_action(&s, (action_t){ACT_SEND_FRAME, BTN_A}).kind == OUT_UNASSIGNED);
    CHECK(dispatch_action(&s, (action_t){ACT_SEND_REPEAT, BTN_A}).kind == OUT_UNASSIGNED);
    CHECK(dispatch_action(&s, (action_t){ACT_NONE, BTN_NONE}).kind == OUT_NONE);
}

static void test_only_one_assigned(void) {
    settings_t s;
    settings_defaults(&s);
    s.a = (button_code_t){true, false, 0x10, 0x20};
    CHECK(dispatch_action(&s, (action_t){ACT_SEND_FRAME, BTN_B}).kind == OUT_UNASSIGNED);
    CHECK(dispatch_action(&s, (action_t){ACT_SEND_FRAME, BTN_A}).kind == OUT_FRAME);
}

int main(void) {
    test_period();
    test_assigned();
    test_unassigned_blinks_per_frame();
    test_only_one_assigned();
    if (fails) {
        printf("test_dispatch: %d failure(s)\n", fails);
        return 1;
    }
    printf("test_dispatch: ok\n");
    return 0;
}

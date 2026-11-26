/* Button -> frame/repeat scheduling (spec: ir-transmit, "Повтор при удержании"). */
#include <stdio.h>
#include "remote.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static const remote_cfg_t CFG3 = {.period_ms = 333, .nec_repeat = false, .debounce_ms = 20};

typedef struct { int frames, repeats, a, b; uint32_t last_start; } tally_t;

/* run ms ticks with given button levels, collect actions */
static void run(remote_t *r, uint32_t *now, uint32_t ms, bool a, bool b, tally_t *t) {
    for (uint32_t i = 0; i < ms; i++, (*now)++) {
        action_t act = remote_tick(r, *now, a, b);
        if (act.kind == ACT_NONE) continue;
        if (act.kind == ACT_SEND_FRAME) t->frames++;
        if (act.kind == ACT_SEND_REPEAT) t->repeats++;
        if (act.button == BTN_A) t->a++;
        if (act.button == BTN_B) t->b++;
        t->last_start = *now;
    }
}

static void test_short_press_sends_one_frame(void) {
    remote_t r; uint32_t now = 0; tally_t t = {0};
    remote_init(&r, &CFG3);
    run(&r, &now, 100, true, false, &t);
    run(&r, &now, 1000, false, false, &t);
    CHECK(t.frames == 1 && t.repeats == 0 && t.a == 1 && t.b == 0);
}

static void test_hold_2s_at_3_per_s_gives_6_to_7_frames_then_stops(void) {
    remote_t r; uint32_t now = 0; tally_t t = {0};
    remote_init(&r, &CFG3);
    run(&r, &now, 2000, true, false, &t);
    uint32_t release = now;
    tally_t after = {0};
    run(&r, &now, 2000, false, false, &after);
    /* spec: 6-7 frames; after release sending stops within one repeat period */
    CHECK(t.frames + after.frames >= 6 && t.frames + after.frames <= 7);
    CHECK(after.frames == 0 || after.last_start < release + CFG3.period_ms);
}

static void test_nec_repeat_mode_sends_frame_then_repeats(void) {
    remote_cfg_t cfg = CFG3; cfg.nec_repeat = true;
    remote_t r; uint32_t now = 0; tally_t t = {0};
    remote_init(&r, &cfg);
    run(&r, &now, 2000, true, false, &t);
    CHECK(t.frames == 1);
    CHECK(t.repeats >= 5 && t.repeats <= 6);
}

static void test_button_b_sends_only_b(void) {
    remote_t r; uint32_t now = 0; tally_t t = {0};
    remote_init(&r, &CFG3);
    run(&r, &now, 100, false, true, &t);
    CHECK(t.b == 1 && t.a == 0);
}

static void test_bounce_shorter_than_debounce_is_ignored(void) {
    remote_t r; uint32_t now = 0; tally_t t = {0};
    remote_init(&r, &CFG3);
    for (int k = 0; k < 10; k++) { run(&r, &now, 3, true, false, &t); run(&r, &now, 3, false, false, &t); }
    run(&r, &now, 500, false, false, &t);
    CHECK(t.frames == 0);
}

static void test_second_button_while_first_held_is_ignored(void) {
    remote_t r; uint32_t now = 0; tally_t t = {0};
    remote_init(&r, &CFG3);
    run(&r, &now, 100, true, false, &t);   /* A pressed: 1 frame */
    run(&r, &now, 200, true, true, &t);    /* both held */
    CHECK(t.b == 0);
}

static void test_repress_while_other_held_waits_for_full_release(void) {
    remote_t r; uint32_t now = 0; tally_t t = {0};
    remote_init(&r, &CFG3);
    run(&r, &now, 100, true, false, &t);  /* A: 1 frame */
    run(&r, &now, 100, true, true, &t);   /* B joins */
    run(&r, &now, 100, false, true, &t);  /* A released, B held */
    tally_t t2 = {0};
    run(&r, &now, 1000, true, true, &t2); /* A pressed again with B held */
    CHECK(t2.frames == 0 && t2.repeats == 0);
}

static void test_both_pressed_together_sends_nothing(void) {
    remote_t r; uint32_t now = 0; tally_t t = {0};
    remote_init(&r, &CFG3);
    run(&r, &now, 1000, true, true, &t);
    CHECK(t.frames == 0 && t.repeats == 0);
}

int main(void) {
    test_short_press_sends_one_frame();
    test_hold_2s_at_3_per_s_gives_6_to_7_frames_then_stops();
    test_nec_repeat_mode_sends_frame_then_repeats();
    test_button_b_sends_only_b();
    test_bounce_shorter_than_debounce_is_ignored();
    test_second_button_while_first_held_is_ignored();
    test_repress_while_other_held_waits_for_full_release();
    test_both_pressed_together_sends_nothing();
    printf(fails ? "%d FAILED\n" : "all remote tests passed\n", fails);
    return fails != 0;
}

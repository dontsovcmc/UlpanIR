/* Host tests for src/power.c: when the remote releases the power latch. */
#include <assert.h>
#include <stdio.h>
#include "power.h"

/* Runs ticks from..to with fixed inputs, returns the first non-NONE event and its time. */
static power_event_t run(power_t *p, uint32_t from, uint32_t to, bool down, bool busy, uint32_t *at) {
    for (uint32_t t = from; t <= to; t++) {
        power_event_t e = power_tick(p, t, down, busy);
        if (e != PWR_EV_NONE) {
            if (at) *at = t;
            return e;
        }
    }
    return PWR_EV_NONE;
}

static void short_press_sends_full_frame_then_off(void) {
    power_t p;
    power_init(&p, 20, false);
    uint32_t at = 0;
    /* pressed 0..99 ms, frame 20..88 ms in progress */
    assert(run(&p, 0, 19, true, false, NULL) == PWR_EV_NONE);
    assert(run(&p, 20, 88, true, true, NULL) == PWR_EV_NONE);
    assert(run(&p, 89, 99, true, false, NULL) == PWR_EV_NONE);
    assert(run(&p, 100, 200, false, false, &at) == PWR_EV_OFF);
    assert(at == 119); /* released for debounce_ms */
    assert(run(&p, 201, 400, false, false, NULL) == PWR_EV_NONE); /* OFF only once */
}

static void release_during_frame_waits_for_frame_end(void) {
    power_t p;
    power_init(&p, 20, false);
    uint32_t at = 0;
    assert(run(&p, 0, 29, true, true, NULL) == PWR_EV_NONE);
    assert(run(&p, 30, 87, false, true, NULL) == PWR_EV_NONE); /* released, frame still going */
    assert(run(&p, 88, 200, false, false, &at) == PWR_EV_OFF);
    assert(at == 88);
}

static void bounce_does_not_switch_off(void) {
    power_t p;
    power_init(&p, 20, false);
    assert(run(&p, 0, 50, true, false, NULL) == PWR_EV_NONE);
    assert(run(&p, 51, 60, false, false, NULL) == PWR_EV_NONE); /* up 10 ms < debounce */
    assert(run(&p, 61, 300, true, false, NULL) == PWR_EV_NONE);
}

static void no_press_at_boot_switches_off(void) {
    power_t p;
    power_init(&p, 20, false);
    uint32_t at = 0;
    assert(run(&p, 0, 100, false, false, &at) == PWR_EV_OFF);
    assert(at == 20);
}

static void press_again_after_off_holds_power(void) {
    power_t p;
    power_init(&p, 20, false);
    assert(run(&p, 0, 100, false, false, NULL) == PWR_EV_OFF);
    uint32_t at = 0;
    assert(run(&p, 101, 120, true, false, &at) == PWR_EV_ON);
    assert(at == 101); /* at once: the user presses again, sending resumes */
    assert(run(&p, 121, 200, true, false, NULL) == PWR_EV_NONE);
    assert(run(&p, 201, 300, false, false, &at) == PWR_EV_OFF);
}

static void low_battery_fades_then_off(void) {
    power_t p;
    power_init(&p, 20, true);
    uint32_t at = 0;
    assert(run(&p, 0, 99, true, false, NULL) == PWR_EV_NONE);
    assert(run(&p, 100, 200, false, false, &at) == PWR_EV_FADE);
    assert(at == 119);
    assert(power_fade_done(&p, true) == PWR_EV_OFF);
    assert(run(&p, 201, 400, false, false, NULL) == PWR_EV_NONE);
}

static void press_during_fade_resumes_and_fades_again(void) {
    power_t p;
    power_init(&p, 20, true);
    assert(run(&p, 0, 99, true, false, NULL) == PWR_EV_NONE);
    assert(run(&p, 100, 200, false, false, NULL) == PWR_EV_FADE);
    assert(power_fade_done(&p, false) == PWR_EV_NONE); /* cut short by a press: power stays on */
    assert(run(&p, 201, 300, true, false, NULL) == PWR_EV_NONE);
    assert(run(&p, 301, 400, false, false, NULL) == PWR_EV_FADE);
}

static void fade_level_curve(void) {
    assert(power_fade_level(0, 2000) == 1000);
    assert(power_fade_level(2000, 2000) == 0);
    assert(power_fade_level(5000, 2000) == 0);
    assert(power_fade_level(1000, 2000) == 250); /* quadratic: half time, quarter brightness */
    uint16_t prev = 1000;
    for (uint32_t t = 0; t <= 2000; t += 10) {
        uint16_t l = power_fade_level(t, 2000);
        assert(l <= prev);
        prev = l;
    }
}

int main(void) {
    short_press_sends_full_frame_then_off();
    release_during_frame_waits_for_frame_end();
    bounce_does_not_switch_off();
    no_press_at_boot_switches_off();
    press_again_after_off_holds_power();
    low_battery_fades_then_off();
    press_during_fade_resumes_and_fades_again();
    fade_level_curve();
    printf("test_power: ok\n");
    return 0;
}

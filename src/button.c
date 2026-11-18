// button.c — GPIO driver for the mode button (onboard GP21)
//
// GP21 already has hardware debounce on the Proton. The software debounce
// below is a second layer so one press never counts twice.
//
// Day one check: run with BUTTON_ACTIVE_LOW 1 and watch the serial output.
// If "pressed" prints when you RELEASE the button, change it to 0.

#include "pico/stdlib.h"
#include "pins.h"
#include "button.h"

#define BUTTON_ACTIVE_LOW   1
#define DEBOUNCE_MS         20

static bool     stable_down = false;
static bool     last_raw    = false;
static uint32_t last_change_ms = 0;

static bool read_raw_down(void) {
    bool level = gpio_get(MODE_BTN_PIN);
    return BUTTON_ACTIVE_LOW ? !level : level;
}

void button_init(void) {
    gpio_init(MODE_BTN_PIN);
    gpio_set_dir(MODE_BTN_PIN, GPIO_IN);
#if BUTTON_ACTIVE_LOW
    gpio_pull_up(MODE_BTN_PIN);
#else
    gpio_pull_down(MODE_BTN_PIN);
#endif
    last_raw = stable_down = read_raw_down();
    last_change_ms = to_ms_since_boot(get_absolute_time());
}

static void button_poll(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool raw = read_raw_down();

    if (raw != last_raw) {          // input moved, restart the timer
        last_raw = raw;
        last_change_ms = now;
    }
    if ((now - last_change_ms) >= DEBOUNCE_MS) {
        stable_down = raw;          // held steady long enough, accept it
    }
}

bool button_is_down(void) {
    button_poll();
    return stable_down;
}

bool button_pressed(void) {
    static bool was_down = false;
    button_poll();
    bool fired = stable_down && !was_down;   // rising edge only
    was_down = stable_down;
    return fired;
}

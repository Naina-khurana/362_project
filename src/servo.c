// servo.c — PWM driver for the MG90S servo
//
// Servo expects a 50 Hz signal (20 ms period). Pulse width sets the angle.
// PWM counter runs at 1 MHz so 1 count = 1 us, which makes the math easy.
//
// Power the servo from a SEPARATE 5V supply and tie its GND to Proton GND.

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pins.h"
#include "servo.h"

// Tune these to the printed finger's real end stops so the servo never
// strains the linkage. MG90S full range is roughly 500-2400 us.
#define SERVO_MIN_US    1000
#define SERVO_MAX_US    2000
#define SERVO_MAX_DEG   180.0f

#define SERVO_PERIOD_US 20000   // 50 Hz

static uint slice;
static uint channel;

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice   = pwm_gpio_to_slice_num(SERVO_PIN);
    channel = pwm_gpio_to_channel(SERVO_PIN);

    // sys clock (150 MHz) / divider -> 1 MHz tick
    float div = (float)clock_get_hz(clk_sys) / 1000000.0f;
    pwm_set_clkdiv(slice, div);
    pwm_set_wrap(slice, SERVO_PERIOD_US - 1);

    // start centered
    pwm_set_chan_level(slice, channel, (SERVO_MIN_US + SERVO_MAX_US) / 2);
    pwm_set_enabled(slice, true);
}

void servo_set_pulse_us(uint16_t us) {
    if (us < SERVO_MIN_US) us = SERVO_MIN_US;
    if (us > SERVO_MAX_US) us = SERVO_MAX_US;
    pwm_set_chan_level(slice, channel, us);
}

void servo_set_angle(float deg) {
    if (deg < 0.0f) deg = 0.0f;
    if (deg > SERVO_MAX_DEG) deg = SERVO_MAX_DEG;
    uint16_t us = SERVO_MIN_US +
        (uint16_t)((deg / SERVO_MAX_DEG) * (SERVO_MAX_US - SERVO_MIN_US));
    servo_set_pulse_us(us);
}

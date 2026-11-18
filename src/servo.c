#include "servo.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

static float current_norm = 0.0f;

void servo_init(void) {
    // 1. Route SERVO_PIN to the PWM peripheral
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);

    // 2. Automatically resolve the PWM slice for SERVO_PIN (GPIO 15 -> Slice 7)
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);

    // 3. Set clock divider so 1 PWM tick = 1 microsecond (1 MHz)
    float clkdiv = (float)clock_get_hz(clk_sys) / 1000000.0f;

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clkdiv);
    pwm_config_set_wrap(&config, 19999); // 20,000 ticks = 20 ms = 50 Hz

    // 4. Load configuration and enable PWM slice
    pwm_init(slice_num, &config, true);

    // 5. Start at a safe center position
    servo_set_pulse_us(SERVO_CENTER_US);
}

void servo_set_pulse_us(uint16_t pulse_us) {
    if (pulse_us < SERVO_MIN_US) pulse_us = SERVO_MIN_US;
    if (pulse_us > SERVO_MAX_US) pulse_us = SERVO_MAX_US;

    // Since 1 tick = 1 us, channel level maps 1:1 to microseconds
    pwm_set_gpio_level(SERVO_PIN, pulse_us);
}

void servo_set_angle(float angle_deg) {
    if (angle_deg < 0.0f)   angle_deg = 0.0f;
    if (angle_deg > 180.0f) angle_deg = 180.0f;

    uint16_t pulse_us = (uint16_t)(SERVO_MIN_US + (angle_deg / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US));
    servo_set_pulse_us(pulse_us);
}

void servo_set_normalized(float norm, float alpha) {
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    current_norm = (alpha * norm) + ((1.0f - alpha) * current_norm);

    uint16_t pulse_us = (uint16_t)(SERVO_MIN_US + current_norm * (SERVO_MAX_US - SERVO_MIN_US));
    servo_set_pulse_us(pulse_us);
}

// flex.c — ADC driver for the flex sensor
//
// Wiring: 3V3 -> flex sensor -> GP40 -> 47k resistor -> GND
// Measure the sensor flat and fully bent with a multimeter, then pick
// R_fixed ~= sqrt(R_flat * R_bent) for the biggest voltage swing.

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pins.h"
#include "flex.h"

#define FLEX_SAMPLES 16

static uint16_t raw_straight = 0;
static uint16_t raw_bent     = 4095;

void flex_init(void) {
    adc_init();
    adc_gpio_init(FLEX_PIN);        // disables digital input + pulls on the pad
    adc_select_input(FLEX_ADC_CH);
}

uint16_t flex_read_raw(void) {
    adc_select_input(FLEX_ADC_CH);  // re-select in case another channel is used later
    uint32_t sum = 0;
    for (int i = 0; i < FLEX_SAMPLES; i++) {
        sum += adc_read();
    }
    return (uint16_t)(sum / FLEX_SAMPLES);
}

float flex_raw_to_volts(uint16_t raw) {
    return raw * 3.3f / 4095.0f;
}

void flex_set_calibration(uint16_t straight, uint16_t bent) {
    raw_straight = straight;
    raw_bent     = bent;
}

void flex_calibrate(void) {
    printf("Calibrating flex sensor. Hold finger STRAIGHT...\n");
    sleep_ms(2000);
    raw_straight = flex_read_raw();
    printf("  straight = %u\n", raw_straight);

    printf("Now BEND finger fully...\n");
    sleep_ms(2000);
    raw_bent = flex_read_raw();
    printf("  bent     = %u\n", raw_bent);

    if (raw_straight == raw_bent) {
        printf("WARNING: straight and bent readings match, check wiring/resistor\n");
    }
}

// Works whether the voltage goes up or down when bending
float flex_get_bend(void) {
    int span = (int)raw_bent - (int)raw_straight;
    if (span == 0) return 0.0f;

    float b = (float)((int)flex_read_raw() - (int)raw_straight) / (float)span;
    if (b < 0.0f) b = 0.0f;
    if (b > 1.0f) b = 1.0f;
    return b;
}

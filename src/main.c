// main.c — ProtoPaw
//
// Current state: ADC -> PWM bring-up test.
// Bend the flex sensor and the servo follows.
//
// Still to add (one file each, same pattern as flex.c / servo.c):
//   imu.c    I2C MPU6050, wrist pitch/roll
//   tft.c    SPI TFT display (course lcd.c library)
//   button.c GPIO mode switch on GP21

#include <stdio.h>
#include "pico/stdlib.h"
#include "pins.h"
#include "flex.h"
#include "servo.h"

#define TEST_MODE_RAW_ADC 0   // set to 1 to only print raw ADC values (Week 1)

int main(void) {
    stdio_init_all();
    sleep_ms(2000);           // give the serial monitor time to connect
    printf("ProtoPaw starting\n");

    flex_init();
    servo_init();

#if TEST_MODE_RAW_ADC
    for (;;) {
        uint16_t raw = flex_read_raw();
        printf("raw=%4u  volts=%.3f\n", raw, flex_raw_to_volts(raw));
        sleep_ms(100);
    }
#endif

    flex_calibrate();

    float filtered = 0.0f;
    int   print_count = 0;

    for (;;) {
        float bend = flex_get_bend();                 // ADC
        filtered = 0.8f * filtered + 0.2f * bend;     // smooth out jitter
        servo_set_angle(filtered * 180.0f);           // PWM

        if (++print_count >= 10) {                    // print every ~100 ms
            print_count = 0;
            printf("bend=%.2f  angle=%.1f\n", filtered, filtered * 180.0f);
        }
        sleep_ms(10);
    }
}

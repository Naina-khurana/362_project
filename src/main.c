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
#include "imu.h"

#define TEST_MODE_RAW_ADC 0   // set to 1 to only print raw ADC values (Week 1)
#define TEST_MODE_IMU     0   // set to 1 to only test the MPU6050 over I2C

int main(void) {
    stdio_init_all();
    sleep_ms(2000);           // give the serial monitor time to connect
    printf("ProtoPaw starting\n");

#if TEST_MODE_IMU
    imu_init();
    imu_scan();                                   // expect 0x68
    printf("WHO_AM_I = 0x%02X\n", imu_whoami());  // 0x68 (or 0x70/0x72 on clones)
    imu_calibrate_gyro();
    for (int n = 0;; n++) {
        imu_update();
        if (n % 10 == 0) {                        // print every ~100 ms
            imu_raw_t r;
            if (imu_read_raw(&r)) {
                printf("acc %6d %6d %6d  gyro %6d %6d %6d  pitch %6.1f  roll %6.1f  wrist %.2f\n",
                       r.ax, r.ay, r.az, r.gx, r.gy, r.gz,
                       imu_get_pitch(), imu_get_roll(), imu_get_wrist());
            } else {
                printf("read failed, check SDA/SCL wiring\n");
            }
        }
        sleep_ms(10);
    }
#endif

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
// main.c — ProtoPaw
//
// FLEX mode  finger bend (ADC)  -> servo (PWM)
// TILT mode  wrist pitch (I2C)  -> servo (PWM)
// Onboard button GP21 (GPIO) switches modes, TFT (SPI) shows live telemetry.

#include <stdio.h>
#include "pico/stdlib.h"
#include "pins.h"
#include "flex.h"
#include "servo.h"
#include "imu.h"
#include "button.h"
#include "tft.h"

#define TEST_MODE_RAW_ADC 0   // set to 1 to only print raw ADC values (Week 1)

#define MODE_FLEX 0
#define MODE_TILT 1

#define LOOP_MS          10   // 100 Hz control loop
#define TFT_EVERY_N      10   // redraw screen every 10 loops (~10 Hz)
#define PRINT_EVERY_N    20   // serial print every 20 loops (~5 Hz)
#define SERVO_MAX_ANGLE  180.0f

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

    button_init();
    bool imu_ok = imu_init();
    if (!imu_ok) printf("Running FLEX mode only (no IMU)\n");
    tft_init();

    flex_calibrate();

    int   mode = MODE_FLEX;
    float filtered = 0.0f;
    int   loop_count = 0;

    for (;;) {
        // GPIO: mode switch
        if (button_pressed() && imu_ok) {
            mode = (mode == MODE_FLEX) ? MODE_TILT : MODE_FLEX;
            printf("Mode -> %s\n", mode == MODE_FLEX ? "FLEX" : "TILT");
        }

        // Inputs
        float bend = flex_get_bend();           // ADC
        imu_update();                           // I2C
        float tilt = imu_get_tilt();

        // Pick source, smooth, drive servo
        float target = (mode == MODE_FLEX) ? bend : tilt;
        filtered = 0.8f * filtered + 0.2f * target;
        float angle = filtered * SERVO_MAX_ANGLE;
        servo_set_angle(angle);                 // PWM

        loop_count++;

        // SPI: screen is slow, update less often than the servo
        if (loop_count % TFT_EVERY_N == 0) {
            tft_update(bend, imu_get_pitch(), imu_get_roll(), angle, mode);
        }

        if (loop_count % PRINT_EVERY_N == 0) {
            printf("%s bend=%.2f pitch=%6.1f roll=%6.1f servo=%5.1f\n",
                   mode == MODE_FLEX ? "FLEX" : "TILT",
                   bend, imu_get_pitch(), imu_get_roll(), angle);
        }

        sleep_ms(LOOP_MS);
    }
}

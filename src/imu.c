// imu.c — I2C driver for the MPU6050 (GY-521)
//
// Wiring (GY-521 already has SDA/SCL pull-ups on the board):
//   VCC -> 3V3    GND -> GND    SDA -> GP4    SCL -> GP5    AD0 -> open/GND (addr 0x68)
//
// Angles come from a complementary filter: the gyro is smooth but drifts,
// the accelerometer doesn't drift but is noisy, so we blend 98% / 2%.

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pins.h"
#include "imu.h"

// MPU6050 registers
#define REG_CONFIG        0x1A    // digital low-pass filter
#define REG_GYRO_CONFIG   0x1B
#define REG_ACCEL_CONFIG  0x1C
#define REG_ACCEL_XOUT_H  0x3B    // 14 bytes: accel XYZ, temp, gyro XYZ
#define REG_PWR_MGMT_1    0x6B
#define REG_WHO_AM_I      0x75

#define IMU_I2C_HZ        100000  // 100 kHz is safest with the long glove cable
#define IMU_TIMEOUT_US    5000

#define ACCEL_LSB_PER_G   16384.0f   // +/-2 g range
#define GYRO_LSB_PER_DPS  131.0f     // +/-250 deg/s range
#define FILTER_ALPHA      0.98f

// Wrist range mapped to 0..1 for imu_get_wrist(). Tune after mounting on the glove.
#define WRIST_MIN_DEG     -45.0f
#define WRIST_MAX_DEG      45.0f

static float gyro_off_x = 0, gyro_off_y = 0;
static float pitch = 0, roll = 0;
static uint64_t last_us = 0;

static bool write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return i2c_write_timeout_us(IMU_I2C, IMU_ADDR, buf, 2, false, IMU_TIMEOUT_US) == 2;
}

static bool read_regs(uint8_t reg, uint8_t *dst, size_t len) {
    if (i2c_write_timeout_us(IMU_I2C, IMU_ADDR, &reg, 1, true, IMU_TIMEOUT_US) != 1) return false;
    return i2c_read_timeout_us(IMU_I2C, IMU_ADDR, dst, len, false, IMU_TIMEOUT_US) == (int)len;
}

bool imu_init(void) {
    i2c_init(IMU_I2C, IMU_I2C_HZ);
    gpio_set_function(IMU_SDA, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA);      // backup pull-ups, harmless with the GY-521's own
    gpio_pull_up(IMU_SCL);
    sleep_ms(100);              // sensor power-up time

    bool ok = write_reg(REG_PWR_MGMT_1, 0x00)     // wake up (it boots asleep)
           && write_reg(REG_CONFIG, 0x03)         // ~44 Hz low-pass, cuts jitter
           && write_reg(REG_GYRO_CONFIG, 0x00)    // +/-250 deg/s
           && write_reg(REG_ACCEL_CONFIG, 0x00);  // +/-2 g

    if (!ok) printf("IMU: no response at 0x%02X, check wiring\n", IMU_ADDR);
    last_us = time_us_64();
    return ok;
}

void imu_scan(void) {
    printf("I2C scan:");
    int found = 0;
    for (int addr = 0x08; addr < 0x78; addr++) {
        uint8_t dummy;
        if (i2c_read_timeout_us(IMU_I2C, addr, &dummy, 1, false, IMU_TIMEOUT_US) >= 0) {
            printf(" 0x%02X", addr);
            found++;
        }
    }
    printf(found ? "\n" : " nothing found\n");
}

uint8_t imu_whoami(void) {
    uint8_t id = 0;
    if (!read_regs(REG_WHO_AM_I, &id, 1)) return 0;
    return id;
}

bool imu_read_raw(imu_raw_t *out) {
    uint8_t b[14];
    if (!read_regs(REG_ACCEL_XOUT_H, b, sizeof b)) return false;
    // each value is big-endian: high byte first
    out->ax = (int16_t)((b[0]  << 8) | b[1]);
    out->ay = (int16_t)((b[2]  << 8) | b[3]);
    out->az = (int16_t)((b[4]  << 8) | b[5]);
    // b[6], b[7] are temperature, skipped
    out->gx = (int16_t)((b[8]  << 8) | b[9]);
    out->gy = (int16_t)((b[10] << 8) | b[11]);
    out->gz = (int16_t)((b[12] << 8) | b[13]);
    return true;
}

void imu_calibrate_gyro(void) {
    printf("Calibrating IMU gyro. Keep the glove STILL...\n");
    const int n = 200;
    float sx = 0, sy = 0;
    int good = 0;
    for (int i = 0; i < n; i++) {
        imu_raw_t r;
        if (imu_read_raw(&r)) { sx += r.gx; sy += r.gy; good++; }
        sleep_ms(5);
    }
    if (good > 0) { gyro_off_x = sx / good; gyro_off_y = sy / good; }
    printf("  gyro offset x=%.1f y=%.1f\n", gyro_off_x, gyro_off_y);

    // start the filter from the accelerometer so it doesn't slowly swing in
    imu_raw_t r;
    if (imu_read_raw(&r)) {
        roll  = atan2f(r.ay, r.az) * 57.2958f;
        pitch = atan2f(-r.ax, sqrtf((float)r.ay * r.ay + (float)r.az * r.az)) * 57.2958f;
    }
    last_us = time_us_64();
}

void imu_update(void) {
    imu_raw_t r;
    if (!imu_read_raw(&r)) return;           // keep the last good angle on a bad read

    uint64_t now = time_us_64();
    float dt = (now - last_us) / 1e6f;
    last_us = now;
    if (dt > 0.1f) dt = 0.1f;                // don't jump after a long pause

    float ax = r.ax / ACCEL_LSB_PER_G;
    float ay = r.ay / ACCEL_LSB_PER_G;
    float az = r.az / ACCEL_LSB_PER_G;
    float acc_roll  = atan2f(ay, az) * 57.2958f;
    float acc_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2958f;

    float gx = (r.gx - gyro_off_x) / GYRO_LSB_PER_DPS;   // deg/s
    float gy = (r.gy - gyro_off_y) / GYRO_LSB_PER_DPS;

    roll  = FILTER_ALPHA * (roll  + gx * dt) + (1.0f - FILTER_ALPHA) * acc_roll;
    pitch = FILTER_ALPHA * (pitch + gy * dt) + (1.0f - FILTER_ALPHA) * acc_pitch;
}

float imu_get_pitch(void) { return pitch; }
float imu_get_roll(void)  { return roll; }

// Wrist up/down mapped to 0..1 so main.c can treat it like flex_get_bend().
// If tilting your wrist moves roll instead of pitch, swap pitch for roll here.
float imu_get_wrist(void) {
    float w = (pitch - WRIST_MIN_DEG) / (WRIST_MAX_DEG - WRIST_MIN_DEG);
    if (w < 0.0f) w = 0.0f;
    if (w > 1.0f) w = 1.0f;
    return w;
}

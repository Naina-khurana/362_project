// imu.c — I2C driver for the MPU6050 on the wrist
//
// Wiring: VIN -> 3V3, GND -> GND, SDA -> GP4, SCL -> GP5
// The Adafruit MPU6050 board already has pull-up resistors on SDA/SCL.
//
// Week 1 check: imu_init() reads WHO_AM_I. If it prints 0x68, I2C works.

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pins.h"
#include "imu.h"

// MPU6050 registers
#define REG_SMPLRT_DIV    0x19
#define REG_CONFIG        0x1A
#define REG_GYRO_CONFIG   0x1B
#define REG_ACCEL_CONFIG  0x1C
#define REG_ACCEL_XOUT_H  0x3B   // 14 bytes: accel X/Y/Z, temp, gyro X/Y/Z
#define REG_PWR_MGMT_1    0x6B
#define REG_WHO_AM_I      0x75

#define ACCEL_LSB_PER_G   16384.0f
#define GYRO_LSB_PER_DPS  131.0f

// Complementary filter weight. Higher = trust gyro more (smoother, more drift).
#define ALPHA             0.98f

// Wrist pitch range mapped onto the servo's full range.
// -TILT_RANGE_DEG -> 0.0, +TILT_RANGE_DEG -> 1.0
#define TILT_RANGE_DEG    60.0f

#define RAD_TO_DEG        57.2957795f

static float    pitch = 0.0f;
static float    roll  = 0.0f;
static uint64_t last_us = 0;
static bool     ok = false;

static bool write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return i2c_write_blocking(IMU_I2C, IMU_ADDR, buf, 2, false) == 2;
}

static bool read_regs(uint8_t reg, uint8_t *dst, size_t len) {
    if (i2c_write_blocking(IMU_I2C, IMU_ADDR, &reg, 1, true) != 1) return false;
    return i2c_read_blocking(IMU_I2C, IMU_ADDR, dst, len, false) == (int)len;
}

bool imu_init(void) {
    i2c_init(IMU_I2C, 400 * 1000);              // 400 kHz fast mode
    gpio_set_function(IMU_SDA, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA);                      // backup, board has its own
    gpio_pull_up(IMU_SCL);

    uint8_t id = 0;
    if (!read_regs(REG_WHO_AM_I, &id, 1)) {
        printf("IMU: no response on I2C, check SDA/SCL/power\n");
        ok = false;
        return false;
    }
    printf("IMU: WHO_AM_I = 0x%02X (expected 0x68)\n", id);

    write_reg(REG_PWR_MGMT_1, 0x00);   // wake up (chip boots asleep)
    sleep_ms(100);
    write_reg(REG_SMPLRT_DIV, 0x07);   // 1 kHz / (1 + 7) = 125 Hz
    write_reg(REG_CONFIG, 0x03);       // ~44 Hz low-pass filter
    write_reg(REG_GYRO_CONFIG, 0x00);  // +-250 deg/s
    write_reg(REG_ACCEL_CONFIG, 0x00); // +-2 g

    // Start the filter from the accelerometer angle so it doesn't swing in
    imu_raw_t r;
    if (imu_read_raw(&r)) {
        float ax = r.ax / ACCEL_LSB_PER_G;
        float ay = r.ay / ACCEL_LSB_PER_G;
        float az = r.az / ACCEL_LSB_PER_G;
        pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
        roll  = atan2f(ay, az) * RAD_TO_DEG;
    }
    last_us = time_us_64();
    ok = true;
    return true;
}

bool imu_read_raw(imu_raw_t *out) {
    uint8_t b[14];
    if (!read_regs(REG_ACCEL_XOUT_H, b, 14)) return false;
    out->ax = (int16_t)((b[0]  << 8) | b[1]);
    out->ay = (int16_t)((b[2]  << 8) | b[3]);
    out->az = (int16_t)((b[4]  << 8) | b[5]);
    // b[6], b[7] are temperature, unused
    out->gx = (int16_t)((b[8]  << 8) | b[9]);
    out->gy = (int16_t)((b[10] << 8) | b[11]);
    out->gz = (int16_t)((b[12] << 8) | b[13]);
    return true;
}

void imu_update(void) {
    if (!ok) return;

    imu_raw_t r;
    if (!imu_read_raw(&r)) return;

    uint64_t now = time_us_64();
    float dt = (now - last_us) / 1000000.0f;
    last_us = now;

    float ax = r.ax / ACCEL_LSB_PER_G;
    float ay = r.ay / ACCEL_LSB_PER_G;
    float az = r.az / ACCEL_LSB_PER_G;
    float gx = r.gx / GYRO_LSB_PER_DPS;   // roll rate
    float gy = r.gy / GYRO_LSB_PER_DPS;   // pitch rate

    // Angle from gravity (noisy but doesn't drift)
    float acc_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
    float acc_roll  = atan2f(ay, az) * RAD_TO_DEG;

    // Complementary filter: integrate gyro, pull slowly toward accel
    pitch = ALPHA * (pitch + gy * dt) + (1.0f - ALPHA) * acc_pitch;
    roll  = ALPHA * (roll  + gx * dt) + (1.0f - ALPHA) * acc_roll;
}

float imu_get_pitch(void) { return pitch; }
float imu_get_roll(void)  { return roll; }

float imu_get_tilt(void) {
    float t = (pitch + TILT_RANGE_DEG) / (2.0f * TILT_RANGE_DEG);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t;
}

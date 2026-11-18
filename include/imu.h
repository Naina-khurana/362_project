// imu.h — I2C driver for the MPU6050 (GY-521) on the back of the glove
#ifndef IMU_H
#define IMU_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t ax, ay, az;     // raw accelerometer counts (16384 = 1 g at +/-2 g)
    int16_t gx, gy, gz;     // raw gyro counts (131 = 1 deg/s at +/-250 deg/s)
} imu_raw_t;

bool    imu_init(void);                  // wakes the sensor, returns false if it doesn't answer
void    imu_scan(void);                  // prints every I2C address that ACKs (debug)
uint8_t imu_whoami(void);                // 0x68 on a real MPU6050, 0x70/0x72 on some clones
bool    imu_read_raw(imu_raw_t *out);    // one burst read of accel + gyro
void    imu_calibrate_gyro(void);        // hold still ~1 s, removes gyro drift offset
void    imu_update(void);                // call every loop iteration
float   imu_get_pitch(void);             // degrees, filtered
float   imu_get_roll(void);              // degrees, filtered
float   imu_get_wrist(void);             // 0.0 .. 1.0, same scale as flex_get_bend()

#endif

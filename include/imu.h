// imu.h — I2C driver for the MPU6050 on the wrist
#ifndef IMU_H
#define IMU_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t ax, ay, az;     // raw accel, +-2 g range (16384 counts per g)
    int16_t gx, gy, gz;     // raw gyro, +-250 deg/s range (131 counts per deg/s)
} imu_raw_t;

bool  imu_init(void);               // returns true if the MPU6050 answered
bool  imu_read_raw(imu_raw_t *out); // returns false on an I2C error
void  imu_update(void);             // call every loop, updates pitch/roll
float imu_get_pitch(void);          // degrees, filtered
float imu_get_roll(void);           // degrees, filtered
float imu_get_tilt(void);           // 0.0..1.0 for the servo (from pitch)

#endif

// servo.h — PWM driver for the MG90S servo
#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

void servo_init(void);
void servo_set_pulse_us(uint16_t us);   // raw pulse width in microseconds
void servo_set_angle(float deg);        // 0..SERVO_MAX_DEG, clamped

#endif

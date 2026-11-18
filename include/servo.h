#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>
#include "pins.h"

// MG90S pulse width limits in microseconds.
// Tune these after attaching the 3D-printed tendon to prevent mechanical stalling.
#define SERVO_MIN_US    500
#define SERVO_MAX_US    2500
#define SERVO_CENTER_US 1500

// Initializes SERVO_PIN (from pins.h) for 50 Hz (20 ms) servo PWM
void servo_init(void);

// Sets raw pulse width in microseconds, clamped to [SERVO_MIN_US, SERVO_MAX_US]
void servo_set_pulse_us(uint16_t pulse_us);

// Sets servo angle in degrees (0.0 to 180.0)
void servo_set_angle(float angle_deg);

// Sets servo position from a normalized input (0.0 = straight, 1.0 = fully bent)
// alpha: EMA smoothing factor (1.0 = no filter, 0.15-0.3 = smooth & responsive)
void servo_set_normalized(float norm, float alpha);

#endif // SERVO_H

// flex.h — ADC driver for the flex sensor
#ifndef FLEX_H
#define FLEX_H

#include <stdint.h>

void     flex_init(void);
uint16_t flex_read_raw(void);          // averaged 12-bit reading, 0..4095
float    flex_raw_to_volts(uint16_t raw);
void     flex_calibrate(void);         // records straight + fully bent readings
void     flex_set_calibration(uint16_t straight, uint16_t bent);
float    flex_get_bend(void);          // 0.0 = straight, 1.0 = fully bent

#endif

// tft.h — SPI TFT display (wraps the course lcd.c library)
#ifndef TFT_H
#define TFT_H

void tft_init(void);
void tft_update(float bend, float pitch_deg, float roll_deg,
                float servo_deg, int mode);

#endif

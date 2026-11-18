// pins.h — ProtoPaw shared pin map (Proton / RP2350B, fresh board)
//
// Reserved on the Proton itself, do not reuse:
//   GP0/GP1   debug probe UART (printf over USB)
//   GP21/GP26 onboard pushbuttons (hardware debounced)
//   GP22-25   onboard LEDs
// ADC only exists on GP40-47 (ADC0-7) on the RP2350B.

#ifndef PINS_H
#define PINS_H

// ---------- ADC: flex sensor ----------
#define FLEX_PIN        26      // GP40
#define FLEX_ADC_CH     0       // GP40 = ADC0 (channel = pin - 40)

// ---------- I2C: MPU6050 IMU ----------
#define IMU_I2C         i2c0
#define IMU_SDA         4       // I2C0 SDA
#define IMU_SCL         5       // I2C0 SCL
#define IMU_ADDR        0x68

// ---------- SPI: TFT LCD (course lib uses spi1) ----------
#define LCD_SPI         spi1
#define PIN_SCK         10      // SPI1 SCK
#define PIN_SDI         11      // SPI1 TX (MOSI)
#define PIN_CS          12
#define PIN_DC          13
#define PIN_nRESET      14

// ---------- PWM: MG90S servo ----------
#define SERVO_PIN       16      // PWM slice 0, channel A

// ---------- GPIO: mode button + status LEDs ----------
#define MODE_BTN_PIN    21      // onboard button, check polarity on day one
#define LED_RED         22
#define LED_YELLOW      23
#define LED_GREEN       24
#define LED_BLUE        25

#endif

# ProtoPaw (ECE 362, Team 72)

Wearable glove (flex sensor + MPU6050) that drives a 3D-printed robotic finger with an MG90S servo, with live telemetry on the TFT LCD. Runs on the Purdue Proton (RP2350B), PlatformIO + Pico SDK.

## Layout

| File | What it does |
|---|---|
| `include/pins.h` | Shared pin map. Change pins here only |
| `src/flex.c` | ADC driver for the flex sensor |
| `src/servo.c` | PWM driver for the servo |
| `src/imu.c` | I2C driver for the MPU6050, pitch/roll with a complementary filter |
| `src/tft.c` | SPI display, wraps the course `lcd.c` library |
| `src/button.c` | GPIO mode button with debounce |
| `src/main.c` | Main loop, FLEX and TILT modes |

## TFT library setup

Copy `lcd.c` into `src/` and `lcd.h` into `include/` from the course project-lcd zip. In `lcd.c`, replace the three `x` defines with

```c
#include "pins.h"
#define CS_NUM    PIN_CS
#define DC_NUM    PIN_DC
#define RESET_NUM PIN_nRESET
```

Until those files are added, `tft.c` builds as empty stubs so everything else still runs.

## Pin map

| Device | Signal | Pin | Peripheral |
|---|---|---|---|
| Flex sensor | Divider midpoint | GP40 | ADC0 |
| MPU6050 | SDA / SCL | GP4 / GP5 | I2C0 |
| TFT LCD | SCK / SDI | GP10 / GP11 | SPI1 |
| TFT LCD | CS / DC / RESET | GP12 / GP13 / GP14 | GPIO |
| MG90S servo | Signal | GP16 | PWM slice 0A |
| Mode button | Onboard | GP21 | GPIO |

Do not use GP0/GP1 (debug UART), GP21/GP26 for SPI (button debounce circuitry), or GP22-25 (onboard LEDs) for anything else.

## Power

- Flex divider. 3V3 to flex sensor to GP40 to 47k to GND
- MPU6050. VIN to 3V3
- TFT. 3V3 with 10 uF + 0.1 uF caps at its power pins
- Servo. Separate 5V supply, grounds tied together

## Testing the ADC

Set `TEST_MODE_RAW_ADC` to 1 in `main.c`, upload, open the serial monitor and bend the sensor. The raw value should move smoothly and never sit at 0 or 4095.

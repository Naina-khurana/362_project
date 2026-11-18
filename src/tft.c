// tft.c — SPI TFT display (wraps the course lcd.c library)
//
// SETUP: copy lcd.c into src/ and lcd.h into include/ from the course
// project-lcd zip. In lcd.c, replace the three "x" defines with
//     #include "pins.h"
//     #define CS_NUM    PIN_CS
//     #define DC_NUM    PIN_DC
//     #define RESET_NUM PIN_nRESET
// and leave "spi1" as is.
//
// Until lcd.h is added, this file compiles to empty stubs so the rest of
// the project still builds and runs.
//
// Wiring: SCK GP10, SDI GP11, CS GP12, DC GP13, RESET GP14, VCC/LED 3V3,
// with 10 uF + 0.1 uF caps right at the display's power pins.

#include <stdio.h>
#include "pico/stdlib.h"
#include "pins.h"
#include "tft.h"

#if __has_include("lcd.h")

#include "hardware/spi.h"
#include "lcd.h"

#define SPI_BAUD     (12 * 1000 * 1000)  // start at 12 MHz, raise once stable

// Screen layout (240 x 320 portrait)
#define TEXT_X       10
#define ROW_MODE     30
#define ROW_BEND     50
#define ROW_PITCH    66
#define ROW_ROLL     82
#define ROW_SERVO    98
#define GRAPH_X0     10
#define GRAPH_X1     229
#define GRAPH_Y0     125
#define GRAPH_Y1     305

static int graph_x = GRAPH_X0;
static int prev_y  = GRAPH_Y1;
static int last_mode = -1;

static void draw_static(void) {
    LCD_Clear(BLACK);
    LCD_DrawString(TEXT_X, 6, WHITE, BLACK, "ProtoPaw", 16, 0);
    LCD_DrawString(TEXT_X, GRAPH_Y0 - 14, GRAY, BLACK, "Finger bend", 12, 0);
    LCD_DrawRectangle(GRAPH_X0 - 1, GRAPH_Y0 - 1, GRAPH_X1 + 1, GRAPH_Y1 + 1, GRAY);
}

void tft_init(void) {
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_nRESET);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_nRESET, GPIO_OUT);
    gpio_put(PIN_CS, 1);       // not selected
    gpio_put(PIN_DC, 0);       // command mode
    gpio_put(PIN_nRESET, 1);   // not in reset

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SDI, GPIO_FUNC_SPI);
    spi_init(LCD_SPI, SPI_BAUD);
    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    LCD_Setup();
    draw_static();
}

// Only redraws what changes, a full-screen redraw is too slow
void tft_update(float bend, float pitch_deg, float roll_deg,
                float servo_deg, int mode) {
    char buf[32];

    if (mode != last_mode) {
        LCD_DrawString(TEXT_X, ROW_MODE, mode ? CYAN : GREEN, BLACK,
                       mode ? "Mode  TILT " : "Mode  FLEX ", 16, 0);
        last_mode = mode;
    }

    snprintf(buf, sizeof buf, "Bend  %3d%%   ", (int)(bend * 100.0f));
    LCD_DrawString(TEXT_X, ROW_BEND, WHITE, BLACK, buf, 16, 0);
    snprintf(buf, sizeof buf, "Pitch %6.1f   ", pitch_deg);
    LCD_DrawString(TEXT_X, ROW_PITCH, WHITE, BLACK, buf, 16, 0);
    snprintf(buf, sizeof buf, "Roll  %6.1f   ", roll_deg);
    LCD_DrawString(TEXT_X, ROW_ROLL, WHITE, BLACK, buf, 16, 0);
    snprintf(buf, sizeof buf, "Servo %5.1f deg   ", servo_deg);
    LCD_DrawString(TEXT_X, ROW_SERVO, YELLOW, BLACK, buf, 16, 0);

    // Scrolling bend graph: erase one column, draw the new segment
    int y = GRAPH_Y1 - (int)(bend * (GRAPH_Y1 - GRAPH_Y0));
    LCD_DrawLine(graph_x, GRAPH_Y0, graph_x, GRAPH_Y1, BLACK);
    if (graph_x > GRAPH_X0) {
        LCD_DrawLine(graph_x - 1, prev_y, graph_x, y, GREEN);
    }
    prev_y = y;

    graph_x++;
    if (graph_x > GRAPH_X1) graph_x = GRAPH_X0;
    // cursor line just ahead of the newest point
    int next = (graph_x > GRAPH_X1 - 1) ? GRAPH_X0 : graph_x + 1;
    LCD_DrawLine(next, GRAPH_Y0, next, GRAPH_Y1, GRAY);
}

#else  // lcd.h not added yet

void tft_init(void) {
    printf("TFT: lcd.h not found, display disabled (see tft.c setup notes)\n");
}

void tft_update(float bend, float pitch_deg, float roll_deg,
                float servo_deg, int mode) {
    (void)bend; (void)pitch_deg; (void)roll_deg; (void)servo_deg; (void)mode;
}

#endif

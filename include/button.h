// button.h — GPIO driver for the mode button (onboard GP21)
#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

void button_init(void);
bool button_is_down(void);    // current debounced state
bool button_pressed(void);    // true once per press (call every loop)

#endif

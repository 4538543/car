#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BUTTON_NONE = 0,
    BUTTON_EQUAL,      /* PB22 */
    BUTTON_A_FAST,     /* PB23 */
    BUTTON_B_FAST      /* PB24 */
} Button_Event;

void Buttons_init(void);

/*
 * Call once every 1 ms. Returns one event only after the input has remained
 * high for 20 consecutive samples. Holding a button does not repeat events.
 */
Button_Event Buttons_poll1ms(void);

#endif

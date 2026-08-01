#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

typedef enum {
    BUTTON_EVENT_NONE               = 0U,
    BUTTON_EVENT_START_LAP_NOW      = (1U << 0),
    BUTTON_EVENT_START_AB_PASS      = (1U << 1),
    BUTTON_EVENT_START_LAP_PASS     = (1U << 2),
    BUTTON_EVENT_STOP               = (1U << 3)
} ButtonEvent;

void Buttons_init(void);
uint32_t Buttons_task1ms(void);

#endif

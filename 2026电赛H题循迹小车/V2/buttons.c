#include "buttons.h"

#include <stdbool.h>

#include "ti_msp_dl_config.h"

#define BUTTON_DEBOUNCE_MS (20U)

typedef struct {
    bool stablePressed;
    uint8_t differentCount;
} ButtonDebouncer;

static ButtonDebouncer gStartAb;
static ButtonDebouncer gStartLap;
static ButtonDebouncer gStop;
static ButtonDebouncer gSpare;

static bool readStartAb(void)
{
    return DL_GPIO_readPins(
        KEY_GPIO_START_AB_PORT, KEY_GPIO_START_AB_PIN) != 0U;
}

static bool readStartLap(void)
{
    return DL_GPIO_readPins(
        KEY_GPIO_START_LAP_PORT, KEY_GPIO_START_LAP_PIN) == 0U;
}

static bool readStop(void)
{
    return DL_GPIO_readPins(
        KEY_GPIO_EMERGENCY_STOP_PORT,
        KEY_GPIO_EMERGENCY_STOP_PIN) == 0U;
}

static bool readSpare(void)
{
    return DL_GPIO_readPins(
        KEY_GPIO_SPARE_PORT, KEY_GPIO_SPARE_PIN) == 0U;
}

static bool updateButton(ButtonDebouncer *button, bool rawPressed)
{
    if (rawPressed == button->stablePressed) {
        button->differentCount = 0U;
        return false;
    }

    if (button->differentCount < BUTTON_DEBOUNCE_MS) {
        button->differentCount++;
    }

    if (button->differentCount >= BUTTON_DEBOUNCE_MS) {
        button->stablePressed = rawPressed;
        button->differentCount = 0U;
        return rawPressed;
    }

    return false;
}

void Buttons_init(void)
{
    gStartAb.stablePressed = readStartAb();
    gStartLap.stablePressed = readStartLap();
    gStop.stablePressed = readStop();
    gSpare.stablePressed = readSpare();

    gStartAb.differentCount = 0U;
    gStartLap.differentCount = 0U;
    gStop.differentCount = 0U;
    gSpare.differentCount = 0U;
}

uint32_t Buttons_task1ms(void)
{
    uint32_t events = BUTTON_EVENT_NONE;

    if (updateButton(&gStartAb, readStartAb())) {
        events |= BUTTON_EVENT_START_AB;
    }
    if (updateButton(&gStartLap, readStartLap())) {
        events |= BUTTON_EVENT_START_LAP;
    }
    if (updateButton(&gStop, readStop())) {
        events |= BUTTON_EVENT_STOP;
    }
    if (updateButton(&gSpare, readSpare())) {
        events |= BUTTON_EVENT_SPARE;
    }

    return events;
}

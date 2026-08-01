#include "buttons.h"

#include <stdbool.h>

#include "ti_msp_dl_config.h"

#define BUTTON_DEBOUNCE_MS (20U)

typedef struct {
    bool stablePressed;
    uint8_t differentCount;
} ButtonDebouncer;

static ButtonDebouncer gStartLapNow;
static ButtonDebouncer gStartAbPass;
static ButtonDebouncer gStartLapPass;
static ButtonDebouncer gStop;

static bool readStartLapNow(void)
{
    return DL_GPIO_readPins(
        KEY_GPIO_START_LAP_NOW_PORT,
        KEY_GPIO_START_LAP_NOW_PIN) != 0U;
}

static bool readStartAbPass(void)
{
    return DL_GPIO_readPins(
        KEY_GPIO_START_AB_PASS_PORT,
        KEY_GPIO_START_AB_PASS_PIN) == 0U;
}

static bool readStartLapPass(void)
{
    return DL_GPIO_readPins(
        KEY_GPIO_START_LAP_PASS_PORT,
        KEY_GPIO_START_LAP_PASS_PIN) == 0U;
}

static bool readStop(void)
{
    return DL_GPIO_readPins(
        KEY_GPIO_EMERGENCY_STOP_PORT,
        KEY_GPIO_EMERGENCY_STOP_PIN) == 0U;
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
    gStartLapNow.stablePressed = readStartLapNow();
    gStartAbPass.stablePressed = readStartAbPass();
    gStartLapPass.stablePressed = readStartLapPass();
    gStop.stablePressed = readStop();

    gStartLapNow.differentCount = 0U;
    gStartAbPass.differentCount = 0U;
    gStartLapPass.differentCount = 0U;
    gStop.differentCount = 0U;
}

uint32_t Buttons_task1ms(void)
{
    uint32_t events = BUTTON_EVENT_NONE;

    if (updateButton(&gStartLapNow, readStartLapNow())) {
        events |= BUTTON_EVENT_START_LAP_NOW;
    }
    if (updateButton(&gStartAbPass, readStartAbPass())) {
        events |= BUTTON_EVENT_START_AB_PASS;
    }
    if (updateButton(&gStartLapPass, readStartLapPass())) {
        events |= BUTTON_EVENT_START_LAP_PASS;
    }
    if (updateButton(&gStop, readStop())) {
        events |= BUTTON_EVENT_STOP;
    }

    return events;
}

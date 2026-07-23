#include "button.h"

#include <stdint.h>

#include "app_config.h"
#include "ti_msp_dl_config.h"

#define BUTTON_DEBOUNCE_MS (20U)

static uint8_t gLowCount;
static bool gHeld;

void Button_init(void)
{
    gLowCount = 0U;
    gHeld = false;
}

bool Button_pollPressed1ms(void)
{
    bool isLow =
        (DL_GPIO_readPins(START_KEY_PORT, START_KEY_PIN) & START_KEY_PIN) == 0U;

    if (isLow) {
        if (gLowCount < BUTTON_DEBOUNCE_MS) {
            gLowCount++;
        }
        if ((!gHeld) && (gLowCount >= BUTTON_DEBOUNCE_MS)) {
            gHeld = true;
            return true;
        }
    } else {
        gLowCount = 0U;
        gHeld = false;
    }

    return false;
}

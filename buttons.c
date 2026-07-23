#include "buttons.h"

#include "ti_msp_dl_config.h"

#define BUTTON_EQUAL_PIN       (DL_GPIO_PIN_22)
#define BUTTON_A_FAST_PIN      (DL_GPIO_PIN_23)
#define BUTTON_B_FAST_PIN      (DL_GPIO_PIN_24)
#define BUTTON_ALL_PINS        (BUTTON_EQUAL_PIN | BUTTON_A_FAST_PIN | \
                                BUTTON_B_FAST_PIN)
#define BUTTON_DEBOUNCE_MS     (20U)

typedef struct {
    uint8_t stableHighCount;
    bool pressed;
} Button_State;

static Button_State gButtonStates[3];

static bool Buttons_updateOne(Button_State *state, bool rawHigh)
{
    if (rawHigh) {
        if (state->stableHighCount < BUTTON_DEBOUNCE_MS) {
            state->stableHighCount++;
        }

        if ((!state->pressed) &&
            (state->stableHighCount >= BUTTON_DEBOUNCE_MS)) {
            state->pressed = true;
            return true;
        }
    } else {
        state->stableHighCount = 0U;
        state->pressed = false;
    }

    return false;
}

void Buttons_init(void)
{
    uint32_t i;

    for (i = 0U; i < 3U; i++) {
        gButtonStates[i].stableHighCount = 0U;
        gButtonStates[i].pressed = false;
    }
}

Button_Event Buttons_poll1ms(void)
{
    uint32_t pins = DL_GPIO_readPins(GPIOB, BUTTON_ALL_PINS);
    bool equalPressed;
    bool aFastPressed;
    bool bFastPressed;

    equalPressed = Buttons_updateOne(
        &gButtonStates[0], (pins & BUTTON_EQUAL_PIN) != 0U);
    aFastPressed = Buttons_updateOne(
        &gButtonStates[1], (pins & BUTTON_A_FAST_PIN) != 0U);
    bFastPressed = Buttons_updateOne(
        &gButtonStates[2], (pins & BUTTON_B_FAST_PIN) != 0U);

    /* When multiple keys become valid together, PB22 > PB23 > PB24. */
    if (equalPressed) {
        return BUTTON_EQUAL;
    }
    if (aFastPressed) {
        return BUTTON_A_FAST;
    }
    if (bFastPressed) {
        return BUTTON_B_FAST;
    }

    return BUTTON_NONE;
}

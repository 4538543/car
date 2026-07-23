#include "task_buttons.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "ti_msp_dl_config.h"

typedef struct {
    uint8_t pressedMs;
    uint8_t releasedMs;
    bool held;
    bool armed;
} KeyState;

static KeyState gKeys[4];

static bool update(KeyState *key, bool pressed)
{
    if (!pressed) {
        key->pressedMs = 0U;
        key->held = false;
        if (key->releasedMs < BUTTON_DEBOUNCE_MS) {
            key->releasedMs++;
        }
        if (key->releasedMs >= BUTTON_DEBOUNCE_MS) {
            key->armed = true;
        }
        return false;
    }

    key->releasedMs = 0U;
    if (!key->armed) {
        return false;
    }
    if (key->pressedMs < BUTTON_DEBOUNCE_MS) {
        key->pressedMs++;
    }
    if ((!key->held) && (key->pressedMs >= BUTTON_DEBOUNCE_MS)) {
        key->held = true;
        return true;
    }
    return false;
}

void TaskButtons_init(void)
{
    uint8_t i;

    /* Enforce the development-board schematic polarities in software too. */
    DL_GPIO_initDigitalInputFeatures(START_KEY_KEY1_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(MOTOR_TEST_KEYS_MOTOR_TEST_ALT_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(MOTOR_TEST_KEYS_MOTOR_TEST_STOP_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(MOTOR_TEST_KEYS_MOTOR_TEST_DISTANCE_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    for (i = 0U; i < 4U; i++) {
        gKeys[i].pressedMs = 0U;
        gKeys[i].releasedMs = 0U;
        gKeys[i].held = false;
        gKeys[i].armed = false;
    }
}

TaskButton_Event TaskButtons_poll1ms(void)
{
    uint32_t pa = DL_GPIO_readPins(Q1_KEY_PORT, Q1_KEY_PIN);
    uint32_t pb = DL_GPIO_readPins(MODE_KEY_PORT,
        Q2_KEY_PIN | Q3_KEY_PIN | Q4_KEY_PIN);
    bool q1 = update(&gKeys[0], (pa & Q1_KEY_PIN) != 0U);
    bool q2 = update(&gKeys[1], (pb & Q2_KEY_PIN) == 0U);
    bool q3 = update(&gKeys[2], (pb & Q3_KEY_PIN) == 0U);
    bool q4 = update(&gKeys[3], (pb & Q4_KEY_PIN) == 0U);

    if (q4) return TASK_BUTTON_Q4;
    if (q3) return TASK_BUTTON_Q3;
    if (q2) return TASK_BUTTON_Q2;
    if (q1) return TASK_BUTTON_Q1;
    return TASK_BUTTON_NONE;
}

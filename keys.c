#include "keys.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"

typedef struct {
    uint8_t pressedMs;
    uint8_t releasedMs;
    uint8_t eventSent;
    uint8_t armed;
} Key_State;

static Key_State g_keys[4];

void Keys_init(void)
{
    uint8_t i;

    for (i = 0U; i < 4U; i++) {
        g_keys[i].pressedMs = 0U;
        g_keys[i].releasedMs = 0U;
        g_keys[i].eventSent = 0U;
        g_keys[i].armed = 0U;
    }
}

Key_Event Keys_task1ms(void)
{
    uint32_t portB = DL_GPIO_readPins(
        GPIOB,
        MISSION_KEYS_KEY_Q2_PIN |
        MISSION_KEYS_KEY_Q3_PIN |
        MISSION_KEYS_KEY_Q4_PIN);
    uint8_t pressed[4] = {
        DL_GPIO_readPins(GPIOA, START_KEY_KEY1_PIN) == 0U,
        (portB & MISSION_KEYS_KEY_Q2_PIN) == 0U,
        (portB & MISSION_KEYS_KEY_Q3_PIN) == 0U,
        (portB & MISSION_KEYS_KEY_Q4_PIN) == 0U
    };
    uint8_t i;

    /*
     * Each key arms independently after being released for 100 ms. This
     * rejects startup-low glitches without allowing one faulty key to lock
     * every mission key.
     */
    for (i = 0U; i < 4U; i++) {
        if (g_keys[i].armed == 0U) {
            if (pressed[i] == 0U) {
                if (g_keys[i].releasedMs < 100U) {
                    g_keys[i].releasedMs++;
                }
                if (g_keys[i].releasedMs >= 100U) {
                    g_keys[i].armed = 1U;
                }
            } else {
                g_keys[i].releasedMs = 0U;
            }
            continue;
        }

        if (pressed[i] != 0U) {
            if (g_keys[i].pressedMs < KEY_DEBOUNCE_MS) {
                g_keys[i].pressedMs++;
            }
            if (g_keys[i].eventSent == 0U &&
                g_keys[i].pressedMs >= KEY_DEBOUNCE_MS) {
                g_keys[i].eventSent = 1U;
                return (Key_Event)(i + 1U);
            }
        } else {
            g_keys[i].pressedMs = 0U;
            g_keys[i].eventSent = 0U;
        }
    }

    return KEY_NONE;
}

#include "indicator.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "ti_msp_dl_config.h"

static uint16_t gBuzzerRemainMs;
static bool gKeepLedOn;

static void Indicator_led(bool on)
{
    if (on) {
        DL_GPIO_clearPins(INDICATOR_LED_PORT, INDICATOR_LED_PIN);
    } else {
        DL_GPIO_setPins(INDICATOR_LED_PORT, INDICATOR_LED_PIN);
    }
}

static void Indicator_buzzer(bool on)
{
    if (on) {
        DL_GPIO_setPins(INDICATOR_BUZZER_PORT, INDICATOR_BUZZER_PIN);
    } else {
        DL_GPIO_clearPins(INDICATOR_BUZZER_PORT, INDICATOR_BUZZER_PIN);
    }
}

void Indicator_init(void)
{
    gBuzzerRemainMs = 0U;
    gKeepLedOn = false;
    Indicator_led(false);
    Indicator_buzzer(false);
}

void Indicator_start(void)
{
    gKeepLedOn = false;
    gBuzzerRemainMs = 100U;
    Indicator_led(true);
    Indicator_buzzer(true);
}

void Indicator_finished(void)
{
    gKeepLedOn = true;
    gBuzzerRemainMs = 400U;
    Indicator_led(true);
    Indicator_buzzer(true);
}

void Indicator_fault(void)
{
    gKeepLedOn = true;
    gBuzzerRemainMs = 1000U;
    Indicator_led(true);
    Indicator_buzzer(true);
}

void Indicator_idle(void)
{
    gKeepLedOn = false;
    gBuzzerRemainMs = 0U;
    Indicator_led(false);
    Indicator_buzzer(false);
}

void Indicator_task1ms(void)
{
    if (gBuzzerRemainMs > 0U) {
        gBuzzerRemainMs--;
        if (gBuzzerRemainMs == 0U) {
            Indicator_buzzer(false);
            if (!gKeepLedOn) {
                Indicator_led(false);
            }
        }
    }
}

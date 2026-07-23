#include "line_sensor.h"

#include <stddef.h>

#include "app_config.h"
#include "ti_msp_dl_config.h"

/* Probe 1 is the leftmost probe and probe 8 is the rightmost probe. */
static const int16_t gWeights[8] = {
    -3500, -2500, -1500, -500, 500, 1500, 2500, 3500
};

static void LineSensor_delayUs(uint32_t us)
{
    delay_cycles(((CPUCLK_FREQ / 1000000U) * us) + 1U);
}

void LineSensor_init(void)
{
    /* The helper board updates DAT in its CLK rising-edge ISR. Keep CLK low
     * while idle so every frame starts with a real rising edge. */
    DL_GPIO_clearPins(LINE_SERIAL_PORT, LINE_CLK_PIN);
    /* More than 1 ms without a clock resets the helper board bit index. */
    LineSensor_delayUs(1200U);
}

bool LineSensor_read(LineSensor_Frame *frame)
{
    uint8_t i;
    uint8_t raw = 0U;
    uint8_t blackMask;
    uint8_t blackCount = 0U;
    int32_t weighted = 0;

    if (frame == NULL) {
        return false;
    }

    /* Official auxiliary-board protocol: bit 0 is probe 1. */
    for (i = 0U; i < 8U; i++) {
        DL_GPIO_setPins(LINE_SERIAL_PORT, LINE_CLK_PIN);
        LineSensor_delayUs(5U);
        DL_GPIO_clearPins(LINE_SERIAL_PORT, LINE_CLK_PIN);
        LineSensor_delayUs(1U);

        if ((DL_GPIO_readPins(LINE_SERIAL_PORT, LINE_DAT_PIN) &
             LINE_DAT_PIN) != 0U) {
            raw |= (uint8_t) (1U << i);
        }

    }
    /* Leave CLK low; the 5 ms caller period resets the board to bit 0. */
    DL_GPIO_clearPins(LINE_SERIAL_PORT, LINE_CLK_PIN);

    /* The vendor example shows 11100111 when probes 4 and 5 are black. */
    blackMask = (uint8_t) ~raw;
    for (i = 0U; i < 8U; i++) {
        if ((blackMask & (uint8_t) (1U << i)) != 0U) {
            blackCount++;
            weighted += gWeights[i];
        }
    }

    frame->rawBits = raw;
    frame->blackMask = blackMask;
    frame->blackCount = blackCount;
    frame->lineFound = (blackCount != 0U);
    frame->error = frame->lineFound
        ? (int16_t) (weighted / (int32_t) blackCount)
        : 0;
    frame->valid = true;
    return true;
}

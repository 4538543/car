#include "gray_sensor.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"

static GraySensorSample gSample;

static void delayMicroseconds(uint32_t microseconds)
{
    uint32_t cyclesPerMicrosecond = CPUCLK_FREQ / 1000000U;

    DL_Common_delayCycles(cyclesPerMicrosecond * microseconds);
}

static uint8_t readSerialByte(void)
{
    uint8_t raw = 0U;
    uint8_t index;

    DL_GPIO_clearPins(GRAY_GPIO_PORT, GRAY_GPIO_GRAY_CLK_PIN);

    for (index = 0U; index < 8U; index++) {
        /*
         * The Ganwei auxiliary-board reference code updates DAT on the CLK
         * rising edge. Allow about 5 us for its GPIO interrupt to complete,
         * then sample DAT after returning CLK low.
         */
        DL_GPIO_setPins(GRAY_GPIO_PORT, GRAY_GPIO_GRAY_CLK_PIN);
        delayMicroseconds(GRAY_SERIAL_HIGH_US);
        DL_GPIO_clearPins(GRAY_GPIO_PORT, GRAY_GPIO_GRAY_CLK_PIN);

        if ((DL_GPIO_readPins(GRAY_GPIO_PORT, GRAY_GPIO_DAT_PIN) &
                GRAY_GPIO_DAT_PIN) != 0U) {
            raw |= (uint8_t)(1U << index);
        }

        delayMicroseconds(GRAY_SERIAL_LOW_US);
    }

    return raw;
}

void GraySensor_init(void)
{
    DL_GPIO_clearPins(GRAY_GPIO_PORT, GRAY_GPIO_GRAY_CLK_PIN);
    gSample.raw = 0U;
    gSample.blackMask = 0U;
    gSample.lineError = 0;
    gSample.lineValid = false;
}

void GraySensor_sample(void)
{
    uint8_t raw = readSerialByte();
    uint8_t blackMask = 0U;
    uint8_t activeCount = 0U;
    uint8_t index;
    int32_t weightedSum = 0;

    for (index = 0U; index < 8U; index++) {
        uint8_t level = (uint8_t)((raw >> index) & 0x01U);

        if (level == GRAY_BLACK_LEVEL) {
            int32_t position;

            blackMask |= (uint8_t)(1U << index);
            activeCount++;

#if GRAY_BIT0_IS_LEFT
            position = 350 - ((int32_t)index * 100);
#else
            position = -350 + ((int32_t)index * 100);
#endif
            weightedSum += position;
        }
    }

    gSample.raw = raw;
    gSample.blackMask = blackMask;

    /*
     * An 18 mm line under sensors spaced 12 mm apart normally activates one
     * or two channels. Zero active channels means the line is lost; too many
     * active channels also covers a disconnected DAT line when black is 0.
     */
    if ((activeCount == 0U) ||
        (activeCount > GRAY_MAX_ACTIVE_SENSORS)) {
        gSample.lineError = 0;
        gSample.lineValid = false;
        return;
    }

    gSample.lineError = (int16_t)(weightedSum / (int32_t)activeCount);
    gSample.lineValid = true;
}

void GraySensor_getSample(GraySensorSample *sample)
{
    *sample = gSample;
}

bool GraySensor_hasLine(void)
{
    return gSample.lineValid;
}

int16_t GraySensor_getLineError(void)
{
    return gSample.lineError;
}

uint8_t GraySensor_getRaw(void)
{
    return gSample.raw;
}

uint8_t GraySensor_getBlackMask(void)
{
    return gSample.blackMask;
}

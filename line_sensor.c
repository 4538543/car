#include "line_sensor.h"
#include "app_config.h"
#include "ti_msp_dl_config.h"

void LineSensor_init(void)
{
    /*
     * The auxiliary board resets its serial bit index when CLK remains low
     * for more than 1 ms.
     */
    DL_GPIO_clearPins(GPIOB, GRAY_SERIAL_GRAY_CLK_PIN);
    delay_cycles(40000U);
}

bool LineSensor_blackLine(void)
{
    uint8_t i;
    uint8_t blackCount = 0U;

    for (i = 0U; i < 8U; i++) {
        /* The board updates DAT from its CLK rising-edge handler. */
        DL_GPIO_setPins(GPIOB, GRAY_SERIAL_GRAY_CLK_PIN);
        delay_cycles(160U);
        DL_GPIO_clearPins(GPIOB, GRAY_SERIAL_GRAY_CLK_PIN);
        delay_cycles(32U);

        /* Vendor protocol: low means this probe sees the black line. */
        if ((DL_GPIO_readPins(GPIOB, GRAY_SERIAL_GRAY_DAT_PIN) &
             GRAY_SERIAL_GRAY_DAT_PIN) == 0U) {
            blackCount++;
        }
    }

    DL_GPIO_clearPins(GPIOB, GRAY_SERIAL_GRAY_CLK_PIN);
    return blackCount >= BLACK_SENSOR_MIN;
}

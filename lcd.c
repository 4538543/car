#include "lcd.h"
#include "ti_msp_dl_config.h"

#include <limits.h>

#define LCD_WIDTH   240U
#define LCD_HEIGHT  320U
#define LCD_BLACK   0x0000U
#define LCD_WHITE   0xFFFFU

static void writeByte(uint8_t value)
{
    uint8_t i;

    for (i = 0U; i < 8U; i++) {
        if ((value & 0x80U) != 0U) {
            DL_GPIO_setPins(GPIOB, LCD_SERIAL_LCD_SDI_PIN);
        } else {
            DL_GPIO_clearPins(GPIOB, LCD_SERIAL_LCD_SDI_PIN);
        }
        DL_GPIO_setPins(GPIOA, LCD_PORTA_LCD_CLK_PIN);
        DL_GPIO_clearPins(GPIOA, LCD_PORTA_LCD_CLK_PIN);
        value <<= 1;
    }
}

static void command(uint8_t value)
{
    DL_GPIO_clearPins(GPIOB, LCD_CTRL_LCD_DC_PIN);
    DL_GPIO_clearPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
    writeByte(value);
    DL_GPIO_setPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
}

static void data8(uint8_t value)
{
    DL_GPIO_setPins(GPIOB, LCD_CTRL_LCD_DC_PIN);
    DL_GPIO_clearPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
    writeByte(value);
    DL_GPIO_setPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
}

static void dataList(const uint8_t *data, uint8_t count)
{
    uint8_t i;

    DL_GPIO_setPins(GPIOB, LCD_CTRL_LCD_DC_PIN);
    DL_GPIO_clearPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
    for (i = 0U; i < count; i++) {
        writeByte(data[i]);
    }
    DL_GPIO_setPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
}

static void address(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t values[4];

    command(0x2AU);
    values[0] = (uint8_t)(x0 >> 8);
    values[1] = (uint8_t)x0;
    values[2] = (uint8_t)(x1 >> 8);
    values[3] = (uint8_t)x1;
    dataList(values, 4U);

    command(0x2BU);
    values[0] = (uint8_t)(y0 >> 8);
    values[1] = (uint8_t)y0;
    values[2] = (uint8_t)(y1 >> 8);
    values[3] = (uint8_t)y1;
    dataList(values, 4U);
    command(0x2CU);
}

static void fillRect(
    uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint32_t pixels;
    uint8_t high = (uint8_t)(color >> 8);
    uint8_t low = (uint8_t)color;

    address(x0, y0, x1, y1);
    pixels = ((uint32_t)x1 - x0 + 1U) * ((uint32_t)y1 - y0 + 1U);

    /* Keep CS asserted for the complete pixel stream. */
    DL_GPIO_setPins(GPIOB, LCD_CTRL_LCD_DC_PIN);
    DL_GPIO_clearPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
    while (pixels-- != 0U) {
        writeByte(high);
        writeByte(low);
    }
    DL_GPIO_setPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
}

static void initCommand(uint8_t cmd, const uint8_t *data, uint8_t count)
{
    command(cmd);
    if (count != 0U) {
        dataList(data, count);
    }
}

void Lcd_init(void)
{
    static const uint8_t powerB[] = {0x00U, 0xC1U, 0x30U};
    static const uint8_t powerSeq[] = {0x64U, 0x03U, 0x12U, 0x81U};
    static const uint8_t driverA[] = {0x85U, 0x00U, 0x78U};
    static const uint8_t powerA[] = {0x39U, 0x2CU, 0x00U, 0x34U, 0x02U};
    static const uint8_t pump[] = {0x20U};
    static const uint8_t driverB[] = {0x00U, 0x00U};
    static const uint8_t vcom[] = {0x3EU, 0x28U};
    static const uint8_t displayFunction[] = {0x08U, 0x82U, 0x27U};

    DL_GPIO_setPins(GPIOB, LCD_SERIAL_LCD_CS_PIN);
    DL_GPIO_clearPins(GPIOA, LCD_PORTA_LCD_CLK_PIN);
    DL_GPIO_setPins(GPIOA, LCD_PORTA_LCD_BLK_PIN);

    command(0x01U);
    delay_cycles(3200000U);
    initCommand(0xCFU, powerB, sizeof(powerB));
    initCommand(0xEDU, powerSeq, sizeof(powerSeq));
    initCommand(0xE8U, driverA, sizeof(driverA));
    initCommand(0xCBU, powerA, sizeof(powerA));
    initCommand(0xF7U, pump, sizeof(pump));
    initCommand(0xEAU, driverB, sizeof(driverB));
    command(0xC0U); data8(0x23U);
    command(0xC1U); data8(0x10U);
    initCommand(0xC5U, vcom, sizeof(vcom));
    command(0xC7U); data8(0x86U);
    command(0x36U); data8(0x48U);
    command(0x3AU); data8(0x55U);
    command(0xB1U); data8(0x00U); data8(0x18U);
    initCommand(0xB6U, displayFunction, sizeof(displayFunction));
    command(0x11U);
    delay_cycles(3840000U);
    command(0x29U);
    delay_cycles(640000U);

    fillRect(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U, LCD_BLACK);
}

static void bar(uint8_t x, uint8_t y, uint8_t h, uint8_t w)
{
    fillRect(x, y, (uint16_t)x + w - 1U, (uint16_t)y + h - 1U, LCD_WHITE);
}

static void digit(uint8_t x, uint8_t y, uint8_t value)
{
    static const uint8_t masks[10] = {
        0x3FU, 0x06U, 0x5BU, 0x4FU, 0x66U,
        0x6DU, 0x7DU, 0x07U, 0x7FU, 0x6FU
    };
    uint8_t mask = masks[value];

    if ((mask & 0x01U) != 0U) bar(x + 2U, y, 3U, 14U);
    if ((mask & 0x02U) != 0U) bar(x + 16U, y + 2U, 14U, 3U);
    if ((mask & 0x04U) != 0U) bar(x + 16U, y + 18U, 14U, 3U);
    if ((mask & 0x08U) != 0U) bar(x + 2U, y + 32U, 3U, 14U);
    if ((mask & 0x10U) != 0U) bar(x, y + 18U, 14U, 3U);
    if ((mask & 0x20U) != 0U) bar(x, y + 2U, 14U, 3U);
    if ((mask & 0x40U) != 0U) bar(x + 2U, y + 16U, 3U, 14U);
}

void Lcd_showYaw(int16_t yawDeciDeg)
{
    static int16_t previous = INT16_MIN;
    uint16_t absolute;

    if (yawDeciDeg == previous) {
        return;
    }
    previous = yawDeciDeg;
    absolute = (yawDeciDeg < 0)
        ? (uint16_t)(-(int32_t)yawDeciDeg)
        : (uint16_t)yawDeciDeg;

    /* Clear only the number area instead of rewriting the whole LCD. */
    fillRect(4U, 36U, 174U, 78U, LCD_BLACK);
    if (yawDeciDeg < 0) {
        bar(8U, 56U, 3U, 14U);
    }
    digit(40U, 40U, (uint8_t)((absolute / 1000U) % 10U));
    digit(72U, 40U, (uint8_t)((absolute / 100U) % 10U));
    digit(104U, 40U, (uint8_t)((absolute / 10U) % 10U));
    bar(136U, 70U, 5U, 5U);
    digit(152U, 40U, (uint8_t)(absolute % 10U));
}

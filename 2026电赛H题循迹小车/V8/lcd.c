#include "lcd.h"

#include <stdbool.h>
#include <stddef.h>

#include "ti_msp_dl_config.h"

#define ILI9341_SOFTWARE_RESET             (0x01U)
#define ILI9341_DISPLAY_OFF                (0x28U)
#define ILI9341_DISPLAY_ON                 (0x29U)
#define ILI9341_COLUMN_ADDRESS_SET         (0x2AU)
#define ILI9341_PAGE_ADDRESS_SET           (0x2BU)
#define ILI9341_MEMORY_WRITE               (0x2CU)
#define ILI9341_MEMORY_ACCESS_CONTROL      (0x36U)
#define ILI9341_PIXEL_FORMAT_SET           (0x3AU)
#define ILI9341_SLEEP_OUT                  (0x11U)

/*
 * Five-column, seven-row glyphs for ASCII 0x20 through 0x5A.
 * The interface only uses uppercase text, digits, and punctuation.
 */
static const uint8_t gFont5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* space */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* $ */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* % */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* & */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* ' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* ( */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* ) */
    {0x14, 0x08, 0x3E, 0x08, 0x14}, /* * */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* + */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* , */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* - */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* / */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* : */
    {0x00, 0x56, 0x36, 0x00, 0x00}, /* ; */
    {0x08, 0x14, 0x22, 0x41, 0x00}, /* < */
    {0x14, 0x14, 0x14, 0x14, 0x14}, /* = */
    {0x00, 0x41, 0x22, 0x14, 0x08}, /* > */
    {0x02, 0x01, 0x51, 0x09, 0x06}, /* ? */
    {0x32, 0x49, 0x79, 0x41, 0x3E}, /* @ */
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* A */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* B */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* C */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* D */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* E */
    {0x7F, 0x09, 0x09, 0x09, 0x01}, /* F */
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, /* G */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* H */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* I */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* J */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* K */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* L */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, /* M */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* N */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* O */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* P */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* Q */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* R */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* S */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* T */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* U */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* V */
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, /* W */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* X */
    {0x07, 0x08, 0x70, 0x08, 0x07}, /* Y */
    {0x61, 0x51, 0x49, 0x45, 0x43}  /* Z */
};

static void delayMs(uint32_t milliseconds)
{
    while (milliseconds > 0U) {
        delay_cycles(32000U);
        milliseconds--;
    }
}

static void spiWriteByte(uint8_t byte)
{
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++) {
        DL_GPIO_clearPins(LCD_GPIO_LCD_CLK_PORT, LCD_GPIO_LCD_CLK_PIN);

        if ((byte & 0x80U) != 0U) {
            DL_GPIO_setPins(LCD_GPIO_SDI_PORT, LCD_GPIO_SDI_PIN);
        } else {
            DL_GPIO_clearPins(LCD_GPIO_SDI_PORT, LCD_GPIO_SDI_PIN);
        }

        DL_GPIO_setPins(LCD_GPIO_LCD_CLK_PORT, LCD_GPIO_LCD_CLK_PIN);
        byte <<= 1U;
    }

    DL_GPIO_clearPins(LCD_GPIO_LCD_CLK_PORT, LCD_GPIO_LCD_CLK_PIN);
}

static void writeCommand(uint8_t command)
{
    DL_GPIO_clearPins(LCD_GPIO_CS_PORT, LCD_GPIO_CS_PIN);
    DL_GPIO_clearPins(LCD_GPIO_DC_PORT, LCD_GPIO_DC_PIN);
    spiWriteByte(command);
    DL_GPIO_setPins(LCD_GPIO_CS_PORT, LCD_GPIO_CS_PIN);
}

static void writeData(const uint8_t *data, uint8_t length)
{
    uint8_t index;

    DL_GPIO_clearPins(LCD_GPIO_CS_PORT, LCD_GPIO_CS_PIN);
    DL_GPIO_setPins(LCD_GPIO_DC_PORT, LCD_GPIO_DC_PIN);

    for (index = 0U; index < length; index++) {
        spiWriteByte(data[index]);
    }

    DL_GPIO_setPins(LCD_GPIO_CS_PORT, LCD_GPIO_CS_PIN);
}

static void writeRegister(
    uint8_t command, const uint8_t *data, uint8_t length)
{
    writeCommand(command);
    writeData(data, length);
}

static void setAddressWindow(
    uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    data[0] = (uint8_t) (x0 >> 8U);
    data[1] = (uint8_t) x0;
    data[2] = (uint8_t) (x1 >> 8U);
    data[3] = (uint8_t) x1;
    writeRegister(ILI9341_COLUMN_ADDRESS_SET, data, 4U);

    data[0] = (uint8_t) (y0 >> 8U);
    data[1] = (uint8_t) y0;
    data[2] = (uint8_t) (y1 >> 8U);
    data[3] = (uint8_t) y1;
    writeRegister(ILI9341_PAGE_ADDRESS_SET, data, 4U);

    writeCommand(ILI9341_MEMORY_WRITE);
}

static void beginPixelStream(void)
{
    DL_GPIO_clearPins(LCD_GPIO_CS_PORT, LCD_GPIO_CS_PIN);
    DL_GPIO_setPins(LCD_GPIO_DC_PORT, LCD_GPIO_DC_PIN);
}

static void endPixelStream(void)
{
    DL_GPIO_setPins(LCD_GPIO_CS_PORT, LCD_GPIO_CS_PIN);
}

static void writePixelColor(uint16_t color)
{
    spiWriteByte((uint8_t) (color >> 8U));
    spiWriteByte((uint8_t) color);
}

static const uint8_t *getGlyph(char character)
{
    uint8_t index;

    if ((character >= 'a') && (character <= 'z')) {
        character = (char) (character - ('a' - 'A'));
    }

    if ((character < ' ') || (character > 'Z')) {
        character = '?';
    }

    index = (uint8_t) character - (uint8_t) ' ';
    return gFont5x7[index];
}

static void drawCharacter(uint16_t x, uint16_t y, char character,
    uint8_t scale, uint16_t foreground, uint16_t background)
{
    const uint8_t *glyph = getGlyph(character);
    uint8_t sourceRow;
    uint8_t repeatRow;
    uint8_t sourceColumn;
    uint8_t repeatColumn;
    uint16_t width = (uint16_t) (6U * scale);
    uint16_t height = (uint16_t) (8U * scale);

    if ((scale == 0U) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT) ||
        ((uint32_t) x + width > LCD_WIDTH) ||
        ((uint32_t) y + height > LCD_HEIGHT)) {
        return;
    }

    setAddressWindow(x, y, (uint16_t) (x + width - 1U),
        (uint16_t) (y + height - 1U));
    beginPixelStream();

    for (sourceRow = 0U; sourceRow < 8U; sourceRow++) {
        for (repeatRow = 0U; repeatRow < scale; repeatRow++) {
            for (sourceColumn = 0U; sourceColumn < 6U; sourceColumn++) {
                bool lit = (sourceColumn < 5U) && (sourceRow < 7U) &&
                    ((glyph[sourceColumn] & (1U << sourceRow)) != 0U);
                uint16_t color = lit ? foreground : background;

                for (repeatColumn = 0U;
                     repeatColumn < scale;
                     repeatColumn++) {
                    writePixelColor(color);
                }
            }
        }
    }

    endPixelStream();
}

void Lcd_init(void)
{
    static const uint8_t powerControlA[] = {0x39U, 0x2CU, 0x00U, 0x34U, 0x02U};
    static const uint8_t powerControlB[] = {0x00U, 0xC1U, 0x30U};
    static const uint8_t driverTimingA[] = {0x85U, 0x00U, 0x78U};
    static const uint8_t driverTimingB[] = {0x00U, 0x00U};
    static const uint8_t powerOnSequence[] = {0x64U, 0x03U, 0x12U, 0x81U};
    static const uint8_t pumpRatio[] = {0x20U};
    static const uint8_t power1[] = {0x23U};
    static const uint8_t power2[] = {0x10U};
    static const uint8_t vcom1[] = {0x3EU, 0x28U};
    static const uint8_t vcom2[] = {0x86U};
    static const uint8_t memoryAccess[] = {0x28U};
    static const uint8_t pixelFormat[] = {0x55U};
    static const uint8_t frameRate[] = {0x00U, 0x18U};
    static const uint8_t displayFunction[] = {0x08U, 0x82U, 0x27U};
    static const uint8_t gammaDisable[] = {0x00U};
    static const uint8_t gammaCurve[] = {0x01U};
    static const uint8_t positiveGamma[] = {
        0x0FU, 0x31U, 0x2BU, 0x0CU, 0x0EU, 0x08U, 0x4EU, 0xF1U,
        0x37U, 0x07U, 0x10U, 0x03U, 0x0EU, 0x09U, 0x00U
    };
    static const uint8_t negativeGamma[] = {
        0x00U, 0x0EU, 0x14U, 0x03U, 0x11U, 0x07U, 0x31U, 0xC1U,
        0x48U, 0x08U, 0x0FU, 0x0CU, 0x31U, 0x36U, 0x0FU
    };

    DL_GPIO_clearPins(LCD_GPIO_BLK_PORT, LCD_GPIO_BLK_PIN);
    DL_GPIO_setPins(LCD_GPIO_CS_PORT, LCD_GPIO_CS_PIN);
    DL_GPIO_clearPins(LCD_GPIO_DC_PORT, LCD_GPIO_DC_PIN);
    DL_GPIO_clearPins(LCD_GPIO_LCD_CLK_PORT, LCD_GPIO_LCD_CLK_PIN);
    DL_GPIO_clearPins(LCD_GPIO_SDI_PORT, LCD_GPIO_SDI_PIN);
    delayMs(20U);

    /*
     * No LCD reset wire was assigned, so use the controller's software-reset
     * command. The LCD module's RST pin must be pulled high on the module.
     */
    writeCommand(ILI9341_SOFTWARE_RESET);
    delayMs(120U);
    writeCommand(ILI9341_DISPLAY_OFF);

    writeRegister(0xCBU, powerControlA, sizeof(powerControlA));
    writeRegister(0xCFU, powerControlB, sizeof(powerControlB));
    writeRegister(0xE8U, driverTimingA, sizeof(driverTimingA));
    writeRegister(0xEAU, driverTimingB, sizeof(driverTimingB));
    writeRegister(0xEDU, powerOnSequence, sizeof(powerOnSequence));
    writeRegister(0xF7U, pumpRatio, sizeof(pumpRatio));
    writeRegister(0xC0U, power1, sizeof(power1));
    writeRegister(0xC1U, power2, sizeof(power2));
    writeRegister(0xC5U, vcom1, sizeof(vcom1));
    writeRegister(0xC7U, vcom2, sizeof(vcom2));
    writeRegister(
        ILI9341_MEMORY_ACCESS_CONTROL, memoryAccess, sizeof(memoryAccess));
    writeRegister(ILI9341_PIXEL_FORMAT_SET, pixelFormat, sizeof(pixelFormat));
    writeRegister(0xB1U, frameRate, sizeof(frameRate));
    writeRegister(0xB6U, displayFunction, sizeof(displayFunction));
    writeRegister(0xF2U, gammaDisable, sizeof(gammaDisable));
    writeRegister(0x26U, gammaCurve, sizeof(gammaCurve));
    writeRegister(0xE0U, positiveGamma, sizeof(positiveGamma));
    writeRegister(0xE1U, negativeGamma, sizeof(negativeGamma));

    writeCommand(ILI9341_SLEEP_OUT);
    delayMs(120U);
    writeCommand(ILI9341_DISPLAY_ON);
    delayMs(20U);

    Lcd_fillScreen(LCD_COLOR_BLACK);
    DL_GPIO_setPins(LCD_GPIO_BLK_PORT, LCD_GPIO_BLK_PIN);
}

void Lcd_fillScreen(uint16_t color)
{
    Lcd_fillRect(0U, 0U, LCD_WIDTH, LCD_HEIGHT, color);
}

void Lcd_fillRect(
    uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint32_t pixelCount;

    if ((width == 0U) || (height == 0U) ||
        (x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) {
        return;
    }

    if (((uint32_t) x + width) > LCD_WIDTH) {
        width = (uint16_t) (LCD_WIDTH - x);
    }
    if (((uint32_t) y + height) > LCD_HEIGHT) {
        height = (uint16_t) (LCD_HEIGHT - y);
    }

    setAddressWindow(x, y, (uint16_t) (x + width - 1U),
        (uint16_t) (y + height - 1U));
    beginPixelStream();

    pixelCount = (uint32_t) width * height;
    while (pixelCount > 0U) {
        writePixelColor(color);
        pixelCount--;
    }

    endPixelStream();
}

void Lcd_drawText(uint16_t x, uint16_t y, const char *text,
    uint8_t scale, uint16_t foreground, uint16_t background)
{
    uint16_t cursorX = x;
    uint16_t advance;

    if ((text == NULL) || (scale == 0U)) {
        return;
    }

    advance = (uint16_t) (6U * scale);
    while ((*text != '\0') &&
           ((uint32_t) cursorX + advance <= LCD_WIDTH)) {
        drawCharacter(
            cursorX, y, *text, scale, foreground, background);
        cursorX = (uint16_t) (cursorX + advance);
        text++;
    }
}

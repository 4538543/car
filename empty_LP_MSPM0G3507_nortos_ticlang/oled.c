#include "oled.h"

#include <stdint.h>

#include "app_config.h"
#include "arc_interface.h"
#include "gyro.h"
#include "ti_msp_dl_config.h"

static bool gPresent;
static uint16_t gRefreshMs;
static char gDisplayText[16];
static uint8_t gDisplayIndex;
static bool gDisplayUpdating;

static void delayI2C(void)
{
    delay_cycles((CPUCLK_FREQ / 1000000U) * 3U);
}

/* Open-drain software I2C: output latch always stays low; disabling output
 * releases the line to the OLED module pull-up. */
static void sclLow(void)
{
    DL_GPIO_clearPins(OLED_SCL_PORT, OLED_SCL_PIN);
    DL_GPIO_enableOutput(OLED_SCL_PORT, OLED_SCL_PIN);
}

static void sclRelease(void)
{
    DL_GPIO_disableOutput(OLED_SCL_PORT, OLED_SCL_PIN);
}

static void sdaLow(void)
{
    DL_GPIO_clearPins(OLED_SDA_PORT, OLED_SDA_PIN);
    DL_GPIO_enableOutput(OLED_SDA_PORT, OLED_SDA_PIN);
}

static void sdaRelease(void)
{
    DL_GPIO_disableOutput(OLED_SDA_PORT, OLED_SDA_PIN);
}

static void start(void)
{
    sdaRelease(); sclRelease(); delayI2C();
    sdaLow(); delayI2C();
    sclLow();
}

static void stop(void)
{
    sdaLow(); delayI2C();
    sclRelease(); delayI2C();
    sdaRelease(); delayI2C();
}

static bool writeByte(uint8_t value)
{
    uint8_t i;
    bool ack;

    for (i = 0U; i < 8U; i++) {
        if ((value & 0x80U) != 0U) sdaRelease(); else sdaLow();
        delayI2C();
        sclRelease(); delayI2C();
        sclLow();
        value <<= 1U;
    }
    sdaRelease(); delayI2C();
    sclRelease(); delayI2C();
    ack = (DL_GPIO_readPins(OLED_SDA_PORT, OLED_SDA_PIN) & OLED_SDA_PIN) == 0U;
    sclLow();
    return ack;
}

static bool beginPacket(uint8_t control)
{
    start();
    if (!writeByte((uint8_t) (OLED_I2C_ADDRESS << 1U))) {
        stop();
        return false;
    }
    if (!writeByte(control)) {
        stop();
        return false;
    }
    return true;
}

static void command(uint8_t value)
{
    if (beginPacket(0x00U)) {
        (void) writeByte(value);
        stop();
    }
}

static const uint8_t *glyph(char c)
{
    static const uint8_t digits[10][5] = {
        {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
    };
    static const uint8_t y[5] = {0x03,0x04,0x78,0x04,0x03};
    static const uint8_t a[5] = {0x7E,0x11,0x11,0x11,0x7E};
    static const uint8_t w[5] = {0x3F,0x40,0x38,0x40,0x3F};
    static const uint8_t s[5] = {0x46,0x49,0x49,0x49,0x31};
    static const uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t minus[5] = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
    static const uint8_t space[5] = {0,0,0,0,0};

    if ((c >= '0') && (c <= '9')) return digits[c - '0'];
    if (c == 'Y') return y;
    if (c == 'A') return a;
    if (c == 'W') return w;
    if (c == 'S') return s;
    if (c == ':') return colon;
    if (c == '-') return minus;
    if (c == '.') return dot;
    return space;
}

static void formatDisplay(char text[16])
{
    float yaw = Gyro_hasYaw() ? (GYRO_ABSOLUTE_YAW_SIGN *
        Gyro_getCourseYawDeg()) : 0.0f;
    int32_t tenths = (int32_t) (yaw * 10.0f);
    uint32_t value;
    uint8_t index = 0U;

    text[index++]='Y'; text[index++]='A'; text[index++]='W'; text[index++]=':';
    text[index++]=' ';
    if (tenths < 0) { text[index++]='-'; value=(uint32_t)(-tenths); }
    else { text[index++]=' '; value=(uint32_t)tenths; }
    text[index++]=(char)('0'+((value/1000U)%10U));
    text[index++]=(char)('0'+((value/100U)%10U));
    text[index++]=(char)('0'+((value/10U)%10U));
    text[index++]='.';
    text[index++]=(char)('0'+(value%10U));
    text[index++]=' ';
    text[index++]='S';
    text[index++]=(char)('0'+(gArcStatusDebug%10U));
    text[index]='\0';
}

static void drawOneCharacter(uint8_t index)
{
    uint8_t pixelColumn = (uint8_t)(index * 6U);
    uint8_t column;
    const uint8_t *data = glyph(gDisplayText[index]);

    /* Set page and column in one command transaction. */
    if (!beginPacket(0x00U)) { gPresent=false; return; }
    (void)writeByte(0xB0U);
    (void)writeByte((uint8_t)(pixelColumn & 0x0FU));
    (void)writeByte((uint8_t)(0x10U | (pixelColumn >> 4U)));
    stop();
    if (!beginPacket(0x40U)) { gPresent=false; return; }
    for (column=0U; column<5U; column++) (void)writeByte(data[column]);
    (void)writeByte(0U);
    stop();
}

void OLED_init(void)
{
    static const uint8_t initSequence[] = {
        0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,
        0x20,0x02,0xA1,0xC8,0xDA,0x12,0x81,0x7F,0xD9,0xF1,
        0xDB,0x40,0xA4,0xA6,0xAF
    };
    uint8_t i;

    DL_GPIO_clearPins(OLED_SCL_PORT, OLED_SCL_PIN);
    DL_GPIO_clearPins(OLED_SDA_PORT, OLED_SDA_PIN);
    sclRelease(); sdaRelease();
    delay_cycles(CPUCLK_FREQ / 20U);
    gPresent = true;
    for (i=0U; i<sizeof(initSequence); i++) command(initSequence[i]);
    gRefreshMs = OLED_REFRESH_MS;
    gDisplayIndex = 0U;
    gDisplayUpdating = false;
    formatDisplay(gDisplayText);
}

void OLED_task1ms(void)
{
    if (gDisplayUpdating) {
        drawOneCharacter(gDisplayIndex++);
        if ((gDisplayIndex >= 15U) ||
            (gDisplayText[gDisplayIndex] == '\0')) {
            gDisplayUpdating = false;
        }
    } else if (++gRefreshMs >= OLED_REFRESH_MS) {
        gRefreshMs = 0U;
        formatDisplay(gDisplayText);
        gDisplayIndex = 0U;
        gDisplayUpdating = true;
    }
}

bool OLED_isPresent(void)
{
    return gPresent;
}

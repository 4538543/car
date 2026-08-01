#include "oled_time.h"

#include <stdbool.h>
#include <stdint.h>

#include "ab_mission.h"
#include "ti_msp_dl_config.h"

#define OLED_ADDRESS                       (0x3CU)
#define OLED_TRANSFER_TIMEOUT              (50000U)
#define OLED_REFRESH_MS                      (100U)
#define OLED_WIDTH                           (128U)

static bool gOledPresent;
static uint16_t gRefreshDivider;
static ABMissionMode gDisplayedMode;
static uint32_t gDisplayedTimeMs;

static bool writePacket(const uint8_t *data, uint8_t length)
{
    uint32_t timeout = OLED_TRANSFER_TIMEOUT;

    while (((DL_I2C_getControllerStatus(OLED_I2C_INST) &
             DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) &&
           (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        return false;
    }

    DL_I2C_resetControllerTransfer(OLED_I2C_INST);
    DL_I2C_fillControllerTXFIFO(OLED_I2C_INST, data, length);
    DL_I2C_startControllerTransfer(OLED_I2C_INST, OLED_ADDRESS,
        DL_I2C_CONTROLLER_DIRECTION_TX, length);
    delay_cycles(100U);

    timeout = OLED_TRANSFER_TIMEOUT;
    while (((DL_I2C_getControllerStatus(OLED_I2C_INST) &
             DL_I2C_CONTROLLER_STATUS_BUSY) != 0U) &&
           (timeout > 0U)) {
        timeout--;
    }
    return (timeout > 0U) &&
        ((DL_I2C_getControllerStatus(OLED_I2C_INST) &
          DL_I2C_CONTROLLER_STATUS_ERROR) == 0U);
}

static bool writeCommands(const uint8_t *commands, uint8_t count)
{
    uint8_t packet[8];
    uint8_t offset = 0U;

    packet[0] = 0x00U;
    while (offset < count) {
        uint8_t chunk = (uint8_t)(count - offset);
        uint8_t i;
        if (chunk > 7U) {
            chunk = 7U;
        }
        for (i = 0U; i < chunk; i++) {
            packet[i + 1U] = commands[offset + i];
        }
        if (!writePacket(packet, (uint8_t)(chunk + 1U))) {
            return false;
        }
        offset = (uint8_t)(offset + chunk);
    }
    return true;
}

static bool setWindow(uint8_t column, uint8_t count, uint8_t page)
{
    uint8_t commands[6] = {
        0x21U, column, (uint8_t)(column + count - 1U),
        0x22U, page, page
    };
    return writeCommands(commands, sizeof(commands));
}

static bool writeData(const uint8_t *data, uint8_t count)
{
    uint8_t packet[8];
    uint8_t offset = 0U;

    packet[0] = 0x40U;
    while (offset < count) {
        uint8_t chunk = (uint8_t)(count - offset);
        uint8_t i;
        if (chunk > 7U) {
            chunk = 7U;
        }
        for (i = 0U; i < chunk; i++) {
            packet[i + 1U] = data[offset + i];
        }
        if (!writePacket(packet, (uint8_t)(chunk + 1U))) {
            return false;
        }
        offset = (uint8_t)(offset + chunk);
    }
    return true;
}

static void glyph(char c, uint8_t out[5])
{
    uint8_t i;
    for (i = 0U; i < 5U; i++) {
        out[i] = 0U;
    }

    switch (c) {
    case '0': { const uint8_t v[5]={0x3EU,0x51U,0x49U,0x45U,0x3EU}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '1': { const uint8_t v[5]={0x00U,0x42U,0x7FU,0x40U,0x00U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '2': { const uint8_t v[5]={0x42U,0x61U,0x51U,0x49U,0x46U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '3': { const uint8_t v[5]={0x21U,0x41U,0x45U,0x4BU,0x31U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '4': { const uint8_t v[5]={0x18U,0x14U,0x12U,0x7FU,0x10U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '5': { const uint8_t v[5]={0x27U,0x45U,0x45U,0x45U,0x39U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '6': { const uint8_t v[5]={0x3CU,0x4AU,0x49U,0x49U,0x30U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '7': { const uint8_t v[5]={0x01U,0x71U,0x09U,0x05U,0x03U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '8': { const uint8_t v[5]={0x36U,0x49U,0x49U,0x49U,0x36U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '9': { const uint8_t v[5]={0x06U,0x49U,0x49U,0x29U,0x1EU}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'A': { const uint8_t v[5]={0x7EU,0x11U,0x11U,0x11U,0x7EU}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'B': { const uint8_t v[5]={0x7FU,0x49U,0x49U,0x49U,0x36U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'D': { const uint8_t v[5]={0x7FU,0x41U,0x41U,0x22U,0x1CU}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'E': { const uint8_t v[5]={0x7FU,0x49U,0x49U,0x49U,0x41U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'I': { const uint8_t v[5]={0x00U,0x41U,0x7FU,0x41U,0x00U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'M': { const uint8_t v[5]={0x7FU,0x02U,0x0CU,0x02U,0x7FU}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'P': { const uint8_t v[5]={0x7FU,0x09U,0x09U,0x09U,0x06U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'S': { const uint8_t v[5]={0x46U,0x49U,0x49U,0x49U,0x31U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'T': { const uint8_t v[5]={0x01U,0x01U,0x7FU,0x01U,0x01U}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case ':': out[1]=0x36U; out[2]=0x36U; break;
    case '.': out[1]=0x60U; out[2]=0x60U; break;
    default: break;
    }
}

static bool drawText(uint8_t column, uint8_t page, const char *text)
{
    uint8_t buffer[96];
    uint8_t length = 0U;

    while ((*text != '\0') && (length <= 90U)) {
        uint8_t pixels[5];
        uint8_t i;
        glyph(*text++, pixels);
        for (i = 0U; i < 5U; i++) {
            buffer[length++] = pixels[i];
        }
        buffer[length++] = 0U;
    }
    return setWindow(column, length, page) && writeData(buffer, length);
}

static void formatTime(char text[12], uint32_t elapsedMs)
{
    uint32_t seconds;
    uint32_t milliseconds;

    if (elapsedMs > 999999U) {
        elapsedMs = 999999U;
    }
    seconds = elapsedMs / 1000U;
    milliseconds = elapsedMs % 1000U;
    text[0] = 'T'; text[1] = ' ';
    text[2] = (char)('0' + ((seconds / 100U) % 10U));
    text[3] = (char)('0' + ((seconds / 10U) % 10U));
    text[4] = (char)('0' + (seconds % 10U));
    text[5] = '.';
    text[6] = (char)('0' + ((milliseconds / 100U) % 10U));
    text[7] = (char)('0' + ((milliseconds / 10U) % 10U));
    text[8] = (char)('0' + (milliseconds % 10U));
    text[9] = 'S'; text[10] = ' '; text[11] = '\0';
}

static const char *modeLabel(ABMissionMode mode)
{
    switch (mode) {
    case AB_MISSION_MODE_LAP_IMMEDIATE: return "PA18 STOP TIME";
    case AB_MISSION_MODE_AB_PASS:       return "PB22 B TIME";
    case AB_MISSION_MODE_LAP_PASS:      return "PB23 D TIME";
    default:                            return "SELECT MODE";
    }
}

void OledTime_init(void)
{
    static const uint8_t initCommands[] = {
        0xAEU, 0xD5U, 0x80U, 0xA8U, 0x3FU, 0xD3U, 0x00U,
        0x40U, 0x8DU, 0x14U, 0x20U, 0x00U, 0xA1U, 0xC8U,
        0xDAU, 0x12U, 0x81U, 0x7FU, 0xD9U, 0xF1U, 0xDBU,
        0x40U, 0xA4U, 0xA6U, 0xAFU
    };
    uint8_t page;
    uint8_t zeros[OLED_WIDTH] = {0U};

    gOledPresent = writeCommands(initCommands, sizeof(initCommands));
    if (gOledPresent) {
        for (page = 0U; page < 8U; page++) {
            if (!setWindow(0U, OLED_WIDTH, page) ||
                !writeData(zeros, OLED_WIDTH)) {
                gOledPresent = false;
                break;
            }
        }
    }
    gRefreshDivider = 0U;
    gDisplayedMode = AB_MISSION_MODE_NONE;
    gDisplayedTimeMs = UINT32_MAX;
    if (gOledPresent) {
        drawText(0U, 0U, modeLabel(AB_MISSION_MODE_NONE));
    }
}

void OledTime_task1ms(void)
{
    ABMissionMode mode;
    uint32_t elapsedMs;
    char timeText[12];

    if (!gOledPresent) {
        return;
    }
    if (++gRefreshDivider < OLED_REFRESH_MS) {
        return;
    }
    gRefreshDivider = 0U;

    mode = ABMission_getMode();
    elapsedMs = ABMission_getElapsedMs();
    if (mode != gDisplayedMode) {
        uint8_t blank[OLED_WIDTH] = {0U};
        setWindow(0U, OLED_WIDTH, 0U);
        writeData(blank, OLED_WIDTH);
        drawText(0U, 0U, modeLabel(mode));
        gDisplayedMode = mode;
    }
    if (elapsedMs != gDisplayedTimeMs) {
        formatTime(timeText, elapsedMs);
        if (!drawText(0U, 2U, timeText)) {
            gOledPresent = false;
        }
        gDisplayedTimeMs = elapsedMs;
    }
}

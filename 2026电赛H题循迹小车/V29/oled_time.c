#include "oled_time.h"

#include <stdbool.h>
#include <stdint.h>

#include "ab_mission.h"
#include "ti_msp_dl_config.h"

#define OLED_ADDRESS                         (0x3CU)
#define OLED_REFRESH_MS                        (100U)
#define OLED_WIDTH                             (128U)
#define OLED_LABEL_BYTES                        (96U)
#define OLED_TIME_BYTES                         (72U)

typedef enum {
    OLED_PHASE_INIT = 0,
    OLED_PHASE_CLEAR_WINDOW,
    OLED_PHASE_CLEAR_DATA,
    OLED_PHASE_RUN,
    OLED_PHASE_DRAW_WINDOW,
    OLED_PHASE_DRAW_DATA,
    OLED_PHASE_FAILED
} OledPhase;

static const uint8_t gInitCommands[] = {
    0xAEU, 0xD5U, 0x80U, 0xA8U, 0x3FU, 0xD3U, 0x00U,
    0x40U, 0x8DU, 0x14U, 0x20U, 0x00U, 0xA1U, 0xC8U,
    0xDAU, 0x12U, 0x81U, 0x7FU, 0xD9U, 0xF1U, 0xDBU,
    0x40U, 0xA4U, 0xA6U, 0xAFU
};

static OledPhase gPhase;
static bool gTransferActive;
static uint8_t gPacket[8];
static uint8_t gInitOffset;
static uint8_t gClearPage;
static uint8_t gClearOffset;
static uint16_t gRefreshDivider;
static ABMissionMode gDisplayedMode;
static uint32_t gDisplayedTimeMs;
static uint8_t gLabelPixels[OLED_LABEL_BYTES];
static uint8_t gTimePixels[OLED_TIME_BYTES];
static bool gLabelDirty;
static bool gTimeDirty;
static const uint8_t *gDrawPixels;
static uint8_t gDrawLength;
static uint8_t gDrawOffset;
static uint8_t gDrawPage;

static void serviceTransfer(void)
{
    uint32_t status;

    if (!gTransferActive) {
        return;
    }
    status = DL_I2C_getControllerStatus(OLED_I2C_INST);
    if ((status & DL_I2C_CONTROLLER_STATUS_BUSY) != 0U) {
        return;
    }
    gTransferActive = false;
    if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
        DL_I2C_resetControllerTransfer(OLED_I2C_INST);
        gPhase = OLED_PHASE_FAILED;
    }
}

static bool queuePacket(uint8_t control, const uint8_t *data, uint8_t count)
{
    uint32_t status;
    uint8_t i;

    if (gTransferActive || (count > 7U) ||
        (gPhase == OLED_PHASE_FAILED)) {
        return false;
    }
    status = DL_I2C_getControllerStatus(OLED_I2C_INST);
    if ((status & DL_I2C_CONTROLLER_STATUS_BUSY) != 0U) {
        return false;
    }

    DL_I2C_resetControllerTransfer(OLED_I2C_INST);
    gPacket[0] = control;
    for (i = 0U; i < count; i++) {
        gPacket[i + 1U] = data[i];
    }
    DL_I2C_fillControllerTXFIFO(
        OLED_I2C_INST, gPacket, (uint8_t)(count + 1U));
    DL_I2C_startControllerTransfer(OLED_I2C_INST, OLED_ADDRESS,
        DL_I2C_CONTROLLER_DIRECTION_TX, (uint8_t)(count + 1U));
    gTransferActive = true;
    return true;
}

static bool queueWindow(uint8_t page, uint8_t length)
{
    uint8_t commands[6] = {
        0x21U, 0x00U, (uint8_t)(length - 1U),
        0x22U, page, page
    };
    return queuePacket(0x00U, commands, sizeof(commands));
}

static void glyph(char c, uint8_t out[5])
{
    uint8_t i;
    for (i = 0U; i < 5U; i++) out[i] = 0U;
    switch (c) {
    case '0': { const uint8_t v[5]={0x3E,0x51,0x49,0x45,0x3E}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '1': { const uint8_t v[5]={0x00,0x42,0x7F,0x40,0x00}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '2': { const uint8_t v[5]={0x42,0x61,0x51,0x49,0x46}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '3': { const uint8_t v[5]={0x21,0x41,0x45,0x4B,0x31}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '4': { const uint8_t v[5]={0x18,0x14,0x12,0x7F,0x10}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '5': { const uint8_t v[5]={0x27,0x45,0x45,0x45,0x39}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '6': { const uint8_t v[5]={0x3C,0x4A,0x49,0x49,0x30}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '7': { const uint8_t v[5]={0x01,0x71,0x09,0x05,0x03}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '8': { const uint8_t v[5]={0x36,0x49,0x49,0x49,0x36}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case '9': { const uint8_t v[5]={0x06,0x49,0x49,0x29,0x1E}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'A': { const uint8_t v[5]={0x7E,0x11,0x11,0x11,0x7E}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'B': { const uint8_t v[5]={0x7F,0x49,0x49,0x49,0x36}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'D': { const uint8_t v[5]={0x7F,0x41,0x41,0x22,0x1C}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'E': { const uint8_t v[5]={0x7F,0x49,0x49,0x49,0x41}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'I': { const uint8_t v[5]={0x00,0x41,0x7F,0x41,0x00}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'M': { const uint8_t v[5]={0x7F,0x02,0x0C,0x02,0x7F}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'P': { const uint8_t v[5]={0x7F,0x09,0x09,0x09,0x06}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'S': { const uint8_t v[5]={0x46,0x49,0x49,0x49,0x31}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case 'T': { const uint8_t v[5]={0x01,0x01,0x7F,0x01,0x01}; for(i=0;i<5;i++)out[i]=v[i]; } break;
    case ':': out[1]=0x36U; out[2]=0x36U; break;
    case '.': out[1]=0x60U; out[2]=0x60U; break;
    default: break;
    }
}

static void renderText(uint8_t *buffer, uint8_t length, const char *text)
{
    uint8_t offset = 0U;
    uint8_t i;
    for (i = 0U; i < length; i++) buffer[i] = 0U;
    while ((*text != '\0') && ((uint16_t)offset + 6U <= length)) {
        uint8_t pixels[5];
        glyph(*text++, pixels);
        for (i = 0U; i < 5U; i++) buffer[offset++] = pixels[i];
        buffer[offset++] = 0U;
    }
}

static void formatTime(char text[12], uint32_t elapsedMs)
{
    uint32_t seconds;
    uint32_t milliseconds;
    if (elapsedMs > 999999U) elapsedMs = 999999U;
    seconds = elapsedMs / 1000U;
    milliseconds = elapsedMs % 1000U;
    text[0]='T'; text[1]=' ';
    text[2]=(char)('0'+((seconds/100U)%10U));
    text[3]=(char)('0'+((seconds/10U)%10U));
    text[4]=(char)('0'+(seconds%10U)); text[5]='.';
    text[6]=(char)('0'+((milliseconds/100U)%10U));
    text[7]=(char)('0'+((milliseconds/10U)%10U));
    text[8]=(char)('0'+(milliseconds%10U));
    text[9]='S'; text[10]=' '; text[11]='\0';
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

static void beginDraw(const uint8_t *pixels, uint8_t length, uint8_t page)
{
    gDrawPixels = pixels;
    gDrawLength = length;
    gDrawOffset = 0U;
    gDrawPage = page;
    gPhase = OLED_PHASE_DRAW_WINDOW;
}

void OledTime_init(void)
{
    gPhase = OLED_PHASE_INIT;
    gTransferActive = false;
    gInitOffset = 0U;
    gClearPage = 0U;
    gClearOffset = 0U;
    gRefreshDivider = 0U;
    gDisplayedMode = AB_MISSION_MODE_NONE;
    gDisplayedTimeMs = UINT32_MAX;
    gLabelDirty = true;
    gTimeDirty = true;
    renderText(gLabelPixels, OLED_LABEL_BYTES,
        modeLabel(AB_MISSION_MODE_NONE));
    renderText(gTimePixels, OLED_TIME_BYTES, "T 000.000S");
}

void OledTime_task1ms(void)
{
    serviceTransfer();
    if (gTransferActive || (gPhase == OLED_PHASE_FAILED)) return;

    if (++gRefreshDivider >= OLED_REFRESH_MS) {
        ABMissionMode mode = ABMission_getMode();
        uint32_t elapsedMs = ABMission_getElapsedMs();
        char timeText[12];
        gRefreshDivider = 0U;
        if (mode != gDisplayedMode) {
            renderText(gLabelPixels, OLED_LABEL_BYTES, modeLabel(mode));
            gDisplayedMode = mode;
            gLabelDirty = true;
        }
        if (elapsedMs != gDisplayedTimeMs) {
            formatTime(timeText, elapsedMs);
            renderText(gTimePixels, OLED_TIME_BYTES, timeText);
            gDisplayedTimeMs = elapsedMs;
            gTimeDirty = true;
        }
    }

    switch (gPhase) {
    case OLED_PHASE_INIT: {
        uint8_t remaining =
            (uint8_t)(sizeof(gInitCommands) - gInitOffset);
        uint8_t chunk = (remaining > 7U) ? 7U : remaining;
        if (queuePacket(0x00U, &gInitCommands[gInitOffset], chunk)) {
            gInitOffset = (uint8_t)(gInitOffset + chunk);
            if (gInitOffset >= sizeof(gInitCommands)) {
                gPhase = OLED_PHASE_CLEAR_WINDOW;
            }
        }
        break;
    }
    case OLED_PHASE_CLEAR_WINDOW:
        if (queueWindow(gClearPage, OLED_WIDTH)) {
            gClearOffset = 0U;
            gPhase = OLED_PHASE_CLEAR_DATA;
        }
        break;
    case OLED_PHASE_CLEAR_DATA: {
        static const uint8_t zeros[7] = {0U};
        uint8_t remaining = (uint8_t)(OLED_WIDTH - gClearOffset);
        uint8_t chunk = (remaining > 7U) ? 7U : remaining;
        if (queuePacket(0x40U, zeros, chunk)) {
            gClearOffset = (uint8_t)(gClearOffset + chunk);
            if (gClearOffset >= OLED_WIDTH) {
                gClearPage++;
                gPhase = (gClearPage >= 8U) ?
                    OLED_PHASE_RUN : OLED_PHASE_CLEAR_WINDOW;
            }
        }
        break;
    }
    case OLED_PHASE_RUN:
        if (gLabelDirty) {
            gLabelDirty = false;
            beginDraw(gLabelPixels, OLED_LABEL_BYTES, 0U);
        } else if (gTimeDirty) {
            gTimeDirty = false;
            beginDraw(gTimePixels, OLED_TIME_BYTES, 2U);
        }
        break;
    case OLED_PHASE_DRAW_WINDOW:
        if (queueWindow(gDrawPage, gDrawLength)) {
            gPhase = OLED_PHASE_DRAW_DATA;
        }
        break;
    case OLED_PHASE_DRAW_DATA: {
        uint8_t remaining = (uint8_t)(gDrawLength - gDrawOffset);
        uint8_t chunk = (remaining > 7U) ? 7U : remaining;
        if (queuePacket(0x40U, &gDrawPixels[gDrawOffset], chunk)) {
            gDrawOffset = (uint8_t)(gDrawOffset + chunk);
            if (gDrawOffset >= gDrawLength) gPhase = OLED_PHASE_RUN;
        }
        break;
    }
    case OLED_PHASE_FAILED:
    default:
        break;
    }
}

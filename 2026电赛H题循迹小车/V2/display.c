#include "display.h"

#include <stdbool.h>
#include <stdint.h>

#include "ab_mission.h"
#include "gyro.h"
#include "lcd.h"

#define DISPLAY_REFRESH_MS                (100U)
#define DISPLAY_CHARACTER_SERVICE_MS       (10U)
#define DISPLAY_TEXT_SCALE                (2U)
#define DISPLAY_CHARACTER_ADVANCE         (12U)

#define WZ_FIELD_LENGTH                   (12U)
#define TIME_FIELD_LENGTH                  (9U)
#define STATE_FIELD_LENGTH                 (8U)
#define DIST_FIELD_LENGTH                  (7U)

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t length;
    char *current;
    char *desired;
    uint16_t foreground;
} DisplayField;

static char gWzCurrent[WZ_FIELD_LENGTH + 1U];
static char gWzDesired[WZ_FIELD_LENGTH + 1U];
static char gTimeCurrent[TIME_FIELD_LENGTH + 1U];
static char gTimeDesired[TIME_FIELD_LENGTH + 1U];
static char gStateCurrent[STATE_FIELD_LENGTH + 1U];
static char gStateDesired[STATE_FIELD_LENGTH + 1U];
static char gDistanceCurrent[DIST_FIELD_LENGTH + 1U];
static char gDistanceDesired[DIST_FIELD_LENGTH + 1U];

static DisplayField gFields[4];
static uint8_t gNextField;
static uint8_t gRefreshDivider;
static uint8_t gCharacterServiceDivider;

static void copyFixed(char *destination, const char *source, uint8_t length)
{
    uint8_t index;

    for (index = 0U; index < length; index++) {
        destination[index] = source[index];
    }
    destination[length] = '\0';
}

static void writeUnsignedFixed(
    char *destination, uint32_t value, uint8_t digits)
{
    uint8_t index = digits;

    while (index > 0U) {
        index--;
        destination[index] = (char) ('0' + (value % 10U));
        value /= 10U;
    }
}

static void formatAngularRate(void)
{
    int32_t angularRate;
    uint32_t magnitude;

    if (!Gyro_hasFreshAngularRate()) {
        copyFixed(gWzDesired, "NO DATA     ", WZ_FIELD_LENGTH);
        return;
    }

    angularRate = Gyro_getAngularRateCentiDps();
    if (angularRate < 0) {
        gWzDesired[0] = '-';
        magnitude = (uint32_t) (-(int64_t) angularRate);
    } else {
        gWzDesired[0] = '+';
        magnitude = (uint32_t) angularRate;
    }

    if (magnitude > 999999U) {
        magnitude = 999999U;
    }

    writeUnsignedFixed(&gWzDesired[1], magnitude / 100U, 4U);
    gWzDesired[5] = '.';
    writeUnsignedFixed(&gWzDesired[6], magnitude % 100U, 2U);
    gWzDesired[8] = ' ';
    gWzDesired[9] = 'D';
    gWzDesired[10] = 'P';
    gWzDesired[11] = 'S';
    gWzDesired[12] = '\0';
}

static void formatTime(void)
{
    uint32_t elapsedMs = ABMission_getElapsedMs();
    uint32_t seconds = elapsedMs / 1000U;

    if (seconds > 999U) {
        seconds = 999U;
        elapsedMs = 999999U;
    }

    writeUnsignedFixed(&gTimeDesired[0], seconds, 3U);
    gTimeDesired[3] = '.';
    writeUnsignedFixed(&gTimeDesired[4], elapsedMs % 1000U, 3U);
    gTimeDesired[7] = ' ';
    gTimeDesired[8] = 'S';
    gTimeDesired[9] = '\0';
}

static void formatState(void)
{
    const char *text;

    switch (ABMission_getState()) {
        case AB_MISSION_CRUISE:
            text = (ABMission_getMode() == AB_MISSION_MODE_LAP) ?
                "LAP RUN " : "CRUISE  ";
            break;
        case AB_MISSION_APPROACH:
            text = (ABMission_getMode() == AB_MISSION_MODE_LAP) ?
                "LAP SLOW" : "SLOW    ";
            break;
        case AB_MISSION_FINISH_COVER:
            text = "LINE HIT";
            break;
        case AB_MISSION_DECEL:
            text = "DECEL   ";
            break;
        case AB_MISSION_BRAKE_WAIT:
            text = "COAST   ";
            break;
        case AB_MISSION_REVERSE_BRAKE:
            text = "BRAKE   ";
            break;
        case AB_MISSION_DONE:
            text = "DONE    ";
            break;
        case AB_MISSION_ABORTED:
            text = "ABORT   ";
            break;
        case AB_MISSION_FAULT:
            text = "FAULT   ";
            break;
        case AB_MISSION_IDLE:
        default:
            text = "IDLE    ";
            break;
    }

    copyFixed(gStateDesired, text, STATE_FIELD_LENGTH);
}

static void formatDistance(void)
{
    uint32_t distanceMm = ABMission_getDistanceMm();

    if (distanceMm > 9999U) {
        distanceMm = 9999U;
    }

    writeUnsignedFixed(&gDistanceDesired[0], distanceMm, 4U);
    gDistanceDesired[4] = ' ';
    gDistanceDesired[5] = 'M';
    gDistanceDesired[6] = 'M';
    gDistanceDesired[7] = '\0';
}

static void refreshDesiredValues(void)
{
    formatAngularRate();
    formatTime();
    formatState();
    formatDistance();
}

static bool updateOneCharacter(DisplayField *field)
{
    uint8_t index;
    char text[2];

    for (index = 0U; index < field->length; index++) {
        if (field->current[index] != field->desired[index]) {
            text[0] = field->desired[index];
            text[1] = '\0';
            Lcd_drawText(
                (uint16_t) (field->x +
                    ((uint16_t) index * DISPLAY_CHARACTER_ADVANCE)),
                field->y, text, DISPLAY_TEXT_SCALE,
                field->foreground, LCD_COLOR_BLACK);
            field->current[index] = field->desired[index];
            return true;
        }
    }

    return false;
}

void Display_init(void)
{
    Lcd_init();

    Lcd_fillRect(0U, 0U, LCD_WIDTH, 38U, LCD_COLOR_DARK_BLUE);
    Lcd_drawText(16U, 10U, "H CAR STRAIGHT", 2U,
        LCD_COLOR_WHITE, LCD_COLOR_DARK_BLUE);

    Lcd_drawText(10U, 58U, "GYRO WZ:", 2U,
        LCD_COLOR_CYAN, LCD_COLOR_BLACK);
    Lcd_drawText(10U, 108U, "TIME:", 2U,
        LCD_COLOR_CYAN, LCD_COLOR_BLACK);
    Lcd_drawText(10U, 158U, "STATE:", 2U,
        LCD_COLOR_CYAN, LCD_COLOR_BLACK);
    Lcd_drawText(10U, 208U, "DIST:", 2U,
        LCD_COLOR_CYAN, LCD_COLOR_BLACK);

    copyFixed(gWzCurrent, "NO DATA     ", WZ_FIELD_LENGTH);
    copyFixed(gWzDesired, "NO DATA     ", WZ_FIELD_LENGTH);
    copyFixed(gTimeCurrent, "000.000 S", TIME_FIELD_LENGTH);
    copyFixed(gTimeDesired, "000.000 S", TIME_FIELD_LENGTH);
    copyFixed(gStateCurrent, "IDLE    ", STATE_FIELD_LENGTH);
    copyFixed(gStateDesired, "IDLE    ", STATE_FIELD_LENGTH);
    copyFixed(gDistanceCurrent, "0000 MM", DIST_FIELD_LENGTH);
    copyFixed(gDistanceDesired, "0000 MM", DIST_FIELD_LENGTH);

    Lcd_drawText(110U, 58U, gWzCurrent, DISPLAY_TEXT_SCALE,
        LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
    Lcd_drawText(110U, 108U, gTimeCurrent, DISPLAY_TEXT_SCALE,
        LCD_COLOR_WHITE, LCD_COLOR_BLACK);
    Lcd_drawText(110U, 158U, gStateCurrent, DISPLAY_TEXT_SCALE,
        LCD_COLOR_GREEN, LCD_COLOR_BLACK);
    Lcd_drawText(110U, 208U, gDistanceCurrent, DISPLAY_TEXT_SCALE,
        LCD_COLOR_WHITE, LCD_COLOR_BLACK);

    gFields[0].x = 110U;
    gFields[0].y = 58U;
    gFields[0].length = WZ_FIELD_LENGTH;
    gFields[0].current = gWzCurrent;
    gFields[0].desired = gWzDesired;
    gFields[0].foreground = LCD_COLOR_YELLOW;

    gFields[1].x = 110U;
    gFields[1].y = 108U;
    gFields[1].length = TIME_FIELD_LENGTH;
    gFields[1].current = gTimeCurrent;
    gFields[1].desired = gTimeDesired;
    gFields[1].foreground = LCD_COLOR_WHITE;

    gFields[2].x = 110U;
    gFields[2].y = 158U;
    gFields[2].length = STATE_FIELD_LENGTH;
    gFields[2].current = gStateCurrent;
    gFields[2].desired = gStateDesired;
    gFields[2].foreground = LCD_COLOR_GREEN;

    gFields[3].x = 110U;
    gFields[3].y = 208U;
    gFields[3].length = DIST_FIELD_LENGTH;
    gFields[3].current = gDistanceCurrent;
    gFields[3].desired = gDistanceDesired;
    gFields[3].foreground = LCD_COLOR_WHITE;

    gNextField = 0U;
    gRefreshDivider = 0U;
    gCharacterServiceDivider = 0U;
}

void Display_task1ms(void)
{
    uint8_t attempts;

    gRefreshDivider++;
    if (gRefreshDivider >= DISPLAY_REFRESH_MS) {
        gRefreshDivider = 0U;
        refreshDesiredValues();
    }

    gCharacterServiceDivider++;
    if (gCharacterServiceDivider < DISPLAY_CHARACTER_SERVICE_MS) {
        return;
    }
    gCharacterServiceDivider = 0U;

    /*
     * Software SPI is deliberately serviced one changed character at a time.
     * This bounds the foreground blocking time and protects the 10 ms motor
     * control schedule from a full-screen redraw.
     */
    for (attempts = 0U; attempts < 4U; attempts++) {
        DisplayField *field = &gFields[gNextField];
        gNextField = (uint8_t) ((gNextField + 1U) % 4U);
        if (updateOneCharacter(field)) {
            break;
        }
    }
}

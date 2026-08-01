#include "stm32_link.h"

#include <stdbool.h>
#include <stddef.h>

#include "ab_mission.h"
#include "app_config.h"
#include "gyro.h"
#include "straight_control.h"
#include "ti_msp_dl_config.h"

typedef enum {
    SEGMENT_UNKNOWN = 0,
    SEGMENT_STRAIGHT,
    SEGMENT_CURVE
} SegmentState;

static uint8_t gTxQueue[STM32_TX_QUEUE_SIZE];
static uint16_t gTxRead;
static uint16_t gTxWrite;
static uint16_t gTxUsed;
static uint16_t gTelemetryDividerMs;
static int32_t gPreviousAngularRate;
static uint32_t gPreviousSpeed;
static int32_t gAngularAcceleration;
static int32_t gLongitudinalAcceleration;
static uint32_t gDroppedFrameCount;
static SegmentState gSegmentState;
static uint16_t gCurveConfirmMs;
static uint16_t gStraightConfirmMs;

static bool missionIsMoving(void)
{
    ABMissionState state = ABMission_getState();

    return (state == AB_MISSION_CRUISE) ||
           (state == AB_MISSION_APPROACH) ||
           (state == AB_MISSION_PASS_BODY) ||
           (state == AB_MISSION_POST_RUN) ||
           (state == AB_MISSION_DECEL);
}

static uint32_t magnitude32(int32_t value)
{
    return (value >= 0) ?
        (uint32_t) value : (uint32_t) (-(int64_t) value);
}

static uint16_t queueFree(void)
{
    return (uint16_t) (STM32_TX_QUEUE_SIZE - gTxUsed);
}

static bool queueBytes(const char *data, uint16_t length)
{
    uint16_t index;

    if (length > queueFree()) {
        gDroppedFrameCount++;
        return false;
    }

    for (index = 0U; index < length; index++) {
        gTxQueue[gTxWrite] = (uint8_t) data[index];
        gTxWrite++;
        if (gTxWrite >= STM32_TX_QUEUE_SIZE) {
            gTxWrite = 0U;
        }
    }
    gTxUsed = (uint16_t) (gTxUsed + length);
    return true;
}

static uint16_t appendText(
    char *buffer, uint16_t index, const char *text)
{
    while (*text != '\0') {
        buffer[index] = *text;
        index++;
        text++;
    }
    return index;
}

static uint16_t textLength(const char *text)
{
    uint16_t length = 0U;

    while (text[length] != '\0') {
        length++;
    }
    return length;
}

static uint16_t appendSigned(
    char *buffer, uint16_t index, int32_t value)
{
    char reverse[11];
    uint8_t digits = 0U;
    uint32_t magnitude;

    if (value < 0) {
        buffer[index++] = '-';
        magnitude = (uint32_t) (-(int64_t) value);
    } else {
        magnitude = (uint32_t) value;
    }

    do {
        reverse[digits++] =
            (char) ('0' + (magnitude % 10U));
        magnitude /= 10U;
    } while ((magnitude != 0U) &&
             (digits < (uint8_t) sizeof(reverse)));

    while (digits > 0U) {
        digits--;
        buffer[index++] = reverse[digits];
    }
    return index;
}

static void serviceTransmitter(void)
{
    while ((gTxUsed > 0U) &&
           !DL_UART_Main_isTXFIFOFull(STM32_UART_INST)) {
        DL_UART_Main_transmitData(
            STM32_UART_INST, gTxQueue[gTxRead]);
        gTxRead++;
        if (gTxRead >= STM32_TX_QUEUE_SIZE) {
            gTxRead = 0U;
        }
        gTxUsed--;
    }
}

static void queueTelemetry(void)
{
    char frame[96];
    uint16_t length = 0U;
    int32_t angularRate = Gyro_getAngularRateCentiDps();
    uint32_t speed =
        StraightControl_getSpeedMmPerSecond();
    int32_t angularAccelerationRaw =
        (angularRate - gPreviousAngularRate) *
        (int32_t) (1000U / STM32_TELEMETRY_PERIOD_MS);
    int32_t longitudinalAccelerationRaw =
        ((int32_t) speed - (int32_t) gPreviousSpeed) *
        (int32_t) (1000U / STM32_TELEMETRY_PERIOD_MS);

    gPreviousAngularRate = angularRate;
    gPreviousSpeed = speed;
    gAngularAcceleration =
        (gAngularAcceleration * 3 +
         angularAccelerationRaw) / 4;
    gLongitudinalAcceleration =
        (gLongitudinalAcceleration * 3 +
         longitudinalAccelerationRaw) / 4;

    length = appendText(frame, length, "$T,WZ=");
    length = appendSigned(frame, length, angularRate);
    length = appendText(frame, length, ",ALPHA=");
    length = appendSigned(
        frame, length, gAngularAcceleration);
    length = appendText(frame, length, ",ACC=");
    length = appendSigned(
        frame, length, gLongitudinalAcceleration);
    length = appendText(frame, length, ",SPD=");
    length = appendSigned(frame, length, (int32_t) speed);
    length = appendText(frame, length, "\r\n");
    (void) queueBytes(frame, length);
}

static void serviceSegmentDetection(void)
{
    uint32_t angularRateMagnitude;

    if (!missionIsMoving() ||
        !Gyro_hasFreshAngularRate() ||
        (StraightControl_getSpeedMmPerSecond() <
            STM32_SEGMENT_MIN_SPEED_MMPS)) {
        gCurveConfirmMs = 0U;
        gStraightConfirmMs = 0U;
        return;
    }

    angularRateMagnitude =
        magnitude32(Gyro_getAngularRateCentiDps());

    if (angularRateMagnitude >=
        STM32_CURVE_RATE_ENTER_CENTIDPS) {
        gStraightConfirmMs = 0U;
        if (gCurveConfirmMs <
            STM32_CURVE_CONFIRM_MS) {
            gCurveConfirmMs++;
        }
        if ((gCurveConfirmMs >=
                STM32_CURVE_CONFIRM_MS) &&
            (gSegmentState != SEGMENT_CURVE)) {
            Stm32Link_notifyEvent(
                STM32_EVENT_ENTER_CURVE);
        }
    } else if (angularRateMagnitude <=
               STM32_CURVE_RATE_EXIT_CENTIDPS) {
        gCurveConfirmMs = 0U;
        if (gStraightConfirmMs <
            STM32_STRAIGHT_CONFIRM_MS) {
            gStraightConfirmMs++;
        }
        if ((gStraightConfirmMs >=
                STM32_STRAIGHT_CONFIRM_MS) &&
            (gSegmentState != SEGMENT_STRAIGHT)) {
            Stm32Link_notifyEvent(
                STM32_EVENT_ENTER_STRAIGHT);
        }
    } else {
        gCurveConfirmMs = 0U;
        gStraightConfirmMs = 0U;
    }
}

void Stm32Link_init(void)
{
    gTxRead = 0U;
    gTxWrite = 0U;
    gTxUsed = 0U;
    gTelemetryDividerMs = 0U;
    gPreviousAngularRate =
        Gyro_getAngularRateCentiDps();
    gPreviousSpeed =
        StraightControl_getSpeedMmPerSecond();
    gAngularAcceleration = 0;
    gLongitudinalAcceleration = 0;
    gDroppedFrameCount = 0U;
    gSegmentState = SEGMENT_UNKNOWN;
    gCurveConfirmMs = 0U;
    gStraightConfirmMs = 0U;
}

void Stm32Link_task1ms(void)
{
    serviceTransmitter();
    serviceSegmentDetection();

    gTelemetryDividerMs++;
    if (gTelemetryDividerMs >=
        STM32_TELEMETRY_PERIOD_MS) {
        gTelemetryDividerMs = 0U;
        queueTelemetry();
    }
}

void Stm32Link_notifyEvent(Stm32LinkEvent event)
{
    const char *frame;

    switch (event) {
        case STM32_EVENT_ACCELERATING:
            frame = "$E,ACCEL\r\n";
            break;
        case STM32_EVENT_DECELERATING:
            frame = "$E,DECEL\r\n";
            break;
        case STM32_EVENT_ENTER_CURVE:
            frame = "$E,CURVE\r\n";
            gSegmentState = SEGMENT_CURVE;
            gCurveConfirmMs = 0U;
            gStraightConfirmMs = 0U;
            break;
        case STM32_EVENT_ENTER_STRAIGHT:
        default:
            frame = "$E,STRAIGHT\r\n";
            gSegmentState = SEGMENT_STRAIGHT;
            gCurveConfirmMs = 0U;
            gStraightConfirmMs = 0U;
            break;
    }

    (void) queueBytes(frame, textLength(frame));
}

int32_t Stm32Link_getAngularAccelerationCentiDps2(void)
{
    return gAngularAcceleration;
}

int32_t Stm32Link_getLongitudinalAccelerationMmps2(void)
{
    return gLongitudinalAcceleration;
}

uint32_t Stm32Link_getDroppedFrameCount(void)
{
    return gDroppedFrameCount;
}

#include "ab_mission.h"

#include "app_config.h"
#include "app_time.h"
#include "encoder.h"
#include "gray_sensor.h"
#include "motor.h"
#include "stm32_link.h"
#include "straight_control.h"

static volatile bool gEmergencyStopRequested;
static volatile bool gRightCurveDetected;
static ABMissionState gState;
static ABMissionMode gMode;
static uint8_t gControlDivider;
static uint16_t gBrakePhaseElapsedMs;
static uint8_t gCurveConfirmTicks;
static int8_t gCurveDirection;
static uint8_t gMarkerConfirmTicks;
static uint8_t gNormalLineRecentTicks;
static bool gLapStartMarkerCaptured;
static uint32_t gLapStartMarkerCount;
static bool gPassPointDetected;
static uint32_t gPassPointCount;
static uint32_t gBodyClearDeadlineMs;
static uint32_t gMissionRuntimeMs;

static bool isLapMode(ABMissionMode mode)
{
    return (mode == AB_MISSION_MODE_LAP_IMMEDIATE) ||
           (mode == AB_MISSION_MODE_LAP_PASS);
}

static bool isRunningState(ABMissionState state)
{
    return (state == AB_MISSION_CRUISE) ||
           (state == AB_MISSION_APPROACH) ||
           (state == AB_MISSION_PASS_BODY) ||
           (state == AB_MISSION_POST_RUN) ||
           (state == AB_MISSION_DECEL) ||
           (state == AB_MISSION_BRAKE_WAIT) ||
           (state == AB_MISSION_REVERSE_BRAKE);
}

static bool isLineControlState(ABMissionState state)
{
    return (state == AB_MISSION_CRUISE) ||
           (state == AB_MISSION_APPROACH) ||
           (state == AB_MISSION_PASS_BODY) ||
           (state == AB_MISSION_POST_RUN) ||
           (state == AB_MISSION_DECEL);
}

static void beginBrakeWait(void)
{
    StraightControl_stop();
    Motor_stop();
    gBrakePhaseElapsedMs = 0U;
    gCurveConfirmTicks = 0U;
    gCurveDirection = 0;
    gMarkerConfirmTicks = 0U;
    gState = AB_MISSION_BRAKE_WAIT;
}

static void finishMission(ABMissionState finalState)
{
    StraightControl_stop();
    Motor_stop();
    AppTime_stop();
    gState = finalState;
}

static void resetMissionTracking(void)
{
    gControlDivider = 0U;
    gBrakePhaseElapsedMs = 0U;
    gCurveConfirmTicks = 0U;
    gCurveDirection = 0;
    gMarkerConfirmTicks = 0U;
    gNormalLineRecentTicks = 0U;
    gLapStartMarkerCaptured = false;
    gLapStartMarkerCount = 0U;
    gPassPointDetected = false;
    gPassPointCount = 0U;
    gBodyClearDeadlineMs = 0U;
    gMissionRuntimeMs = 0U;
}

void ABMission_init(void)
{
    gEmergencyStopRequested = false;
    gRightCurveDetected = false;
    gState = AB_MISSION_IDLE;
    gMode = AB_MISSION_MODE_NONE;
    resetMissionTracking();
}

static void startMission(ABMissionMode mode, int32_t cruisePermille)
{
    if (isRunningState(gState)) {
        return;
    }

    Motor_stop();
    Encoder_reset();
    gEmergencyStopRequested = false;
    gRightCurveDetected = false;
    gMode = mode;
    resetMissionTracking();

    AppTime_start();
    StraightControl_start(cruisePermille);
    /*
     * Grayscale owns steering in all three tasks. Encoders remain active for
     * speed, traveled distance and stalled-wheel protection.
     */
    StraightControl_setEncoderSynchronizationEnabled(false);
    gState = AB_MISSION_CRUISE;
    Stm32Link_notifyEvent(STM32_EVENT_ACCELERATING);
    Stm32Link_notifyEvent(STM32_EVENT_ENTER_STRAIGHT);
}

void ABMission_startLapImmediate(void)
{
    startMission(
        AB_MISSION_MODE_LAP_IMMEDIATE,
        LAP_CRUISE_PWM_PERMILLE);
}

void ABMission_startAbPass(void)
{
    startMission(
        AB_MISSION_MODE_AB_PASS,
        AB_CRUISE_PWM_PERMILLE);
}

void ABMission_startLapPass(void)
{
    startMission(
        AB_MISSION_MODE_LAP_PASS,
        LAP_CRUISE_PWM_PERMILLE);
}

static void updateNormalLineHistory(void)
{
    uint8_t activeCount = GraySensor_getActiveCount();

    if (GraySensor_hasLine() &&
        (activeCount >= 1U) && (activeCount <= 2U)) {
        gNormalLineRecentTicks =
            MARKER_NORMAL_LINE_RECENT_10MS_TICKS;
    } else if (gNormalLineRecentTicks > 0U) {
        gNormalLineRecentTicks--;
    }
}

static bool confirmWideMarker(bool requireRecentNormalLine)
{
    bool candidate = GraySensor_isFinishMarkerCandidate();

    if (requireRecentNormalLine &&
        (gNormalLineRecentTicks == 0U)) {
        candidate = false;
    }

    if (candidate) {
        if (gMarkerConfirmTicks <
            LAP_FINISH_CONFIRM_10MS_TICKS) {
            gMarkerConfirmTicks++;
        }
    } else {
        gMarkerConfirmTicks = 0U;
    }

    if (gMarkerConfirmTicks >=
        LAP_FINISH_CONFIRM_10MS_TICKS) {
        gMarkerConfirmTicks = 0U;
        gNormalLineRecentTicks = 0U;
        return true;
    }
    return false;
}

static bool captureInitialLapMarker(uint32_t averageCount)
{
    if (gLapStartMarkerCaptured) {
        return true;
    }

    /*
     * The probe starts behind the start/finish stripe. The first wide stripe
     * is recorded only as the lap distance reference and can never complete
     * the task.
     */
    if (averageCount <= LAP_START_MARKER_SEARCH_COUNT) {
        if (confirmWideMarker(false)) {
            gLapStartMarkerCaptured = true;
            gLapStartMarkerCount = averageCount;
        }
        return false;
    }

    /*
     * If the initial stripe was not sampled, retain an encoder-only fallback
     * referenced to the button position.
     */
    gLapStartMarkerCaptured = true;
    gLapStartMarkerCount = 0U;
    gMarkerConfirmTicks = 0U;
    return true;
}

static uint32_t relativeLapCount(uint32_t averageCount)
{
    return (averageCount >= gLapStartMarkerCount) ?
        (averageCount - gLapStartMarkerCount) : 0U;
}

static uint32_t calculateBodyClearTimeoutMs(void)
{
    uint32_t speed = StraightControl_getSpeedMmPerSecond();
    uint32_t timeoutMs;

    if (speed < 50U) {
        speed = 50U;
    }

    timeoutMs =
        (((VEHICLE_LENGTH_MM * 1000U) / speed) * 2U) +
        PASS_BODY_TIMEOUT_MARGIN_MS;
    if (timeoutMs < PASS_BODY_TIMEOUT_MIN_MS) {
        timeoutMs = PASS_BODY_TIMEOUT_MIN_MS;
    } else if (timeoutMs > PASS_BODY_TIMEOUT_MAX_MS) {
        timeoutMs = PASS_BODY_TIMEOUT_MAX_MS;
    }
    return timeoutMs;
}

static void beginPassPoint(uint32_t averageCount)
{
    gPassPointDetected = true;
    gPassPointCount = averageCount;
    gBodyClearDeadlineMs =
        gMissionRuntimeMs + calculateBodyClearTimeoutMs();
    StraightControl_setTargetBase(
        PASS_AFTER_POINT_PWM_PERMILLE);
    Stm32Link_notifyEvent(STM32_EVENT_DECELERATING);
    if (gMode == AB_MISSION_MODE_AB_PASS) {
        Stm32Link_notifyEvent(STM32_EVENT_ENTER_CURVE);
    }
    gState = AB_MISSION_PASS_BODY;
}

static void servicePassPoint(uint32_t averageCount)
{
    uint32_t countAfterPoint =
        (averageCount >= gPassPointCount) ?
        (averageCount - gPassPointCount) : 0U;

    if ((gState == AB_MISSION_PASS_BODY) &&
        (gMissionRuntimeMs >= gBodyClearDeadlineMs) &&
        (countAfterPoint < PASS_BODY_CLEAR_COUNT)) {
        finishMission(AB_MISSION_FAULT);
        return;
    }

    if ((gState == AB_MISSION_PASS_BODY) &&
        (countAfterPoint >= PASS_BODY_CLEAR_COUNT)) {
        /*
         * The full 350 mm body has crossed B/A. Freeze the measured task time
         * but deliberately keep following the line.
         */
        AppTime_stop();
        gState = AB_MISSION_POST_RUN;
    }

    if (countAfterPoint >= PASS_HARD_STOP_COUNT) {
        beginBrakeWait();
        return;
    }

    if ((gState != AB_MISSION_DECEL) &&
        (countAfterPoint >= PASS_BRAKE_AFTER_COUNT)) {
        StraightControl_setTargetBase(0);
        Stm32Link_notifyEvent(
            STM32_EVENT_DECELERATING);
        gState = AB_MISSION_DECEL;
        return;
    }

    if ((gState == AB_MISSION_DECEL) &&
        (StraightControl_getRampedBase() == 0)) {
        beginBrakeWait();
    }
}

static bool detectBcurve(void)
{
    int16_t lineError = StraightControl_getLineError();
    int8_t direction = 0;

    if (StraightControl_hasValidLine()) {
        if (lineError >= GRAY_CURVE_ERROR_THRESHOLD) {
            direction = 1;
        } else if (lineError <= -GRAY_CURVE_ERROR_THRESHOLD) {
            direction = -1;
        }
    }

    if (direction == 0) {
        gCurveConfirmTicks = 0U;
        gCurveDirection = 0;
    } else if (direction != gCurveDirection) {
        gCurveDirection = direction;
        gCurveConfirmTicks = 1U;
    } else if (gCurveConfirmTicks <
               GRAY_CURVE_CONFIRM_10MS_TICKS) {
        gCurveConfirmTicks++;
    }

    return gRightCurveDetected ||
        (gCurveConfirmTicks >=
            GRAY_CURVE_CONFIRM_10MS_TICKS);
}

static void serviceLapMode(uint32_t averageCount)
{
    uint32_t lapCount;

    if (!captureInitialLapMarker(averageCount)) {
        return;
    }

    lapCount = relativeLapCount(averageCount);

    if (gPassPointDetected) {
        servicePassPoint(averageCount);
        return;
    }

    if ((gState == AB_MISSION_CRUISE) &&
        (lapCount >= LAP_SLOW_DOWN_COUNT)) {
        StraightControl_setTargetBase(
            LAP_APPROACH_PWM_PERMILLE);
        Stm32Link_notifyEvent(
            STM32_EVENT_DECELERATING);
        gState = AB_MISSION_APPROACH;
    }

    if ((lapCount >= LAP_FINISH_ARM_RELATIVE_COUNT) &&
        confirmWideMarker(true)) {
        if (gMode == AB_MISSION_MODE_LAP_IMMEDIATE) {
            AppTime_stop();
            beginBrakeWait();
        } else {
            beginPassPoint(averageCount);
        }
        return;
    }

    if (lapCount >= LAP_MISSED_MARKER_RELATIVE_COUNT) {
        finishMission(AB_MISSION_FAULT);
    }
}

static void serviceAbPassMode(uint32_t averageCount)
{
    if (gPassPointDetected) {
        servicePassPoint(averageCount);
        return;
    }

    if ((gState == AB_MISSION_CRUISE) &&
        (averageCount >= AB_SLOW_DOWN_COUNT)) {
        StraightControl_setTargetBase(
            AB_APPROACH_PWM_PERMILLE);
        Stm32Link_notifyEvent(
            STM32_EVENT_DECELERATING);
        gState = AB_MISSION_APPROACH;
    }

    /*
     * The persistent line displacement identifies entry into B's circular
     * section. The 1500 mm encoder target remains a fallback if the curve
     * signature is weak.
     */
    if (averageCount >= AB_CURVE_ARM_COUNT) {
        if (detectBcurve() ||
            (averageCount >= AB_TARGET_COUNT)) {
            beginPassPoint(averageCount);
        }
    }
}

void ABMission_task1ms(void)
{
    uint32_t averageCount;
    uint32_t timeoutMs;

    if (gEmergencyStopRequested) {
        gEmergencyStopRequested = false;
        if (isRunningState(gState)) {
            finishMission(AB_MISSION_ABORTED);
        } else {
            Motor_stop();
        }
        return;
    }

    if (!isRunningState(gState)) {
        return;
    }

    if (gMissionRuntimeMs < UINT32_MAX) {
        gMissionRuntimeMs++;
    }

    timeoutMs = isLapMode(gMode) ?
        LAP_TIMEOUT_MS : AB_TIMEOUT_MS;
    if (gMissionRuntimeMs >= timeoutMs) {
        finishMission(AB_MISSION_FAULT);
        return;
    }

    if (gState == AB_MISSION_BRAKE_WAIT) {
        if (gBrakePhaseElapsedMs < AB_BRAKE_ZERO_WAIT_MS) {
            gBrakePhaseElapsedMs++;
        }
        if (gBrakePhaseElapsedMs >= AB_BRAKE_ZERO_WAIT_MS) {
            Motor_driveReversePermille(
                AB_REVERSE_BRAKE_PWM_PERMILLE,
                AB_REVERSE_BRAKE_PWM_PERMILLE);
            gBrakePhaseElapsedMs = 0U;
            gState = AB_MISSION_REVERSE_BRAKE;
        }
        return;
    }

    if (gState == AB_MISSION_REVERSE_BRAKE) {
        if (gBrakePhaseElapsedMs < AB_REVERSE_BRAKE_TIME_MS) {
            gBrakePhaseElapsedMs++;
        }
        if (gBrakePhaseElapsedMs >= AB_REVERSE_BRAKE_TIME_MS) {
            finishMission(AB_MISSION_DONE);
        }
        return;
    }

    if (!isLineControlState(gState)) {
        return;
    }

    gControlDivider++;
    if (gControlDivider < 10U) {
        return;
    }
    gControlDivider = 0U;

    StraightControl_task10ms();
    if (StraightControl_hasEncoderFault()) {
        finishMission(AB_MISSION_FAULT);
        return;
    }

    updateNormalLineHistory();
    averageCount = Encoder_getAverageMagnitude();

    if (isLapMode(gMode)) {
        serviceLapMode(averageCount);
    } else {
        serviceAbPassMode(averageCount);
    }
}

void ABMission_requestEmergencyStop(void)
{
    Motor_emergencyStop();
    gEmergencyStopRequested = true;
}

void ABMission_requestEmergencyStopFromISR(void)
{
    Motor_emergencyStop();
    gEmergencyStopRequested = true;
}

void ABMission_notifyRightCurve(void)
{
    gRightCurveDetected = true;
}

ABMissionState ABMission_getState(void)
{
    return gState;
}

ABMissionMode ABMission_getMode(void)
{
    return gMode;
}

uint32_t ABMission_getAverageCount(void)
{
    return Encoder_getAverageMagnitude();
}

uint32_t ABMission_getDistanceMm(void)
{
    return Encoder_countsToMillimeters(
        Encoder_getAverageMagnitude());
}

uint32_t ABMission_getElapsedMs(void)
{
    return AppTime_getElapsedMs();
}

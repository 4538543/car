#include "ab_mission.h"

#include "app_config.h"
#include "app_time.h"
#include "encoder.h"
#include "motor.h"
#include "straight_control.h"

static volatile bool gEmergencyStopRequested;
static volatile bool gRightCurveDetected;
static ABMissionState gState;
static uint8_t gControlDivider;
static uint16_t gBrakePhaseElapsedMs;
static uint8_t gCurveConfirmTicks;
static int8_t gCurveDirection;
static ABMissionMode gMode;

static bool isRunningState(ABMissionState state)
{
    return (state == AB_MISSION_CRUISE) ||
           (state == AB_MISSION_APPROACH) ||
           (state == AB_MISSION_DECEL) ||
           (state == AB_MISSION_BRAKE_WAIT) ||
           (state == AB_MISSION_REVERSE_BRAKE);
}

static bool isStraightControlState(ABMissionState state)
{
    return (state == AB_MISSION_CRUISE) ||
           (state == AB_MISSION_APPROACH) ||
           (state == AB_MISSION_DECEL);
}

static void beginBrakeWait(void)
{
    StraightControl_stop();
    Motor_stop();
    gBrakePhaseElapsedMs = 0U;
    gCurveConfirmTicks = 0U;
    gCurveDirection = 0;
    gState = AB_MISSION_BRAKE_WAIT;
}

static void finishMission(ABMissionState finalState)
{
    StraightControl_stop();
    Motor_stop();
    AppTime_stop();
    gState = finalState;
}

void ABMission_init(void)
{
    gEmergencyStopRequested = false;
    gRightCurveDetected = false;
    gState = AB_MISSION_IDLE;
    gControlDivider = 0U;
    gBrakePhaseElapsedMs = 0U;
    gCurveConfirmTicks = 0U;
    gCurveDirection = 0;
    gMode = AB_MISSION_MODE_NONE;
}

static void startMission(
    ABMissionMode mode, int32_t cruisePermille)
{
    if (isRunningState(gState)) {
        return;
    }

    Motor_stop();
    Encoder_reset();
    gEmergencyStopRequested = false;
    gRightCurveDetected = false;
    gControlDivider = 0U;
    gBrakePhaseElapsedMs = 0U;
    gCurveConfirmTicks = 0U;
    gCurveDirection = 0;
    gMode = mode;

    AppTime_start();
    StraightControl_start(cruisePermille);
    StraightControl_setEncoderSynchronizationEnabled(
        mode == AB_MISSION_MODE_AB);
    gState = AB_MISSION_CRUISE;
}

void ABMission_start(void)
{
    startMission(AB_MISSION_MODE_AB, AB_CRUISE_PWM_PERMILLE);
}

void ABMission_startLap(void)
{
    startMission(AB_MISSION_MODE_LAP, LAP_CRUISE_PWM_PERMILLE);
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

    timeoutMs = (gMode == AB_MISSION_MODE_LAP) ?
        LAP_TIMEOUT_MS : AB_TIMEOUT_MS;
    if (AppTime_getElapsedMs() >= timeoutMs) {
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

    if (!isStraightControlState(gState)) {
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

    averageCount = Encoder_getAverageMagnitude();

    if (gMode == AB_MISSION_MODE_LAP) {
        if ((gState == AB_MISSION_CRUISE) &&
            (averageCount >= LAP_SLOW_DOWN_COUNT)) {
            StraightControl_setTargetBase(
                LAP_APPROACH_PWM_PERMILLE);
            gState = AB_MISSION_APPROACH;
        }

        if (averageCount >= LAP_TARGET_COUNT) {
            beginBrakeWait();
            return;
        }

        if ((gState != AB_MISSION_DECEL) &&
            (averageCount >= LAP_BRAKE_START_COUNT)) {
            StraightControl_setTargetBase(0);
            gState = AB_MISSION_DECEL;
            return;
        }

        if ((gState == AB_MISSION_DECEL) &&
            (StraightControl_getRampedBase() == 0)) {
            beginBrakeWait();
        }
        return;
    }

    if ((gState == AB_MISSION_CRUISE) &&
        (averageCount >= AB_SLOW_DOWN_COUNT)) {
        StraightControl_setTargetBase(AB_APPROACH_PWM_PERMILLE);
        gState = AB_MISSION_APPROACH;
    }

    /*
     * Near B, a line that stays away from the center on the same side for
     * three samples means the sensor board has reached the circular arc.
     * A single correction spike cannot stop the vehicle.
     */
    if (averageCount >= AB_CURVE_ARM_COUNT) {
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

        if (gRightCurveDetected ||
            (gCurveConfirmTicks >=
             GRAY_CURVE_CONFIRM_10MS_TICKS)) {
            beginBrakeWait();
            return;
        }
    } else {
        gCurveConfirmTicks = 0U;
        gCurveDirection = 0;
    }

    if (averageCount >= AB_TARGET_COUNT) {
        beginBrakeWait();
        return;
    }

    if ((gState != AB_MISSION_DECEL) &&
        (averageCount >= AB_BRAKE_START_COUNT)) {
        StraightControl_setTargetBase(0);
        gState = AB_MISSION_DECEL;
        return;
    }

    if ((gState == AB_MISSION_DECEL) &&
        (StraightControl_getRampedBase() == 0)) {
        beginBrakeWait();
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
    return Encoder_countsToMillimeters(Encoder_getAverageMagnitude());
}

uint32_t ABMission_getElapsedMs(void)
{
    return AppTime_getElapsedMs();
}

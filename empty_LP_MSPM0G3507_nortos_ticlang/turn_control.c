#include "turn_control.h"

#include <stdbool.h>
#include <math.h>

#include "app_config.h"
#include "gyro.h"
#include "motor.h"

volatile float gTurnYawDelta;
volatile float gTurnTargetDeg;
volatile float gTurnAbsoluteError;

static float gStartYaw;
static float gAngleMagnitude;
static int8_t gYawDirection;
static int8_t gInitialPhysicalDirection;
static int8_t gYawPerPhysicalDirection;
static uint16_t gElapsedMs;
static uint16_t gStableMs;
static bool gInTargetWindow;
static bool gAbsolutePivotMode;

static float wrap180(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static float normalize360(float angle)
{
    while (angle >= 360.0f) angle -= 360.0f;
    while (angle < 0.0f) angle += 360.0f;
    return angle;
}

static void drivePhysicalDirection(int8_t direction, uint16_t outerDuty)
{
    if (direction > 0) {
        /* Left physical turn: right wheel advances faster. */
        Motor_setForward(TURN_INNER_PERMILLE, outerDuty);
    } else {
        /* Right physical turn: left wheel advances faster. */
        Motor_setForward(outerDuty, TURN_INNER_PERMILLE);
    }
}

bool TurnControl_start(int8_t direction, float angleDeg)
{
    if ((!Gyro_hasYaw()) || (direction == 0)) {
        return false;
    }

    gInitialPhysicalDirection = (direction > 0) ? 1 : -1;
    gYawPerPhysicalDirection = 0;
    gStartYaw = Gyro_getYawDeg();
    gAngleMagnitude = fabsf(angleDeg);
    gYawDirection = 0;
    gTurnTargetDeg = 0.0f;
    gTurnYawDelta = 0.0f;
    gElapsedMs = 0U;
    gStableMs = 0U;
    gInTargetWindow = false;
    gAbsolutePivotMode = false;
    gTurnAbsoluteError = 0.0f;
    Motor_enable();

    /* Forward-only pivot: no wheel is ever commanded in reverse. */
    drivePhysicalDirection(gInitialPhysicalDirection, TURN_OUTER_PERMILLE);
    return true;
}

bool TurnControl_startAbsolute(float targetYawDeg)
{
    /* Require a decoded yaw, but tolerate a short receive-age excursion.
     * Otherwise Q3/Q4 can fault-stop immediately when the key happens to be
     * pressed between UART frame bursts. */
    if (!Gyro_hasYaw()) {
        return false;
    }

    gAbsolutePivotMode = true;
    gTurnTargetDeg = normalize360(targetYawDeg);
    gTurnYawDelta = 0.0f;
    gTurnAbsoluteError = wrap180(gTurnTargetDeg - normalize360(
        GYRO_ABSOLUTE_YAW_SIGN * Gyro_getCourseYawDeg()));
    gElapsedMs = 0U;
    gStableMs = 0U;
    gInTargetWindow = false;
    Motor_enable();
    Motor_brakeAll();
    return true;
}

static Turn_Result absolutePivotTask(void)
{
    float yaw = normalize360(GYRO_ABSOLUTE_YAW_SIGN *
        Gyro_getCourseYawDeg());
    float rate = Gyro_hasRate() ?
        GYRO_ABSOLUTE_YAW_SIGN * Gyro_getRateDps() : 0.0f;
    float predictedYaw = normalize360(yaw +
        rate * ABS_TURN_PREDICTION_S);
    float error = wrap180(gTurnTargetDeg - yaw);
    float predictedError = wrap180(gTurnTargetDeg - predictedYaw);
    float magnitude = fabsf(error);

    gTurnAbsoluteError = error;
    gTurnYawDelta = error;

    if (magnitude <= ABS_TURN_TOLERANCE_DEG) {
        gInTargetWindow = true;
        Motor_brakeAll();
    } else if (gInTargetWindow &&
               (magnitude < ABS_TURN_RESTART_ERROR_DEG)) {
        /* Once captured, ignore a small inertial overshoot. */
        Motor_brakeAll();
    } else {
        gInTargetWindow = false;
    }

    if (gInTargetWindow &&
        (fabsf(rate) <= ABS_TURN_RATE_TOLERANCE_DPS)) {
        if (gStableMs < ABS_TURN_SETTLE_MS) gStableMs++;
    } else {
        gStableMs = 0U;
    }
    if (gStableMs >= ABS_TURN_SETTLE_MS) {
        Motor_brakeAll();
        return TURN_REACHED;
    }

    if (!gInTargetWindow) {
        uint16_t duty;
        float predictedMagnitude = fabsf(predictedError);

        /* If the rate prediction says inertia will carry the car across the
         * target, brake instead of immediately reversing. */
        if ((error * predictedError <= 0.0f) &&
            (magnitude < ABS_TURN_FAST_ZONE_DEG)) {
            Motor_brakeAll();
            return TURN_RUNNING;
        }
        if (predictedMagnitude >= ABS_TURN_FAST_ZONE_DEG) {
            duty = ABS_TURN_MAX_PERMILLE;
        } else if (predictedMagnitude >= ABS_TURN_FINE_ZONE_DEG) {
            duty = ABS_TURN_MID_PERMILLE;
        } else {
            duty = ABS_TURN_MIN_PERMILLE;
        }

        /* Positive course-yaw error uses left-forward/right-reverse; a
         * negative error uses the opposite pair. */
        if (error > 0.0f) {
            Motor_setSigned((int16_t)duty, (int16_t)-duty);
        } else {
            Motor_setSigned((int16_t)-duty, (int16_t)duty);
        }
    }
    return TURN_RUNNING;
}

Turn_Result TurnControl_task1ms(void)
{
    float remaining;
    float rate;

    gElapsedMs++;
    if ((!Gyro_hasYaw()) ||
        (gElapsedMs >= TURN_TIMEOUT_MS)) {
        Motor_brakeAll();
        return TURN_FAULT;
    }

    if (gAbsolutePivotMode) {
        return absolutePivotTask();
    }

    gTurnYawDelta = wrap180(Gyro_getYawDeg() - gStartYaw);
    /* Learn the sensor sign from the commanded physical turn. This makes the
     * controller independent of gyro mounting direction and yaw convention. */
    if ((gYawDirection == 0) &&
        (fabsf(gTurnYawDelta) >= TURN_SIGN_DETECT_DEG)) {
        gYawDirection = (gTurnYawDelta > 0.0f) ? 1 : -1;
        gYawPerPhysicalDirection =
            gYawDirection * gInitialPhysicalDirection;
        gTurnTargetDeg = (float) gYawDirection * gAngleMagnitude;
    }
    if (gYawDirection == 0) {
        return TURN_RUNNING;
    }
    remaining = gTurnTargetDeg - gTurnYawDelta;
    rate = Gyro_hasRate() ? Gyro_getRateDps() : 0.0f;
    if (fabsf(remaining) <= TURN_TOLERANCE_DEG) {
        gInTargetWindow = true;
        Motor_brakeAll();
    } else if (gInTargetWindow &&
               (fabsf(remaining) < TURN_RESTART_ERROR_DEG)) {
        /* Hysteresis: after entering the target window, do not reverse for a
         * tiny inertial overshoot. This removes left-right chatter. */
        Motor_brakeAll();
    } else {
        gInTargetWindow = false;
    }

    if (gInTargetWindow &&
        (fabsf(rate) <= TURN_RATE_TOLERANCE_DPS)) {
        if (gStableMs < TURN_SETTLE_MS) gStableMs++;
    } else {
        gStableMs = 0U;
    }

    if (gStableMs >= TURN_SETTLE_MS) {
        Motor_brakeAll();
        return TURN_REACHED;
    }

    if (!gInTargetWindow) {
        int8_t desiredYawDirection = (remaining > 0.0f) ? 1 : -1;
        int8_t desiredPhysicalDirection =
            desiredYawDirection * gYawPerPhysicalDirection;
        float error = fabsf(remaining);
        uint16_t duty;

        /* Continuous deceleration: full speed outside 15 degrees, then
         * decrease linearly to the minimum usable motor duty. */
        if (error >= TURN_SLOW_ZONE_DEG) {
            duty = TURN_OUTER_PERMILLE;
        } else {
            duty = (uint16_t) (TURN_CORRECT_MIN_PERMILLE +
                ((TURN_OUTER_PERMILLE - TURN_CORRECT_MIN_PERMILLE) *
                 error / TURN_SLOW_ZONE_DEG));
        }
        drivePhysicalDirection(desiredPhysicalDirection, duty);
    }
    return TURN_RUNNING;
}

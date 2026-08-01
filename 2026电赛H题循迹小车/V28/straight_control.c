#include "straight_control.h"

#include "app_config.h"
#include "encoder.h"
#include "gray_sensor.h"
#include "motor.h"

static bool gActive;
static bool gEncoderFault;
static int32_t gRampedBasePermille;
static int32_t gTargetBasePermille;
static EncoderSnapshot gPrevious;
static uint32_t gEncoderFaultTicks;
static int32_t gLineIntegral;
static int16_t gPreviousLineError;
static int16_t gLastValidLineError;
static uint8_t gLineLostTicks;
static bool gEncoderSynchronizationEnabled;
static uint32_t gSpeedMmPerSecond;
static bool gStartupCenterAcceptance;
static bool gCurveAssistActive;
static int8_t gCurveAssistDirection;
static uint8_t gCurveAssistEnterTicks;
static uint8_t gCurveAssistReleaseTicks;
static uint16_t gLineGainPercent;
static uint16_t gDerivativePercent;
static int32_t gRampUpStep;
static int32_t gRampDownStep;
static bool gCurveSpeedActive;
static uint8_t gCurveSpeedEnterTicks;
static uint8_t gStraightConfirmTicks;
static int32_t gRampedSegmentLimitPermille;
static int32_t gCurveBasePermille;

static uint32_t magnitude32(int32_t value)
{
    if (value >= 0) {
        return (uint32_t)value;
    }
    return (uint32_t)(-(int64_t)value);
}

static int32_t clampCorrection(int32_t correction)
{
    if (correction > STRAIGHT_MAX_CORRECTION_PERMILLE) {
        return STRAIGHT_MAX_CORRECTION_PERMILLE;
    }
    if (correction < -STRAIGHT_MAX_CORRECTION_PERMILLE) {
        return -STRAIGHT_MAX_CORRECTION_PERMILLE;
    }
    return correction;
}

static int32_t clampLineCorrection(int32_t correction)
{
    int32_t limit = (LINE_PID_MAX_CORRECTION_PERMILLE *
        (int32_t)gLineGainPercent) / 100;

    if (correction > limit) {
        return limit;
    }
    if (correction < -limit) {
        return -limit;
    }
    return correction;
}

static int32_t clampDriveCorrection(int32_t correction)
{
    int32_t limit = (DRIVE_MAX_CORRECTION_PERMILLE *
        (int32_t)gLineGainPercent) / 100;

    if (correction > limit) {
        return limit;
    }
    if (correction < -limit) {
        return -limit;
    }
    return correction;
}

static int32_t applySignedDeadband(int32_t value, int32_t deadband)
{
    if (value > deadband) {
        return value - deadband;
    }
    if (value < -deadband) {
        return value + deadband;
    }
    return 0;
}

static void updateCurveAssist(int32_t lineError)
{
    uint32_t errorMagnitude = magnitude32(lineError);
    int8_t errorDirection =
        (lineError > 0) ? 1 : ((lineError < 0) ? -1 : 0);

    if (!gCurveAssistActive) {
        gCurveAssistReleaseTicks = 0U;
        if ((gRampedBasePermille >=
                LINE_STEERING_FULL_RAMP_PERMILLE) &&
            (errorMagnitude >=
                CURVE_ASSIST_ENTER_ERROR)) {
            if (errorDirection !=
                gCurveAssistDirection) {
                gCurveAssistDirection =
                    errorDirection;
                gCurveAssistEnterTicks = 1U;
            } else if (gCurveAssistEnterTicks <
                       CURVE_ASSIST_ENTER_10MS_TICKS) {
                gCurveAssistEnterTicks++;
            }

            if (gCurveAssistEnterTicks >=
                CURVE_ASSIST_ENTER_10MS_TICKS) {
                gCurveAssistActive = true;
                gCurveAssistEnterTicks = 0U;
            }
        } else {
            gCurveAssistEnterTicks = 0U;
            gCurveAssistDirection = 0;
        }
        return;
    }

    if ((errorDirection ==
            -gCurveAssistDirection) &&
        (errorMagnitude >=
            CURVE_ASSIST_RELEASE_ERROR)) {
        if (gCurveAssistReleaseTicks <
            CURVE_ASSIST_RELEASE_10MS_TICKS) {
            gCurveAssistReleaseTicks++;
        }
        if (gCurveAssistReleaseTicks >=
            CURVE_ASSIST_RELEASE_10MS_TICKS) {
            gCurveAssistActive = false;
            gCurveAssistDirection = 0;
            gCurveAssistReleaseTicks = 0U;
        }
    } else {
        gCurveAssistReleaseTicks = 0U;
    }
}

static int32_t calculateLinePid(void)
{
    int32_t error;
    int32_t derivative;
    int32_t correction;
    uint32_t errorMagnitude;

    GraySensor_sample();
    if (!GraySensor_hasLine()) {
        if (GraySensor_isFinishMarkerCandidate()) {
            gPreviousLineError = 0;
            return 0;
        }

        /*
         * Keep steering toward the last known line briefly if it leaves an
         * outer probe in a curve. Zero correction here would send the vehicle
         * along the tangent and make reacquisition less likely.
         */
        if ((gLineLostTicks < LINE_LOST_HOLD_10MS_TICKS) &&
            ((gLastValidLineError >= LINE_LOST_MIN_ERROR) ||
             (gLastValidLineError <= -LINE_LOST_MIN_ERROR))) {
            gLineLostTicks++;
            correction = (gLastValidLineError > 0) ?
                LINE_LOST_RECOVERY_CORRECTION_PERMILLE :
                -LINE_LOST_RECOVERY_CORRECTION_PERMILLE;
            correction = (correction * (int32_t)gLineGainPercent) / 100;
            return clampLineCorrection(correction);
        }

        gLineIntegral = 0;
        gPreviousLineError = 0;
        gLastValidLineError = 0;
        return 0;
    }

    error = GraySensor_getLineError();
    if (gStartupCenterAcceptance &&
        ((GraySensor_getBlackMask() &
            GRAY_CENTER_ACCEPT_SENSOR_MASK) != 0U) &&
        (GraySensor_getActiveCount() <=
            GRAY_CENTER_ACCEPT_MAX_ACTIVE_SENSORS)) {
        error = 0;
    }
    updateCurveAssist(error);
    gLineLostTicks = 0U;
    gLastValidLineError = (int16_t)error;

    /*
     * Zero error means both center probes are balanced, or the temporary
     * startup acceptance band is active. Clearing PID memory prevents a
     * derivative kick when the line reaches this target.
     */
    if (error == 0) {
        gLineIntegral = 0;
        gPreviousLineError = 0;
        gLastValidLineError = 0;
        return 0;
    }

    errorMagnitude = magnitude32(error);
    derivative = error - gPreviousLineError;
    gPreviousLineError = (int16_t)error;
    if (gDerivativePercent != 100U) {
        derivative = (derivative * (int32_t)gDerivativePercent) / 100;
    }

    /*
     * Digital probes jump in 100-unit steps. Use gentler gains for the first
     * probe outside the center band, while retaining the stronger V2 gains
     * on the outer probes needed by the 0.5 m radius curves.
     */
    if (errorMagnitude <= LINE_PID_NEAR_ERROR_LIMIT) {
        gLineIntegral = 0;
        correction =
            (error * LINE_PID_NEAR_KP_NUM +
             derivative * LINE_PID_NEAR_KD_NUM) /
            LINE_PID_SCALE;
        correction = (correction * (int32_t)gLineGainPercent) / 100;
        return clampLineCorrection(correction);
    }

    gLineIntegral += error;
    if (gLineIntegral > LINE_PID_INTEGRAL_LIMIT) {
        gLineIntegral = LINE_PID_INTEGRAL_LIMIT;
    } else if (gLineIntegral < -LINE_PID_INTEGRAL_LIMIT) {
        gLineIntegral = -LINE_PID_INTEGRAL_LIMIT;
    }

    correction =
        (error * LINE_PID_KP_NUM +
         gLineIntegral * LINE_PID_KI_NUM +
         derivative * LINE_PID_KD_NUM) /
        LINE_PID_SCALE;
    correction = (correction * (int32_t)gLineGainPercent) / 100;
    return clampLineCorrection(correction);
}

static int32_t clampOutput(int32_t output)
{
    if (output < 0) {
        return 0;
    }
    if (output > 1000) {
        return 1000;
    }
    return output;
}

static void updateRamp(void)
{
    if (gRampedBasePermille < gTargetBasePermille) {
        gRampedBasePermille += gRampUpStep;
        if (gRampedBasePermille > gTargetBasePermille) {
            gRampedBasePermille = gTargetBasePermille;
        }
    } else if (gRampedBasePermille > gTargetBasePermille) {
        gRampedBasePermille -= gRampDownStep;
        if (gRampedBasePermille < gTargetBasePermille) {
            gRampedBasePermille = gTargetBasePermille;
        }
    }
}

static bool isCurveMileageWindow(uint32_t distanceMm)
{
    return ((distanceMm >= LINE_FIRST_CURVE_WINDOW_START_MM) &&
            (distanceMm <= LINE_FIRST_CURVE_WINDOW_END_MM)) ||
           ((distanceMm >= LINE_SECOND_CURVE_WINDOW_START_MM) &&
            (distanceMm <= LINE_SECOND_CURVE_WINDOW_END_MM));
}

static void updateSegmentSpeedLimit(uint32_t distanceMm)
{
    uint32_t errorMagnitude = StraightControl_hasValidLine() ?
        magnitude32(StraightControl_getLineError()) : 0U;
    bool curveMileageWindow = isCurveMileageWindow(distanceMm);
    int32_t curveLimit = gCurveBasePermille;
    int32_t targetLimit;

    if (!gCurveSpeedActive) {
        gStraightConfirmTicks = 0U;
        if (curveMileageWindow &&
            (errorMagnitude >= LINE_CURVE_SPEED_ERROR_THRESHOLD)) {
            if (gCurveSpeedEnterTicks <
                LINE_CURVE_ENTER_CONFIRM_10MS_TICKS) {
                gCurveSpeedEnterTicks++;
            }
            if (gCurveSpeedEnterTicks >=
                LINE_CURVE_ENTER_CONFIRM_10MS_TICKS) {
                gCurveSpeedActive = true;
                gCurveSpeedEnterTicks = 0U;
            }
        } else {
            gCurveSpeedEnterTicks = 0U;
        }
    } else {
        gCurveSpeedEnterTicks = 0U;
        if (StraightControl_hasValidLine() &&
            (errorMagnitude <= LINE_STRAIGHT_ERROR_THRESHOLD)) {
            if (gStraightConfirmTicks <
                LINE_STRAIGHT_CONFIRM_10MS_TICKS) {
                gStraightConfirmTicks++;
            }
            if (gStraightConfirmTicks >=
                LINE_STRAIGHT_CONFIRM_10MS_TICKS) {
                gCurveSpeedActive = false;
                gStraightConfirmTicks = 0U;
            }
        } else {
            gStraightConfirmTicks = 0U;
        }
    }

    targetLimit = gCurveSpeedActive ? curveLimit : 1000;
    if (gRampedSegmentLimitPermille < targetLimit) {
        gRampedSegmentLimitPermille += gRampUpStep;
        if (gRampedSegmentLimitPermille > targetLimit) {
            gRampedSegmentLimitPermille = targetLimit;
        }
    } else if (gRampedSegmentLimitPermille > targetLimit) {
        gRampedSegmentLimitPermille -= gRampDownStep;
        if (gRampedSegmentLimitPermille < targetLimit) {
            gRampedSegmentLimitPermille = targetLimit;
        }
    }
}

void StraightControl_init(void)
{
    gActive = false;
    gEncoderFault = false;
    gRampedBasePermille = 0;
    gTargetBasePermille = 0;
    gEncoderFaultTicks = 0U;
    gLineIntegral = 0;
    gPreviousLineError = 0;
    gLastValidLineError = 0;
    gLineLostTicks = 0U;
    gEncoderSynchronizationEnabled = true;
    gSpeedMmPerSecond = 0U;
    gStartupCenterAcceptance = false;
    gCurveAssistActive = false;
    gCurveAssistDirection = 0;
    gCurveAssistEnterTicks = 0U;
    gCurveAssistReleaseTicks = 0U;
    gLineGainPercent = 100U;
    gDerivativePercent = 100U;
    gRampUpStep = PA18_RAMP_UP_STEP_PER_10MS;
    gRampDownStep = PA18_RAMP_DOWN_STEP_PER_10MS;
    gCurveSpeedActive = false;
    gCurveSpeedEnterTicks = 0U;
    gStraightConfirmTicks = 0U;
    gRampedSegmentLimitPermille = 1000;
    gCurveBasePermille = PA18_CURVE_PWM_PERMILLE;
    Encoder_getSnapshot(&gPrevious);
}

void StraightControl_start(int32_t targetBasePermille,
    uint16_t lineGainPercent, uint16_t derivativePercent,
    int32_t rampUpStep,
    int32_t rampDownStep, int32_t curveBasePermille)
{
    gActive = true;
    gEncoderFault = false;
    gRampedBasePermille = 0;
    gTargetBasePermille = targetBasePermille;
    gEncoderFaultTicks = 0U;
    gLineIntegral = 0;
    gPreviousLineError = 0;
    gLastValidLineError = 0;
    gLineLostTicks = 0U;
    gEncoderSynchronizationEnabled = true;
    gSpeedMmPerSecond = 0U;
    gStartupCenterAcceptance = true;
    gCurveAssistActive = false;
    gCurveAssistDirection = 0;
    gCurveAssistEnterTicks = 0U;
    gCurveAssistReleaseTicks = 0U;
    gLineGainPercent = lineGainPercent;
    gDerivativePercent = derivativePercent;
    gRampUpStep = rampUpStep;
    gRampDownStep = rampDownStep;
    gCurveSpeedActive = false;
    gCurveSpeedEnterTicks = 0U;
    gStraightConfirmTicks = 0U;
    gRampedSegmentLimitPermille = 1000;
    gCurveBasePermille = curveBasePermille;
    Encoder_getSnapshot(&gPrevious);
    Motor_driveForwardPermille(0, 0);
}

void StraightControl_setTargetBase(int32_t targetBasePermille)
{
    gTargetBasePermille = targetBasePermille;
}

void StraightControl_setEncoderSynchronizationEnabled(bool enabled)
{
    gEncoderSynchronizationEnabled = enabled;
}

void StraightControl_task10ms(void)
{
    EncoderSnapshot current;
    uint32_t leftPosition;
    uint32_t rightPosition;
    uint32_t previousLeft;
    uint32_t previousRight;
    uint32_t leftDelta;
    uint32_t rightDelta;
    int32_t positionError;
    int32_t speedError;
    int32_t correction;
    int32_t encoderCorrection;
    int32_t lineCorrection;
    int32_t driveBase;
    int32_t leftOutput;
    int32_t rightOutput;
    uint32_t instantaneousSpeed;
    uint32_t distanceMm;

    if (!gActive) {
        return;
    }

    Encoder_getSnapshot(&current);
    leftPosition = magnitude32(current.left);
    rightPosition = magnitude32(current.right);
    previousLeft = magnitude32(gPrevious.left);
    previousRight = magnitude32(gPrevious.right);
    leftDelta = (leftPosition >= previousLeft) ?
        (leftPosition - previousLeft) : 0U;
    rightDelta = (rightPosition >= previousRight) ?
        (rightPosition - previousRight) : 0U;
    gPrevious = current;
    instantaneousSpeed = Encoder_countsToMillimetersPerSecond(
        (leftDelta + rightDelta) / 2U, 10U);
    gSpeedMmPerSecond =
        (gSpeedMmPerSecond * 3U + instantaneousSpeed) / 4U;
    distanceMm = Encoder_countsToMillimeters(
        (leftPosition + rightPosition) / 2U);

    updateRamp();
    if (gStartupCenterAcceptance &&
        (gRampedBasePermille >=
            LINE_START_CENTER_ACCEPT_END_PERMILLE)) {
        gStartupCenterAcceptance = false;
    }

    if (gRampedBasePermille >= ENCODER_FAULT_CHECK_PWM_PERMILLE) {
        if ((leftDelta == 0U) || (rightDelta == 0U)) {
            if (gEncoderFaultTicks < ENCODER_FAULT_CONFIRM_10MS_TICKS) {
                gEncoderFaultTicks++;
            }
        } else {
            gEncoderFaultTicks = 0U;
        }

        if (gEncoderFaultTicks >= ENCODER_FAULT_CONFIRM_10MS_TICKS) {
            gEncoderFault = true;
            gActive = false;
            Motor_stop();
            return;
        }
    }

    positionError = applySignedDeadband(
        (int32_t)leftPosition - (int32_t)rightPosition,
        STRAIGHT_POSITION_ERROR_DEADBAND);
    speedError = applySignedDeadband(
        (int32_t)leftDelta - (int32_t)rightDelta,
        STRAIGHT_SPEED_ERROR_DEADBAND);
    lineCorrection = calculateLinePid();
    updateSegmentSpeedLimit(distanceMm);
    if (gCurveAssistActive &&
        (((gCurveAssistDirection > 0) &&
          (lineCorrection >= 0)) ||
         ((gCurveAssistDirection < 0) &&
          (lineCorrection <= 0))) &&
        (magnitude32(lineCorrection) <
            ((CURVE_ASSIST_FEEDFORWARD_PERMILLE *
              (uint32_t)gLineGainPercent) / 100U))) {
        lineCorrection =
            (int32_t) gCurveAssistDirection *
            ((CURVE_ASSIST_FEEDFORWARD_PERMILLE *
              (int32_t)gLineGainPercent) / 100);
    }
    if (gEncoderSynchronizationEnabled) {
        if (StraightControl_hasValidLine() &&
            (magnitude32(StraightControl_getLineError()) >=
                STRAIGHT_SYNC_DISABLE_LINE_ERROR)) {
            /*
             * A deliberate line-following turn must be allowed to create a
             * wheel-distance difference. The accumulated encoder position
             * loop would otherwise counter-steer on the next samples.
             */
            encoderCorrection = 0;
        } else {
            encoderCorrection =
                positionError * STRAIGHT_KP_POSITION +
                speedError * STRAIGHT_KP_SPEED;
            encoderCorrection = clampCorrection(encoderCorrection);
        }
    } else {
        /*
         * A complete lap requires different inner/outer wheel distances in
         * both semicircles. Equal-count correction would oppose line PID.
         * Encoder pulses are still checked for a stalled wheel.
         */
        encoderCorrection = 0;
    }
    correction = clampDriveCorrection(encoderCorrection + lineCorrection);

    if (gRampedBasePermille < LINE_STEERING_FULL_RAMP_PERMILLE) {
        correction =
            (correction * gRampedBasePermille) /
            LINE_STEERING_FULL_RAMP_PERMILLE;
    }

    driveBase = gRampedBasePermille;
    if (driveBase > gRampedSegmentLimitPermille) {
        driveBase = gRampedSegmentLimitPermille;
    }

    leftOutput = clampOutput(driveBase - correction);
    rightOutput = clampOutput(driveBase + correction);
    Motor_driveForwardPermille(leftOutput, rightOutput);
}

void StraightControl_stop(void)
{
    gActive = false;
    gRampedBasePermille = 0;
    gTargetBasePermille = 0;
    gLineIntegral = 0;
    gPreviousLineError = 0;
    gLastValidLineError = 0;
    gLineLostTicks = 0U;
    gSpeedMmPerSecond = 0U;
    gStartupCenterAcceptance = false;
    gCurveAssistActive = false;
    gCurveAssistDirection = 0;
    gCurveAssistEnterTicks = 0U;
    gCurveAssistReleaseTicks = 0U;
    gCurveSpeedActive = false;
    gCurveSpeedEnterTicks = 0U;
    gStraightConfirmTicks = 0U;
    gRampedSegmentLimitPermille = 1000;
    Motor_stop();
}

bool StraightControl_hasEncoderFault(void)
{
    return gEncoderFault;
}

int32_t StraightControl_getRampedBase(void)
{
    return gRampedBasePermille;
}

uint32_t StraightControl_getSpeedMmPerSecond(void)
{
    return gSpeedMmPerSecond;
}

bool StraightControl_hasValidLine(void)
{
    return GraySensor_hasLine();
}

int16_t StraightControl_getLineError(void)
{
    return GraySensor_getLineError();
}

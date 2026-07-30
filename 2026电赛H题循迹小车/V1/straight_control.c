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
static bool gEncoderSynchronizationEnabled;

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
    if (correction > LINE_PID_MAX_CORRECTION_PERMILLE) {
        return LINE_PID_MAX_CORRECTION_PERMILLE;
    }
    if (correction < -LINE_PID_MAX_CORRECTION_PERMILLE) {
        return -LINE_PID_MAX_CORRECTION_PERMILLE;
    }
    return correction;
}

static int32_t clampDriveCorrection(int32_t correction)
{
    if (correction > DRIVE_MAX_CORRECTION_PERMILLE) {
        return DRIVE_MAX_CORRECTION_PERMILLE;
    }
    if (correction < -DRIVE_MAX_CORRECTION_PERMILLE) {
        return -DRIVE_MAX_CORRECTION_PERMILLE;
    }
    return correction;
}

static int32_t calculateLinePid(void)
{
    int32_t error;
    int32_t derivative;
    int32_t correction;

    GraySensor_sample();
    if (!GraySensor_hasLine()) {
        gLineIntegral = 0;
        gPreviousLineError = 0;
        return 0;
    }

    error = GraySensor_getLineError();
    gLineIntegral += error;
    if (gLineIntegral > LINE_PID_INTEGRAL_LIMIT) {
        gLineIntegral = LINE_PID_INTEGRAL_LIMIT;
    } else if (gLineIntegral < -LINE_PID_INTEGRAL_LIMIT) {
        gLineIntegral = -LINE_PID_INTEGRAL_LIMIT;
    }

    derivative = error - gPreviousLineError;
    gPreviousLineError = (int16_t)error;

    correction =
        (error * LINE_PID_KP_NUM +
         gLineIntegral * LINE_PID_KI_NUM +
         derivative * LINE_PID_KD_NUM) /
        LINE_PID_SCALE;
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
        gRampedBasePermille += STRAIGHT_RAMP_UP_STEP_PER_10MS;
        if (gRampedBasePermille > gTargetBasePermille) {
            gRampedBasePermille = gTargetBasePermille;
        }
    } else if (gRampedBasePermille > gTargetBasePermille) {
        gRampedBasePermille -= STRAIGHT_RAMP_DOWN_STEP_PER_10MS;
        if (gRampedBasePermille < gTargetBasePermille) {
            gRampedBasePermille = gTargetBasePermille;
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
    gEncoderSynchronizationEnabled = true;
    Encoder_getSnapshot(&gPrevious);
}

void StraightControl_start(int32_t targetBasePermille)
{
    gActive = true;
    gEncoderFault = false;
    gRampedBasePermille = 0;
    gTargetBasePermille = targetBasePermille;
    gEncoderFaultTicks = 0U;
    gLineIntegral = 0;
    gPreviousLineError = 0;
    gEncoderSynchronizationEnabled = true;
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
    int32_t leftOutput;
    int32_t rightOutput;

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

    updateRamp();

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

    positionError = (int32_t)leftPosition - (int32_t)rightPosition;
    speedError = (int32_t)leftDelta - (int32_t)rightDelta;
    if (gEncoderSynchronizationEnabled) {
        encoderCorrection =
            positionError * STRAIGHT_KP_POSITION +
            speedError * STRAIGHT_KP_SPEED;
        encoderCorrection = clampCorrection(encoderCorrection);
    } else {
        /*
         * A complete lap requires different inner/outer wheel distances in
         * both semicircles. Equal-count correction would oppose line PID.
         * Encoder pulses are still checked for a stalled wheel.
         */
        encoderCorrection = 0;
    }
    lineCorrection = calculateLinePid();
    correction = clampDriveCorrection(encoderCorrection + lineCorrection);

    leftOutput = clampOutput(gRampedBasePermille - correction);
    rightOutput = clampOutput(gRampedBasePermille + correction);
    Motor_driveForwardPermille(leftOutput, rightOutput);
}

void StraightControl_stop(void)
{
    gActive = false;
    gRampedBasePermille = 0;
    gTargetBasePermille = 0;
    gLineIntegral = 0;
    gPreviousLineError = 0;
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

bool StraightControl_hasValidLine(void)
{
    return GraySensor_hasLine();
}

int16_t StraightControl_getLineError(void)
{
    return GraySensor_getLineError();
}

#include "straight_control.h"

#include <math.h>

#include "app_config.h"
#include "encoder.h"
#include "gyro.h"
#include "line_sensor.h"
#include "motor.h"

#define PI_F (3.14159265f)

volatile uint32_t gStraightLeftPulses;
volatile uint32_t gStraightRightPulses;
volatile uint32_t gStraightTargetPulses;
volatile int16_t gStraightCorrection;
volatile uint8_t gStraightRawGray;
volatile float gStraightTargetYawDebug;
volatile uint8_t gStraightEndpointEligibleDebug;
volatile int8_t gCurveYawSignDebug;
volatile float gCurveYawErrorDebug;

static uint32_t gElapsedMs;
static uint16_t gBlackMs;
static uint16_t gWhiteMs;
static bool gEndpointArmed;
static uint8_t gControlDivider;
static uint8_t gLineDivider;
static int32_t gEncoderIntegral;
static float gTargetYaw;
static bool gUseAbsoluteYaw;
static float gFilteredYawError;
static int32_t gAppliedCorrection;
static int8_t gCurveDirection;
static int8_t gCurveRawToCourseSign;
static float gCurveStartRawYaw;
static bool gUseQ1Speed;

static int32_t StraightControl_baseDuty(void)
{
    uint32_t average =
        (gStraightLeftPulses + gStraightRightPulses) / 2U;
    int32_t targetDuty;

    targetDuty = (average * 100U >=
        gStraightTargetPulses * STRAIGHT_APPROACH_PERCENT)
        ? ((gCurveDirection != 0) ? CURVE_APPROACH_PERMILLE :
           (gUseQ1Speed ? Q1_STRAIGHT_APPROACH_PERMILLE :
                          STRAIGHT_APPROACH_PERMILLE))
        : ((gCurveDirection != 0) ? CURVE_BASE_PERMILLE :
           (gUseQ1Speed ? Q1_STRAIGHT_BASE_PERMILLE :
                          STRAIGHT_BASE_PERMILLE));
    if (gElapsedMs >= STRAIGHT_RAMP_TIME_MS) {
        return targetDuty;
    }
    return STRAIGHT_START_PERMILLE +
        ((targetDuty - STRAIGHT_START_PERMILLE) *
         (int32_t) gElapsedMs) / (int32_t) STRAIGHT_RAMP_TIME_MS;
}

static int32_t clamp32(int32_t value, int32_t low, int32_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

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

static float transitionAngle(uint32_t averagePulses)
{
    static const float arcFraction[17] = {
        0.00000000f, 0.02765246f, 0.06811548f, 0.12072311f,
        0.18378977f, 0.25534617f, 0.33331761f, 0.41558310f,
        0.50000000f, 0.58441690f, 0.66668239f, 0.74465383f,
        0.81621023f, 0.87927689f, 0.93188452f, 0.97234754f,
        1.00000000f
    };
    float distanceFraction = (gStraightTargetPulses == 0U) ? 1.0f :
        (float)averagePulses / (float)gStraightTargetPulses;
    float t;
    float u;
    float dx;
    float dy;
    float angle;
    uint8_t upper;

    if (distanceFraction >= 1.0f) {
        t = 1.0f;
    } else {
        /* Invert a 16-segment Bezier arc-length table. Encoder progress is
         * distance, whereas the cubic parameter t is not distance-linear. */
        upper = 1U;
        while ((upper < 16U) &&
               (distanceFraction > arcFraction[upper])) {
            upper++;
        }
        t = ((float)(upper - 1U) +
            (distanceFraction - arcFraction[upper - 1U]) /
            (arcFraction[upper] - arcFraction[upper - 1U])) / 16.0f;
    }
    u = 1.0f - t;
    dx = 3.0f * (u * u * TRANSITION_TANGENT_MM +
        2.0f * u * t *
            (TRANSITION_WIDTH_MM - 2.0f * TRANSITION_TANGENT_MM) +
        t * t * TRANSITION_TANGENT_MM);
    dy = 6.0f * TRANSITION_HEIGHT_MM * u * t;
    angle = atan2f(dy, dx) * (180.0f / PI_F);
    return angle;
}

static void StraightControl_startCommon(float distanceMm,
                                        bool useAbsoluteYaw,
                                        float targetYawDeg)
{
    float circumference = PI_F * WHEEL_DIAMETER_MM;

    gStraightTargetPulses = (uint32_t)
        ((distanceMm * ENCODER_PULSES_PER_REV / circumference) + 0.5f);
    gStraightLeftPulses = 0U;
    gStraightRightPulses = 0U;
    gStraightCorrection = 0;
    gStraightRawGray = 0xFFU;
    gStraightEndpointEligibleDebug = 0U;
    gElapsedMs = 0U;
    gBlackMs = 0U;
    gWhiteMs = 0U;
    gEndpointArmed = false;
    gControlDivider = 0U;
    gLineDivider = 0U;
    gEncoderIntegral = 0;
    gFilteredYawError = 0.0f;
    gAppliedCorrection = 0;
    gCurveDirection = 0;
    gCurveRawToCourseSign = 0;
    gCurveStartRawYaw = 0.0f;
    gUseQ1Speed = false;
    gCurveYawSignDebug = 0;
    gCurveYawErrorDebug = 0.0f;
    gUseAbsoluteYaw = useAbsoluteYaw;
    if (useAbsoluteYaw) {
        gTargetYaw = normalize360(targetYawDeg);
    } else {
        gTargetYaw = (Gyro_isOnline() && Gyro_hasYaw()) ?
            Gyro_getYawDeg() : 0.0f;
    }
    gStraightTargetYawDebug = gTargetYaw;
    Encoder_reset();
    Motor_enable();
    Motor_setForward(STRAIGHT_START_PERMILLE, STRAIGHT_START_PERMILLE);
}

void StraightControl_start(float distanceMm)
{
    StraightControl_startCommon(distanceMm, false, 0.0f);
}

void StraightControl_startQ1Test(float distanceMm)
{
    StraightControl_startCommon(distanceMm, false, 0.0f);
    gUseQ1Speed = true;
}

void StraightControl_startAbsolute(float distanceMm, float targetYawDeg)
{
    StraightControl_startCommon(distanceMm, true, targetYawDeg);
}

void StraightControl_startTransition(TransitionCurve_Direction direction)
{
    StraightControl_startCommon(TRANSITION_CURVE_LENGTH_MM,
        false, 0.0f);
    gCurveDirection = (int8_t)direction;
    gCurveStartRawYaw = Gyro_hasYaw() ? Gyro_getYawDeg() : 0.0f;
    gCurveRawToCourseSign = 0;
    gCurveYawSignDebug = 0;
    gCurveYawErrorDebug = 0.0f;
    /* For a curve this debug value is relative to its entry tangent. */
    gStraightTargetYawDebug = 0.0f;
}

void StraightControl_stop(void)
{
    Motor_brakeAll();
}

Straight_Result StraightControl_task1ms(void)
{
    uint32_t leftTotal;
    uint32_t rightTotal;
    uint32_t average;
    uint32_t endpointArmPercent;
    bool endpointDistanceEligible;

    gElapsedMs++;
    Encoder_getTotal(&leftTotal, &rightTotal);
    gStraightLeftPulses = leftTotal;
    gStraightRightPulses = rightTotal;
    average = (gStraightLeftPulses + gStraightRightPulses) / 2U;
    endpointArmPercent = (gCurveDirection != 0) ?
        CURVE_ENDPOINT_ARM_PERCENT : STRAIGHT_ENDPOINT_ARM_PERCENT;
    endpointDistanceEligible =
        (average * 100U >= gStraightTargetPulses * endpointArmPercent);
    gStraightEndpointEligibleDebug = endpointDistanceEligible ? 1U : 0U;
    if (average >= gStraightTargetPulses) {
        StraightControl_stop();
        return STRAIGHT_REACHED;
    }
    if (gElapsedMs >= STRAIGHT_TIMEOUT_MS) {
        StraightControl_stop();
        return STRAIGHT_TIMEOUT;
    }

    if (++gLineDivider >= LINE_READ_PERIOD_MS) {
        LineSensor_Frame frame;
        gLineDivider = 0U;
        if (LineSensor_read(&frame)) {
            gStraightRawGray = frame.rawBits;
            if (!gEndpointArmed) {
                if (frame.blackCount <= ENDPOINT_WHITE_MAX) {
                    gWhiteMs += LINE_READ_PERIOD_MS;
                    if (gWhiteMs >= ENDPOINT_WHITE_CONFIRM_MS) {
                        gEndpointArmed = true;
                    }
                } else {
                    gWhiteMs = 0U;
                }
            } else if (endpointDistanceEligible) {
                if (frame.blackCount >= ENDPOINT_BLACK_MIN) {
                    gBlackMs += LINE_READ_PERIOD_MS;
                    if (gBlackMs >= ENDPOINT_CONFIRM_MS) {
                        StraightControl_stop();
                        return STRAIGHT_REACHED;
                    }
                } else {
                    gBlackMs = 0U;
                }
            } else {
                /* Ignore the starting circle/line until enough physical
                 * distance has been covered to make the opposite endpoint
                 * geometrically possible. */
                gBlackMs = 0U;
            }
        }
    }

    if (++gControlDivider >= STRAIGHT_CONTROL_PERIOD_MS) {
        uint32_t leftDelta;
        uint32_t rightDelta;
        int32_t encoderError;
        int32_t correction;
        int32_t baseDuty;
        int32_t leftDuty;
        int32_t rightDuty;

        gControlDivider = 0U;
        Encoder_sample(&leftDelta, &rightDelta);
        if (gCurveDirection != 0) {
            /* A curve intentionally commands unequal wheel speeds. Encoder
             * totals still parameterize progress, but equal-speed PI would
             * fight the yaw trajectory and can reverse the intended turn. */
            gEncoderIntegral = 0;
            correction = 0;
        } else {
            encoderError = (int32_t)leftDelta - (int32_t)rightDelta;
            gEncoderIntegral = clamp32(gEncoderIntegral + encoderError,
                -ENCODER_I_LIMIT, ENCODER_I_LIMIT);
            correction = ENCODER_KP * encoderError +
                ENCODER_KI * gEncoderIntegral;
        }

        if (Gyro_isOnline() && Gyro_hasYaw()) {
            float yaw = Gyro_getYawDeg();
            float rate = Gyro_hasRate() ? Gyro_getRateDps() : 0.0f;
            float yawError;

            if (gCurveDirection != 0) {
                float rawDelta = wrap180(yaw - gCurveStartRawYaw);
                float targetRelative = (float)gCurveDirection *
                    transitionAngle(average);

                gStraightTargetYawDebug = targetRelative;
                if ((gCurveRawToCourseSign == 0) &&
                    (fabsf(rawDelta) >= CURVE_SIGN_DETECT_DEG)) {
                    int8_t rawSign = (rawDelta > 0.0f) ? 1 : -1;
                    gCurveRawToCourseSign = gCurveDirection * rawSign;
                    gCurveYawSignDebug = gCurveRawToCourseSign;
                }
                if (gCurveRawToCourseSign == 0) {
                    /* Known physical steering command used only long enough
                     * to learn the installed gyro's yaw polarity. Negative
                     * correction turns right; positive turns left. */
                    correction = (gCurveDirection > 0) ?
                        -CURVE_SIGN_DETECT_CORRECTION :
                         CURVE_SIGN_DETECT_CORRECTION;
                    gFilteredYawError = 0.0f;
                    gCurveYawErrorDebug = 0.0f;
                    goto apply_correction;
                }
                yawError = (float)gCurveRawToCourseSign * rawDelta -
                    targetRelative;
                gCurveYawErrorDebug = yawError;
                rate *= (float)gCurveRawToCourseSign;
            } else {
                if (gUseAbsoluteYaw) {
                yaw = normalize360(GYRO_ABSOLUTE_YAW_SIGN *
                    Gyro_getCourseYawDeg());
                rate *= GYRO_ABSOLUTE_YAW_SIGN;
                }
                yawError = wrap180(yaw - gTargetYaw);
            }
            gFilteredYawError = 0.80f * gFilteredYawError + 0.20f * yawError;
            correction += GYRO_STEER_SIGN * (int32_t)
                (((gCurveDirection != 0) ? CURVE_HEADING_KP :
                                           GYRO_HEADING_KP) *
                 gFilteredYawError + GYRO_RATE_KD * rate);
        }
apply_correction:
        correction = clamp32(correction,
            (gCurveDirection != 0) ? -CURVE_CORRECTION_LIMIT :
                                     -STRAIGHT_CORRECTION_LIMIT,
            (gCurveDirection != 0) ? CURVE_CORRECTION_LIMIT :
                                     STRAIGHT_CORRECTION_LIMIT);
        {
            int32_t slew = (gCurveDirection != 0) ?
                CURVE_CORRECTION_SLEW : STRAIGHT_CORRECTION_SLEW;
        if (correction > gAppliedCorrection + slew) {
            gAppliedCorrection += slew;
        } else if (correction <
                   gAppliedCorrection - slew) {
            gAppliedCorrection -= slew;
        } else {
            gAppliedCorrection = correction;
        }
        }
        gStraightCorrection = (int16_t) gAppliedCorrection;
        baseDuty = StraightControl_baseDuty();
        leftDuty = clamp32(baseDuty - gAppliedCorrection,
            0, STRAIGHT_MAX_PERMILLE);
        rightDuty = clamp32(baseDuty + gAppliedCorrection,
            0, STRAIGHT_MAX_PERMILLE);
        Motor_setForward((uint16_t) leftDuty, (uint16_t) rightDuty);
    }
    return STRAIGHT_RUNNING;
}

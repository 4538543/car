#include "arc_interface.h"

#include <math.h>
#include <stdlib.h>

#include "app_config.h"
#include "encoder.h"
#include "gyro.h"
#include "line_sensor.h"
#include "motor.h"

#define PI_F (3.14159265f)

static Arc_Request gRequest;
static Arc_Status gStatus;
static bool gComplete;
static bool gSteepEntry;
static bool gSkipExitAlign;
static bool gAlignCaptured;
static bool gGrayOnlyTest;
static bool gYawAssist;
static int8_t gDirection;
static uint32_t gStartAveragePulses;
static uint32_t gFinishArmPulses;
static uint32_t gArcTargetPulses;
static uint16_t gElapsedMs;
static uint16_t gStateElapsedMs;
static uint16_t gAcquireStableMs;
static uint16_t gLostMs;
static uint16_t gAlignStableMs;
static uint8_t gPeriodMs;
static int16_t gLastLineError;
static int8_t gLastSearchSign;
static int32_t gLineIntegral;
static int32_t gFilteredDerivative;
static int32_t gAppliedCorrection;
static float gLastYaw;
static float gLastAlignError;
static float gLastHeadingError;

volatile unsigned int gArcRequestDebug;
volatile uint8_t gArcStatusDebug;
volatile uint8_t gArcFaultDebug;
volatile uint8_t gArcRawBitsDebug;
volatile int16_t gArcLineErrorDebug;
volatile int16_t gArcCorrectionDebug;
volatile int16_t gArcHeadingCorrectionDebug;
volatile uint16_t gArcHeadingBlendDebug;
volatile float gArcYawDebug;
volatile float gArcTargetYawDebug;
volatile float gArcAccumulatedYawDebug;
volatile uint32_t gArcSegmentPulsesDebug;

static int32_t clamp32(int32_t value, int32_t low, int32_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static float normalize360(float angle)
{
    while (angle >= 360.0f) angle -= 360.0f;
    while (angle < 0.0f) angle += 360.0f;
    return angle;
}

static float wrap180(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static float absoluteYaw(void)
{
    return normalize360(GYRO_ABSOLUTE_YAW_SIGN *
        Gyro_getCourseYawDeg());
}

static void setForward(int32_t left, int32_t right)
{
    left = clamp32(left, 0, ARC_MAX_PERMILLE);
    right = clamp32(right, 0, ARC_MAX_PERMILLE);
    Motor_setForward((uint16_t)left, (uint16_t)right);
}

static void setFault(uint8_t code)
{
    gArcFaultDebug = code;
    gStatus = ARC_STATUS_FAULT;
    gArcStatusDebug = (uint8_t)gStatus;
    Motor_brakeAll();
}

static void updateYaw(void)
{
    float yaw = absoluteYaw();
    gArcAccumulatedYawDebug += wrap180(yaw - gLastYaw);
    gLastYaw = yaw;
    gArcYawDebug = yaw;
}

static void trackLine(const LineSensor_Frame *frame)
{
    int16_t error = frame->error;
    int32_t errorStep;
    int32_t rawDerivative;
    int32_t lineCorrection;
    int32_t headingCorrection = 0;
    int32_t captureCorrection;
    int32_t correction;
    int32_t base;
    uint32_t entryBlend;
    uint32_t headingBlend = 0U;
    float progress = (float)gDirection * gArcAccumulatedYawDebug;
    float headingError = wrap180(gArcTargetYawDebug - absoluteYaw());

    if ((error >= -ARC_ERROR_DEADBAND) && (error <= ARC_ERROR_DEADBAND)) {
        error = 0;
    } else {
        gLastSearchSign = (error > 0) ? 1 : -1;
    }

    /* The eight discrete probes can make the weighted error jump by roughly
     * one full sensor spacing in a single 5 ms sample. Limit that one-sample
     * step so P/D cannot command an ejecting snap turn from a noisy frame. */
    errorStep = (int32_t)error - (int32_t)gLastLineError;
    errorStep = clamp32(errorStep,
        -ARC_ERROR_STEP_LIMIT, ARC_ERROR_STEP_LIMIT);
    error = (int16_t)((int32_t)gLastLineError + errorStep);
    rawDerivative = (int32_t)error - (int32_t)gLastLineError;
    gFilteredDerivative =
        (ARC_D_FILTER_OLD_WEIGHT * gFilteredDerivative + rawDerivative) /
        ARC_D_FILTER_DIVISOR;
    gLineIntegral = clamp32(gLineIntegral + error, -ARC_I_LIMIT, ARC_I_LIMIT);
    lineCorrection = (ARC_GRAY_KP * (int32_t)error +
                      ARC_GRAY_KI * gLineIntegral +
                      ARC_GRAY_KD * gFilteredDerivative) / ARC_GAIN_SCALE;
    lineCorrection += (int32_t)gDirection * ARC_FEEDFORWARD;
    lineCorrection = clamp32(lineCorrection,
        -ARC_TRACK_TURN_LIMIT, ARC_TRACK_TURN_LIMIT) * GYRO_STEER_SIGN;

    /* Only introduce final absolute-heading control near the end of the
     * semicircle. It is deliberately limited so gray position tracking keeps
     * priority while the line is still visible. */
    if (gYawAssist && Gyro_isOnline() &&
        (progress > ARC_HEADING_BLEND_START_DEG)) {
        float span = ARC_HEADING_BLEND_FULL_DEG -
            ARC_HEADING_BLEND_START_DEG;
        float progressed = progress - ARC_HEADING_BLEND_START_DEG;
        headingBlend = (progressed >= span) ? 1000U :
            (uint32_t)(1000.0f * progressed / span);
        headingCorrection = (int32_t)(
            ARC_EXIT_YAW_KP * headingError +
            ARC_EXIT_YAW_KD * (headingError - gLastHeadingError));
        headingCorrection = clamp32(headingCorrection,
            -ARC_EXIT_YAW_LIMIT, ARC_EXIT_YAW_LIMIT);
        headingCorrection = headingCorrection * (int32_t)headingBlend / 1000;
    }
    gLastHeadingError = headingError;

    /* A diagonal reaches the arc with a sizeable tangent-angle error.
     * Begin with a deliberate low-speed turn toward the known arc direction,
     * then hand authority to the gray PID. Starting near zero correction here
     * would let the car drive straight across and lose the curve. */
    entryBlend = (gElapsedMs >= ARC_ENTRY_BLEND_MS) ?
        ARC_ENTRY_BLEND_MS : gElapsedMs;
    /* Every arc starts slowly after the endpoint brake, then ramps to normal
     * tracking speed. A diagonal entry additionally receives a known capture
     * turn so it cannot drive straight across the curve. */
    base = ARC_ENTRY_START_PERMILLE +
        ((ARC_BASE_PERMILLE - ARC_ENTRY_START_PERMILLE) *
         (int32_t)entryBlend) / (int32_t)ARC_ENTRY_BLEND_MS;
    if (gSteepEntry) {
        captureCorrection = (int32_t)gDirection *
            ARC_ENTRY_CAPTURE_TURN * GYRO_STEER_SIGN;
        lineCorrection =
            (captureCorrection *
             ((int32_t)ARC_ENTRY_BLEND_MS - (int32_t)entryBlend) +
             lineCorrection * (int32_t)entryBlend) /
            (int32_t)ARC_ENTRY_BLEND_MS;
    } else {
        captureCorrection = 0;
    }

    /* Slow down when the line is near an outer probe instead of asking the
     * saturated PID to recover while still travelling at full arc speed. */
    if (abs(error) > ARC_LARGE_ERROR_START) {
        int32_t excess = abs(error) - ARC_LARGE_ERROR_START;
        int32_t range = 3500 - ARC_LARGE_ERROR_START;
        int32_t slowdown = ARC_LARGE_ERROR_SLOWDOWN * excess / range;
        if (slowdown > ARC_LARGE_ERROR_SLOWDOWN) {
            slowdown = ARC_LARGE_ERROR_SLOWDOWN;
        }
        base -= slowdown;
    }

    /* Before leaving the semicircle, reduce speed progressively so gray PID
     * and the absolute-heading blend can settle on the true exit tangent. */
    if (progress > ARC_EXIT_SLOW_START_DEG) {
        float span = ARC_EXIT_SLOW_FULL_DEG - ARC_EXIT_SLOW_START_DEG;
        float fraction = (progress - ARC_EXIT_SLOW_START_DEG) / span;
        int32_t exitBase;
        if (fraction > 1.0f) fraction = 1.0f;
        exitBase = ARC_BASE_PERMILLE - (int32_t)(
            (ARC_BASE_PERMILLE - ARC_EXIT_APPROACH_PERMILLE) * fraction);
        if (base > exitBase) base = exitBase;
    }

    correction = clamp32(lineCorrection + headingCorrection,
        -ARC_TRACK_TURN_LIMIT, ARC_TRACK_TURN_LIMIT);
    {
        int32_t slew = ARC_CORRECTION_SLEW;
        /* Build steering force smoothly, but release an obsolete correction
         * quickly. A symmetric slow slew keeps turning the old way for tens
         * of milliseconds after the sensor has crossed the line. */
        if (((correction > 0) && (gAppliedCorrection < 0)) ||
            ((correction < 0) && (gAppliedCorrection > 0)) ||
            (abs(correction) < abs(gAppliedCorrection))) {
            slew = ARC_CORRECTION_RELEASE_SLEW;
        }
    if (correction > gAppliedCorrection + slew) {
        gAppliedCorrection += slew;
    } else if (correction < gAppliedCorrection - slew) {
        gAppliedCorrection -= slew;
    } else {
        gAppliedCorrection = correction;
    }
    }
    correction = gAppliedCorrection;

    /* Positive sensor error means line right: left wheel must be faster. */
    setForward(base + correction, base - correction);
    gLastLineError = error;
    gArcCorrectionDebug = (int16_t)correction;
    gArcHeadingCorrectionDebug = (int16_t)headingCorrection;
    gArcHeadingBlendDebug = (uint16_t)headingBlend;
}

static void recoverLine(void)
{
    int32_t correction;

    /* During entry, keep turning into the written-in arc direction even if
     * the steep approach briefly moves all probes off the black line. */
    if (gSteepEntry && (gElapsedMs < ARC_ENTRY_BLEND_MS)) {
        correction = (int32_t)gDirection *
            ARC_ENTRY_CAPTURE_TURN * GYRO_STEER_SIGN;
        setForward(ARC_ENTRY_START_PERMILLE + correction,
                   ARC_ENTRY_START_PERMILLE - correction);
        gArcCorrectionDebug = (int16_t)correction;
        return;
    }

    correction = (int32_t)gLastSearchSign * ARC_RECOVERY_TURN;
    correction += (int32_t)gDirection * ARC_FEEDFORWARD;
    correction = clamp32(correction, -ARC_TURN_LIMIT, ARC_TURN_LIMIT);
    correction *= GYRO_STEER_SIGN;

    /* If the forward search has not seen black for a while, continuing to
     * advance only carries the sensor farther away. Pivot in the remembered
     * search direction until any probe sees the line again; trackingStep()
     * then returns to normal PID automatically. */
    if (gLostMs >= ARC_LOST_TIMEOUT_MS) {
        int16_t pivot = (correction >= 0) ?
            (int16_t)ARC_RECOVERY_PIVOT_PERMILLE :
            (int16_t)-ARC_RECOVERY_PIVOT_PERMILLE;
        Motor_setSigned(pivot, (int16_t)-pivot);
        gArcCorrectionDebug = pivot;
        return;
    }
    setForward(ARC_RECOVERY_PERMILLE + correction,
               ARC_RECOVERY_PERMILLE - correction);
    gArcCorrectionDebug = (int16_t)correction;
}

static void beginAlign(void)
{
    float error = wrap180(gArcTargetYawDebug - absoluteYaw());

    /* Q3 C->B immediately turns to the absolute 143-degree diagonal. Holding
     * 180 degrees first would be redundant and can create a visible twitch. */
    if (gSkipExitAlign) {
        Motor_brakeAll();
        gComplete = true;
        gStatus = ARC_STATUS_COMPLETE;
        gArcStatusDebug = (uint8_t)gStatus;
        return;
    }

    /* Normal case: late-arc fusion already delivered the required tangent. */
    if (fabsf(error) <= ARC_DIRECT_EXIT_TOLERANCE_DEG) {
        Motor_brakeAll();
        gComplete = true;
        gStatus = ARC_STATUS_COMPLETE;
        gArcStatusDebug = (uint8_t)gStatus;
        return;
    }

    /* Fallback: correct a large residual error around the axle centre. */
    gStatus = ARC_STATUS_ALIGNING;
    gStateElapsedMs = 0U;
    gAlignStableMs = 0U;
    gAlignCaptured = false;
    gLastAlignError = wrap180(gArcTargetYawDebug - absoluteYaw());
    gArcStatusDebug = (uint8_t)gStatus;
}

static void alignStep(void)
{
    float yaw = absoluteYaw();
    float error = wrap180(gArcTargetYawDebug - yaw);
    float derivative = error - gLastAlignError;
    int32_t correction = (int32_t)
        (ARC_ALIGN_KP * error + ARC_ALIGN_KD * derivative);

    correction = clamp32(correction,
        -ARC_PIVOT_MAX_PERMILLE, ARC_PIVOT_MAX_PERMILLE);
    gLastAlignError = error;
    gArcYawDebug = yaw;
    gArcCorrectionDebug = (int16_t)correction;

    if (fabsf(error) <= ARC_ALIGN_TOLERANCE_DEG) {
        gAlignCaptured = true;
        Motor_brakeAll();
    } else if (gAlignCaptured &&
               (fabsf(error) < ARC_ALIGN_RESTART_ERROR_DEG)) {
        /* Do not reverse for a small inertial overshoot around the target. */
        Motor_brakeAll();
    } else {
        int32_t duty;
        float magnitude = fabsf(error);

        gAlignCaptured = false;
        if (magnitude >= ARC_PIVOT_SLOW_ZONE_DEG) {
            duty = ARC_PIVOT_MAX_PERMILLE;
        } else {
            duty = ARC_PIVOT_MIN_PERMILLE + (int32_t)(
                (ARC_PIVOT_MAX_PERMILLE - ARC_PIVOT_MIN_PERMILLE) *
                magnitude / ARC_PIVOT_SLOW_ZONE_DEG);
        }
        /* Same steering polarity as the old forward aligner, but equal and
         * opposite wheel commands remove almost all translational drift. */
        if (error >= 0.0f) {
            Motor_setSigned((int16_t)duty, (int16_t)-duty);
        } else {
            Motor_setSigned((int16_t)-duty, (int16_t)duty);
        }
    }
    if (gAlignCaptured &&
        ((!Gyro_hasRate()) ||
         (fabsf(Gyro_getRateDps()) <= ABS_TURN_RATE_TOLERANCE_DPS))) {
        gAlignStableMs += ARC_CONTROL_PERIOD_MS;
    } else {
        gAlignStableMs = 0U;
    }
    if (gAlignStableMs >= ARC_ALIGN_STABLE_MS) {
        Motor_brakeAll();
        gComplete = true;
        gStatus = ARC_STATUS_COMPLETE;
        gArcStatusDebug = (uint8_t)gStatus;
    }
}

static void trackingStep(void)
{
    LineSensor_Frame frame;
    uint32_t leftTotal;
    uint32_t rightTotal;
    uint32_t average;
    bool finishArmed;

    if (gYawAssist && Gyro_isOnline()) updateYaw();
    Encoder_getTotal(&leftTotal, &rightTotal);
    average = (leftTotal + rightTotal) / 2U;
    gArcSegmentPulsesDebug = average - gStartAveragePulses;
    /* A temporary UART gap must not disable the back-half speed schedule.
     * Encoder distance supplies a bounded fallback progress estimate. */
    if ((gGrayOnlyTest || (!gYawAssist) || (!Gyro_isOnline())) &&
        (gArcTargetPulses != 0U)) {
        float encoderProgress = 180.0f *
            (float)gArcSegmentPulsesDebug / (float)gArcTargetPulses;
        float currentProgress = (float)gDirection *
            gArcAccumulatedYawDebug;
        if (encoderProgress > 180.0f) encoderProgress = 180.0f;
        if (encoderProgress > currentProgress) {
            gArcAccumulatedYawDebug =
                (float)gDirection * encoderProgress;
        }
    }
    finishArmed = (gArcSegmentPulsesDebug >= gFinishArmPulses) &&
        (gGrayOnlyTest ||
         (((float)gDirection * gArcAccumulatedYawDebug) >=
          ARC_FINISH_YAW_DEG));

    if (!LineSensor_read(&frame)) {
        setFault(3U);
        return;
    }
    gArcRawBitsDebug = frame.rawBits;
    gArcLineErrorDebug = frame.error;

    if (frame.lineFound) {
        bool wasLost = (gLostMs != 0U);
        gLostMs = 0U;
        if (wasLost) {
            gLastLineError = frame.error;
            gFilteredDerivative = 0;
            gAppliedCorrection = (int32_t)gDirection * ARC_FEEDFORWARD *
                GYRO_STEER_SIGN;
        }
        if (gStatus == ARC_STATUS_ACQUIRING) {
            gAcquireStableMs += ARC_CONTROL_PERIOD_MS;
            if (gAcquireStableMs >= ARC_ACQUIRE_CONFIRM_MS) {
                gStatus = ARC_STATUS_TRACKING;
                gArcStatusDebug = (uint8_t)gStatus;
                gLineIntegral = 0;
                gLastLineError = frame.error;
                gFilteredDerivative = 0;
            }
        }
        trackLine(&frame);
    } else {
        gAcquireStableMs = 0U;
        gLostMs += ARC_CONTROL_PERIOD_MS;
        if ((gStatus == ARC_STATUS_TRACKING) && finishArmed &&
            (gLostMs >= ARC_END_CONFIRM_MS)) {
            beginAlign();
        } else if ((gStatus == ARC_STATUS_TRACKING) && finishArmed) {
            /* Do not drive beyond the geometrical endpoint while confirming
             * that the line has really ended. */
            Motor_brakeAll();
        } else {
            recoverLine();
        }
    }
}

void ArcInterface_init(void)
{
    gRequest = ARC_NONE;
    gStatus = ARC_STATUS_IDLE;
    gComplete = false;
    gSteepEntry = false;
    gSkipExitAlign = false;
    gAlignCaptured = false;
    gGrayOnlyTest = false;
    gYawAssist = false;
    gArcRequestDebug = ARC_NONE;
    gArcStatusDebug = ARC_STATUS_IDLE;
    gArcFaultDebug = 0U;
    gArcRawBitsDebug = 0xFFU;
    gArcLineErrorDebug = 0;
    gArcCorrectionDebug = 0;
    gArcHeadingCorrectionDebug = 0;
    gArcHeadingBlendDebug = 0U;
    gArcYawDebug = 0.0f;
    gArcTargetYawDebug = 0.0f;
    gArcAccumulatedYawDebug = 0.0f;
    gArcSegmentPulsesDebug = 0U;
}

static void requestArc(Arc_Request request, bool steepEntry,
                       bool skipExitAlign, bool relativeExitTarget,
                       bool grayOnlyTest)
{
    uint32_t leftTotal;
    uint32_t rightTotal;
    float arcLengthMm = PI_F * ARC_RADIUS_MM;
    float targetPulses = arcLengthMm * ENCODER_PULSES_PER_REV /
        (PI_F * WHEEL_DIAMETER_MM);

    gRequest = request;
    gComplete = false;
    gSteepEntry = steepEntry;
    gSkipExitAlign = skipExitAlign;
    gGrayOnlyTest = grayOnlyTest;
    gArcRequestDebug = (unsigned int)request;
    gArcFaultDebug = 0U;
    gElapsedMs = 0U;
    gStateElapsedMs = 0U;
    gAcquireStableMs = 0U;
    gLostMs = 0U;
    gAlignStableMs = 0U;
    gPeriodMs = 0U;
    gLastLineError = 0;
    gLastSearchSign = 0;
    gLineIntegral = 0;
    gFilteredDerivative = 0;
    gAppliedCorrection = 0;
    gLastHeadingError = 0.0f;
    gArcAccumulatedYawDebug = 0.0f;
    gArcSegmentPulsesDebug = 0U;
    gFinishArmPulses = (uint32_t)(targetPulses *
        (grayOnlyTest ? 95.0f : ARC_FINISH_DISTANCE_PERCENT) / 100.0f);
    gArcTargetPulses = (uint32_t)targetPulses;

    /* A short UART age excursion must not cancel a gray-tracking arc at the
     * exact straight/arc handoff. At least one valid yaw frame is required;
     * tracking may continue from the latest yaw while gray PID owns steering. */
    if (((!grayOnlyTest) && (!Gyro_hasYaw())) ||
        (request == ARC_NONE)) {
        setFault(1U);
        return;
    }
    gDirection = (request == ARC_RIGHT_C_TO_B) ? -1 : 1;
    /* PB22 uses yaw assistance when a valid course zero was captured, but
     * remains able to run gray/encoder-only for bench diagnosis. */
    gYawAssist = (!grayOnlyTest) || Gyro_hasYaw();
    gAppliedCorrection = (int32_t)gDirection * ARC_FEEDFORWARD *
        GYRO_STEER_SIGN;
    gLastYaw = Gyro_hasYaw() ? absoluteYaw() : 0.0f;
    gLastSearchSign = gDirection;
    if (relativeExitTarget) {
        gArcTargetYawDebug = normalize360(gLastYaw +
            (float)gDirection * 180.0f);
    } else {
        gArcTargetYawDebug = (request == ARC_LEFT_D_TO_A) ?
            ARC_EXIT_HEADING_0_DEG : ARC_EXIT_HEADING_180_DEG;
    }
    gArcYawDebug = gLastYaw;
    gLastHeadingError = wrap180(gArcTargetYawDebug - gLastYaw);
    Encoder_getTotal(&leftTotal, &rightTotal);
    gStartAveragePulses = (leftTotal + rightTotal) / 2U;
    gStatus = ARC_STATUS_ACQUIRING;
    gArcStatusDebug = (uint8_t)gStatus;
    Motor_enable();
    if (gSteepEntry) {
        Motor_setForward(ARC_ENTRY_START_PERMILLE,
                         ARC_ENTRY_START_PERMILLE);
    } else {
        Motor_setForward(ARC_BASE_PERMILLE, ARC_BASE_PERMILLE);
    }
}

void ArcInterface_request(Arc_Request request)
{
    requestArc(request, false, false, false, false);
}

void ArcInterface_requestRelativeExit(Arc_Request request)
{
    requestArc(request, false, false, true, false);
}

void ArcInterface_requestNoExitAlign(Arc_Request request)
{
    requestArc(request, false, true, false, false);
}

void ArcInterface_requestFromDiagonal(Arc_Request request)
{
    requestArc(request, true, false, false, false);
}

void ArcInterface_requestFromDiagonalNoExitAlign(Arc_Request request)
{
    requestArc(request, true, true, false, false);
}

void ArcInterface_requestGrayPidTest(Arc_Request request)
{
    requestArc(request, false, true, false, true);
}

void ArcInterface_task1ms(void)
{
    if ((gStatus != ARC_STATUS_ACQUIRING) &&
        (gStatus != ARC_STATUS_TRACKING) &&
        (gStatus != ARC_STATUS_ALIGNING)) return;

    gElapsedMs++;
    gStateElapsedMs++;
    /* Do not brake merely because a UART frame is temporarily late. The arc
     * remains bounded by ARC_TIMEOUT_MS and gray tracking can still recover. */
    if ((!gGrayOnlyTest) && (!Gyro_hasYaw())) {
        setFault(2U);
        return;
    }
    if (gElapsedMs >= ARC_TIMEOUT_MS) { setFault(4U); return; }
    if ((gStatus == ARC_STATUS_ALIGNING) &&
        (gStateElapsedMs >= ARC_ALIGN_TIMEOUT_MS)) { setFault(4U); return; }

    if (++gPeriodMs >= ARC_CONTROL_PERIOD_MS) {
        gPeriodMs = 0U;
        if (gStatus == ARC_STATUS_ALIGNING) alignStep();
        else trackingStep();
    }
}

Arc_Request ArcInterface_getRequest(void) { return gRequest; }
Arc_Status ArcInterface_getStatus(void) { return gStatus; }

void ArcInterface_markComplete(void)
{
    Motor_brakeAll();
    gComplete = true;
    gStatus = ARC_STATUS_COMPLETE;
    gArcStatusDebug = (uint8_t)gStatus;
}

bool ArcInterface_takeComplete(void)
{
    bool result = gComplete;
    if (result) {
        gComplete = false;
        gRequest = ARC_NONE;
        gArcRequestDebug = ARC_NONE;
        gStatus = ARC_STATUS_IDLE;
        gArcStatusDebug = ARC_STATUS_IDLE;
    }
    return result;
}

bool ArcInterface_hasFault(void) { return gStatus == ARC_STATUS_FAULT; }

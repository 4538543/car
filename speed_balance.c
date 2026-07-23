#include "speed_balance.h"

#include <stdint.h>

#include "app_config.h"
#include "encoder.h"
#include "motor.h"

static int32_t gIntegral;

static int32_t SpeedBalance_clamp(int32_t value, int32_t limit)
{
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

void SpeedBalance_reset(void)
{
    gIntegral = 0;
    Encoder_reset();
}

void SpeedBalance_step(void)
{
    uint32_t leftPulses;
    uint32_t rightPulses;
    int32_t error;
    int32_t correction;
    int32_t leftDuty;
    int32_t rightDuty;

    Encoder_sample(&leftPulses, &rightPulses);
    error = (int32_t) leftPulses - (int32_t) rightPulses;
    gIntegral = SpeedBalance_clamp(
        gIntegral + error, SPEED_BALANCE_I_LIMIT);
    correction = SPEED_BALANCE_KP * error + SPEED_BALANCE_KI * gIntegral;
    correction = SpeedBalance_clamp(correction, SPEED_BALANCE_OUT_LIMIT);

    leftDuty = (int32_t) APP_BASE_SPEED_PERMILLE - correction;
    rightDuty = (int32_t) APP_BASE_SPEED_PERMILLE + correction;
    if (leftDuty < 0) {
        leftDuty = 0;
    }
    if (rightDuty < 0) {
        rightDuty = 0;
    }
    if (leftDuty > (int32_t) APP_MAX_SPEED_PERMILLE) {
        leftDuty = APP_MAX_SPEED_PERMILLE;
    }
    if (rightDuty > (int32_t) APP_MAX_SPEED_PERMILLE) {
        rightDuty = APP_MAX_SPEED_PERMILLE;
    }

    Motor_setForward((uint16_t) leftDuty, (uint16_t) rightDuty);
}

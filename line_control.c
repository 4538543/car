#include "line_control.h"

#include "app_config.h"
#include "motor.h"

static int16_t gLastError;

static int32_t LineControl_clamp(int32_t value, int32_t low, int32_t high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

void LineControl_reset(void)
{
    gLastError = 0;
}

int16_t LineControl_step(int16_t lineError)
{
    int32_t derivative = (int32_t) lineError - (int32_t) gLastError;
    int32_t turn = ((int32_t) LINE_KP * lineError) / 1000L +
                   ((int32_t) LINE_KD * derivative) / 1000L;

    gLastError = lineError;
    turn *= APP_STEERING_SIGN;
    turn = LineControl_clamp(turn, -LINE_TURN_LIMIT, LINE_TURN_LIMIT);
    return (int16_t) turn;
}

void LineControl_drive(int16_t turn)
{
    int32_t left = (int32_t) APP_BASE_SPEED_PERMILLE + turn;
    int32_t right = (int32_t) APP_BASE_SPEED_PERMILLE - turn;

    left = LineControl_clamp(left, 0, APP_MAX_SPEED_PERMILLE);
    right = LineControl_clamp(right, 0, APP_MAX_SPEED_PERMILLE);
    Motor_setForward((uint16_t) left, (uint16_t) right);
}

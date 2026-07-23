#include "q3_mission.h"

#include "app_config.h"
#include "gyro.h"
#include "indicator.h"
#include "line_sensor.h"
#include "motor.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    Q3_IDLE,
    Q3_AC_ALIGN_ZERO,
    Q3_AC_TURN_38,
    Q3_AC_DRIVE,
    Q3_WAIT_CB_TRACE,
    Q3_BD_ALIGN_180,
    Q3_BD_TURN_142,
    Q3_BD_DRIVE,
    Q3_REVERSE_BRAKE,
    Q3_DONE,
    Q3_FAULT
} Q3_State;

static Q3_State g_state;
static uint32_t g_stateMs;
static uint32_t g_blackMs;
static uint32_t g_stableMs;
static int16_t g_targetYaw;
static int16_t g_previousError;
static int32_t g_integral;
static bool g_brakeAfterAc;

static int16_t wrapError(int16_t error)
{
    while (error > 1800) {
        error -= 3600;
    }
    while (error < -1800) {
        error += 3600;
    }
    return error;
}

static int16_t absolute16(int16_t value)
{
    return (value < 0) ? (int16_t)-value : value;
}

static void resetPid(void)
{
    g_previousError = 0;
    g_integral = 0;
}

static int16_t pidCorrection(int16_t target, int16_t limit)
{
    int16_t error = wrapError((int16_t)(target - Gyro_yawDeciDeg()));
    int16_t derivative = (int16_t)(error - g_previousError);
    int32_t output;

    g_previousError = error;
    g_integral += error;
    if (g_integral > HEADING_INTEGRAL_LIMIT) {
        g_integral = HEADING_INTEGRAL_LIMIT;
    } else if (g_integral < -HEADING_INTEGRAL_LIMIT) {
        g_integral = -HEADING_INTEGRAL_LIMIT;
    }

    output =
        ((int32_t)error * HEADING_KP_NUM / HEADING_KP_DEN) +
        (g_integral / HEADING_KI_DEN) +
        ((int32_t)derivative * HEADING_KD_NUM);
    if (output > limit) {
        output = limit;
    } else if (output < -limit) {
        output = -limit;
    }
    return (int16_t)output;
}

static void enterState(Q3_State next, int16_t target)
{
    g_state = next;
    g_targetYaw = target;
    g_stateMs = 0U;
    g_blackMs = 0U;
    g_stableMs = 0U;
    resetPid();
}

static void fail(void)
{
    Motor_stop();
    g_state = Q3_FAULT;
    Indicator_fault();
}

static bool updateTurn(int16_t target, Q3_State next)
{
    int16_t error;
    int16_t command;

    if (!Gyro_valid()) {
        fail();
        return false;
    }

    error = wrapError((int16_t)(target - Gyro_yawDeciDeg()));
    if (absolute16(error) <= TURN_TOLERANCE_DECI_DEG) {
        Motor_stop();
        if (++g_stableMs >= TURN_STABLE_MS) {
            enterState(next, target);
            return true;
        }
        return false;
    }

    g_stableMs = 0U;
    command = pidCorrection(target, TURN_SPEED);
    if (command > 0 && command < TURN_MIN_SPEED) {
        command = TURN_MIN_SPEED;
    } else if (command < 0 && command > -(int16_t)TURN_MIN_SPEED) {
        command = -(int16_t)TURN_MIN_SPEED;
    }
    Motor_drive(command, (int16_t)-command);
    return false;
}

static void driveStraight(void)
{
    int16_t correction = pidCorrection(g_targetYaw, HEADING_LIMIT);

    Motor_drive(
        (int16_t)(DRIVE_SPEED + correction),
        (int16_t)(DRIVE_SPEED - correction));
}

static void updateBlackLine(void)
{
    if (g_stateMs > START_GUARD_MS && LineSensor_blackLine()) {
        if (g_blackMs < BLACK_CONFIRM_MS) {
            g_blackMs++;
        }
        if (g_blackMs >= BLACK_CONFIRM_MS) {
            g_brakeAfterAc = (g_state == Q3_AC_DRIVE);
            g_state = Q3_REVERSE_BRAKE;
            g_stateMs = 0U;
            Motor_drive(-(int16_t)BRAKE_REVERSE_SPEED,
                        -(int16_t)BRAKE_REVERSE_SPEED);
        }
    } else {
        g_blackMs = 0U;
    }
}

void Q3Mission_init(void)
{
    g_state = Q3_IDLE;
    g_stateMs = 0U;
    g_blackMs = 0U;
    g_stableMs = 0U;
    g_targetYaw = 0;
    g_brakeAfterAc = false;
    resetPid();
}

void Q3Mission_start(void)
{
    Indicator_point();
    enterState(Q3_AC_ALIGN_ZERO, 0);
}

void Q3Mission_continueFromB(void)
{
    if (g_state == Q3_WAIT_CB_TRACE) {
        Indicator_point();
        enterState(Q3_BD_ALIGN_180, Q3_B_START_DECI_DEG);
    }
}

Q3Mission_Result Q3Mission_task1ms(void)
{
    if (g_state == Q3_DONE) {
        return Q3_MISSION_DONE;
    }
    if (g_state == Q3_FAULT) {
        return Q3_MISSION_FAULT;
    }

    if (g_state == Q3_REVERSE_BRAKE) {
        if (++g_stateMs >= BRAKE_REVERSE_MS) {
            Motor_stop();
            Indicator_done();
            if (g_brakeAfterAc) {
                /*
                 * Stop at C and wait for the future CB tracing module. That
                 * module calls Q3Mission_continueFromB() on arrival at B.
                 */
                g_state = Q3_WAIT_CB_TRACE;
                return Q3_MISSION_RUNNING;
            }
            g_state = Q3_DONE;
            return Q3_MISSION_DONE;
        }
        return Q3_MISSION_RUNNING;
    }

    if (g_state == Q3_WAIT_CB_TRACE) {
        Motor_stop();
        return Q3_MISSION_RUNNING;
    }

    if (++g_stateMs > MISSION_TIMEOUT_MS) {
        fail();
        return Q3_MISSION_FAULT;
    }

    switch (g_state) {
        case Q3_AC_ALIGN_ZERO:
            updateTurn(0, Q3_AC_TURN_38);
            break;
        case Q3_AC_TURN_38:
            updateTurn(Q3_AC_TARGET_DECI_DEG, Q3_AC_DRIVE);
            break;
        case Q3_BD_ALIGN_180:
            updateTurn(Q3_B_START_DECI_DEG, Q3_BD_TURN_142);
            break;
        case Q3_BD_TURN_142:
            updateTurn(Q3_BD_TARGET_DECI_DEG, Q3_BD_DRIVE);
            break;
        case Q3_AC_DRIVE:
        case Q3_BD_DRIVE:
            if (!Gyro_valid()) {
                fail();
                return Q3_MISSION_FAULT;
            }
            if ((g_stateMs % HEADING_PERIOD_MS) == 0U) {
                driveStraight();
            }
            updateBlackLine();
            break;
        default:
            fail();
            return Q3_MISSION_FAULT;
    }

    return (g_state == Q3_FAULT)
        ? Q3_MISSION_FAULT
        : Q3_MISSION_RUNNING;
}

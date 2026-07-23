#include "mission.h"

#include "app_config.h"
#include "encoder.h"
#include "gyro.h"
#include "indicator.h"
#include "keys.h"
#include "lcd.h"
#include "line_sensor.h"
#include "motor.h"
#include "q3_mission.h"

typedef enum {
    MISSION_IDLE,
    MISSION_Q1_RUN,
    MISSION_Q2_AB,
    MISSION_Q2_CD,
    MISSION_Q3_ACTIVE,
    MISSION_REVERSE_BRAKE,
    MISSION_FINISHED,
    MISSION_FAULT
} Mission_State;

static Mission_State g_state;
static uint32_t g_stateMs;
static uint32_t g_blackMs;
static uint32_t g_lcdMs;
static int16_t g_targetYaw;
static int16_t g_previousError;
static int32_t g_integral;

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

static void resetHeadingController(void)
{
    g_previousError = 0;
    g_integral = 0;
}

static int16_t headingCorrection(int16_t targetYaw)
{
    int16_t error = wrapError((int16_t)(targetYaw - Gyro_yawDeciDeg()));
    int16_t derivative = (int16_t)(error - g_previousError);
    int32_t correction;

    g_previousError = error;
    g_integral += error;
    if (g_integral > HEADING_INTEGRAL_LIMIT) {
        g_integral = HEADING_INTEGRAL_LIMIT;
    } else if (g_integral < -HEADING_INTEGRAL_LIMIT) {
        g_integral = -HEADING_INTEGRAL_LIMIT;
    }

    correction =
        ((int32_t)error * HEADING_KP_NUM / HEADING_KP_DEN) +
        (g_integral / HEADING_KI_DEN) +
        ((int32_t)derivative * HEADING_KD_NUM);

    if (correction > HEADING_LIMIT) {
        correction = HEADING_LIMIT;
    } else if (correction < -HEADING_LIMIT) {
        correction = -HEADING_LIMIT;
    }
    return (int16_t)correction;
}

static void beginMission(Mission_State next, int16_t targetYaw)
{
    g_state = next;
    g_targetYaw = targetYaw;
    g_stateMs = 0U;
    g_blackMs = 0U;
    Encoder_reset();
    resetHeadingController();
    Indicator_point();
}

static void finishImmediately(bool fault)
{
    Motor_stop();
    g_state = fault ? MISSION_FAULT : MISSION_FINISHED;
    if (fault) {
        Indicator_fault();
    } else {
        Indicator_done();
    }
}

static void beginReverseBrake(void)
{
    g_state = MISSION_REVERSE_BRAKE;
    g_stateMs = 0U;
    Motor_drive(-(int16_t)BRAKE_REVERSE_SPEED,
                -(int16_t)BRAKE_REVERSE_SPEED);
}

static void processStartKey(Key_Event key)
{
    if (key == KEY_Q1) {
        beginMission(MISSION_Q1_RUN, 0);
    } else if (key == KEY_Q2) {
        beginMission(MISSION_Q2_AB, 0);
    } else if (key == KEY_Q3) {
        Q3Mission_start();
        g_state = MISSION_Q3_ACTIVE;
        g_stateMs = 0U;
    }
}

static void driveStraight(void)
{
    int16_t correction = 0;

    if (Gyro_valid()) {
        correction = headingCorrection(g_targetYaw);
    }
    Motor_drive(
        (int16_t)(DRIVE_SPEED + correction),
        (int16_t)(DRIVE_SPEED - correction));
}

void Mission_init(void)
{
    Motor_stop();
    Q3Mission_init();
    g_state = MISSION_IDLE;
    g_stateMs = 0U;
    g_blackMs = 0U;
    g_lcdMs = 0U;
    g_targetYaw = 0;
    resetHeadingController();
}

void Mission_beginQ2_CD(void)
{
    if (g_state == MISSION_FINISHED || g_state == MISSION_IDLE) {
        beginMission(MISSION_Q2_CD, 1800);
    }
}

void Mission_task1ms(void)
{
    Key_Event key = Keys_task1ms();

    Indicator_task1ms();
    if (++g_lcdMs >= LCD_PERIOD_MS) {
        g_lcdMs = 0U;
        Lcd_showYaw(Gyro_yawDeciDeg());
    }

    if (g_state == MISSION_FINISHED || g_state == MISSION_FAULT) {
        Motor_stop();
        if (key == KEY_NONE) {
            return;
        }
        g_state = MISSION_IDLE;
    }

    if (g_state == MISSION_IDLE) {
        Motor_stop();
        processStartKey(key);
        return;
    }

    if (g_state == MISSION_Q3_ACTIVE) {
        Q3Mission_Result result = Q3Mission_task1ms();
        if (result == Q3_MISSION_DONE) {
            g_state = MISSION_FINISHED;
        } else if (result == Q3_MISSION_FAULT) {
            g_state = MISSION_FAULT;
        }
        return;
    }

    if (g_state == MISSION_REVERSE_BRAKE) {
        if (++g_stateMs >= BRAKE_REVERSE_MS) {
            finishImmediately(false);
        }
        return;
    }

    if (++g_stateMs > MISSION_TIMEOUT_MS) {
        finishImmediately(true);
        return;
    }

    if ((g_stateMs % HEADING_PERIOD_MS) == 0U) {
        driveStraight();
    }

    if (g_stateMs > START_GUARD_MS && LineSensor_blackLine()) {
        if (g_blackMs < BLACK_CONFIRM_MS) {
            g_blackMs++;
        }
        if (g_blackMs >= BLACK_CONFIRM_MS) {
            beginReverseBrake();
        }
    } else {
        g_blackMs = 0U;
    }
}

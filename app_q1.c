#include "app_q1.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "button.h"
#include "encoder.h"
#include "indicator.h"
#include "line_sensor.h"
#include "motor.h"
#include "speed_balance.h"

typedef enum {
    Q1_IDLE = 0,
    Q1_RUNNING,
    Q1_BRAKING,
    Q1_FINISHED,
    Q1_FAULT
} Q1_State;

static Q1_State gState;
static LineSensor_Frame gLine;
static uint32_t gRunTimeMs;
static uint16_t gBlackTimeMs;
static uint16_t gBrakeTimeMs;

static void AppQ1_stopNow(bool fault)
{
    Motor_coastAll();
    Motor_disable();
    gState = fault ? Q1_FAULT : Q1_IDLE;
    if (fault) {
        Indicator_fault();
    } else {
        Indicator_idle();
    }
}

static void AppQ1_start(void)
{
    gRunTimeMs = 0U;
    gBlackTimeMs = 0U;
    SpeedBalance_reset();
    Motor_enable();
    Motor_setForward(APP_BASE_SPEED_PERMILLE, APP_BASE_SPEED_PERMILLE);
    Indicator_start();
    gState = Q1_RUNNING;
}

static void AppQ1_arriveAtBlackLine(void)
{
    Motor_brakeAll();
    gBrakeTimeMs = APP_BRAKE_TIME_MS;
    gState = Q1_BRAKING;
    Indicator_finished();
}

static void AppQ1_checkBlackLine5ms(void)
{
    if (!LineSensor_read(&gLine)) {
        if (gState == Q1_RUNNING) {
            AppQ1_stopNow(true);
        }
        return;
    }

    if ((gState != Q1_RUNNING) ||
        (gRunTimeMs < APP_START_GUARD_MS)) {
        gBlackTimeMs = 0U;
        return;
    }

    if (gLine.blackCount >= Q1_BLACK_SENSOR_MIN) {
        gBlackTimeMs += APP_CONTROL_PERIOD_MS;
        if (gBlackTimeMs >= Q1_BLACK_CONFIRM_MS) {
            AppQ1_arriveAtBlackLine();
        }
    } else {
        gBlackTimeMs = 0U;
    }
}

void AppQ1_init(void)
{
    Button_init();
    Indicator_init();
    Motor_init();
    LineSensor_init();
    Encoder_init();

    gState = Q1_IDLE;
    gRunTimeMs = 0U;
    gBlackTimeMs = 0U;
    gBrakeTimeMs = 0U;
    gLine.valid = false;
}

void AppQ1_task1ms(void)
{
    static uint8_t lineDivider;
    static uint8_t speedDivider;

    Indicator_task1ms();

    if (Button_pollPressed1ms()) {
        if ((gState == Q1_RUNNING) || (gState == Q1_BRAKING)) {
            AppQ1_stopNow(false);
        } else {
            Indicator_idle();
            AppQ1_start();
        }
    }

    if (gState == Q1_RUNNING) {
        gRunTimeMs++;
        if (gRunTimeMs >= APP_MAX_RUN_MS) {
            AppQ1_stopNow(true);
        }
    } else if (gState == Q1_BRAKING) {
        if (gBrakeTimeMs > 0U) {
            gBrakeTimeMs--;
        }
        if (gBrakeTimeMs == 0U) {
            Motor_coastAll();
            Motor_disable();
            gState = Q1_FINISHED;
        }
    }

    lineDivider++;
    if (lineDivider >= APP_CONTROL_PERIOD_MS) {
        lineDivider = 0U;
        AppQ1_checkBlackLine5ms();
    }

    speedDivider++;
    if (speedDivider >= APP_SPEED_PERIOD_MS) {
        speedDivider = 0U;
        if (gState == Q1_RUNNING) {
            SpeedBalance_step();
        }
    }
}

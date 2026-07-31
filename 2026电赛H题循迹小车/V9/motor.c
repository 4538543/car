#include "motor.h"

#include <stdbool.h>

#include "app_config.h"
#include "ti_msp_dl_config.h"

#define MOTOR_PWM_PERIOD_COUNTS (1600U)
#define MOTOR_PWM_STOP_COMPARE  (MOTOR_PWM_PERIOD_COUNTS)

static int32_t clampPermille(int32_t value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 1000) {
        return 1000;
    }
    return value;
}

static uint32_t permilleToCompare(int32_t permille)
{
    uint32_t duty = (uint32_t)clampPermille(permille);
    return MOTOR_PWM_PERIOD_COUNTS -
        ((MOTOR_PWM_PERIOD_COUNTS * duty) / 1000U);
}

static void setLeftForwardDirection(void)
{
#if MOTOR_LEFT_FORWARD_IN1_HIGH
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_AIN1_PIN);
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_AIN2_PIN);
#else
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_AIN1_PIN);
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_AIN2_PIN);
#endif
}

static void setRightForwardDirection(void)
{
#if MOTOR_RIGHT_FORWARD_IN1_HIGH
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_BIN1_PIN);
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_BIN2_PIN);
#else
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_BIN1_PIN);
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_BIN2_PIN);
#endif
}

static void setLeftReverseDirection(void)
{
#if MOTOR_LEFT_FORWARD_IN1_HIGH
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_AIN1_PIN);
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_AIN2_PIN);
#else
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_AIN1_PIN);
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_AIN2_PIN);
#endif
}

static void setRightReverseDirection(void)
{
#if MOTOR_RIGHT_FORWARD_IN1_HIGH
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_BIN1_PIN);
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_BIN2_PIN);
#else
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_BIN1_PIN);
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_BIN2_PIN);
#endif
}

static void disablePwmOutputs(void)
{
    /*
     * ODIS forces both timer outputs low without changing IOMUX or stopping
     * TIMA0. This avoids the reset observed when PB8/PB9 were dynamically
     * switched between GPIO and timer functions.
     */
    DL_TimerA_setCCPOutputDisabled(MOTOR_PWM_INST,
        DL_TIMER_CCP_DIS_OUT_LOW, DL_TIMER_CCP_DIS_OUT_LOW);
}

static void enablePwmOutputs(void)
{
    DL_GPIO_setPins(MOTOR_CTRL_GPIO_PORT, MOTOR_CTRL_GPIO_STBY_PIN);
    DL_TimerA_setCCPOutputDisabled(MOTOR_PWM_INST,
        DL_TIMER_CCP_DIS_OUT_SET_BY_OCTL,
        DL_TIMER_CCP_DIS_OUT_SET_BY_OCTL);
}

void Motor_init(void)
{
    /*
     * Reassert the calibrated stop compare before exposing either PWM pin.
     */
    DL_TimerA_setCaptureCompareValue(
        MOTOR_PWM_INST, MOTOR_PWM_STOP_COMPARE, GPIO_MOTOR_PWM_C0_IDX);
    DL_TimerA_setCaptureCompareValue(
        MOTOR_PWM_INST, MOTOR_PWM_STOP_COMPARE, GPIO_MOTOR_PWM_C1_IDX);

    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT,
        MOTOR_CTRL_GPIO_AIN1_PIN | MOTOR_CTRL_GPIO_AIN2_PIN |
        MOTOR_CTRL_GPIO_BIN1_PIN | MOTOR_CTRL_GPIO_BIN2_PIN |
        MOTOR_CTRL_GPIO_STBY_PIN);
    disablePwmOutputs();
    DL_TimerA_startCounter(MOTOR_PWM_INST);
}

void Motor_driveForwardPermille(int32_t leftPermille, int32_t rightPermille)
{
    uint32_t primask;

    leftPermille = clampPermille(leftPermille);
    rightPermille = clampPermille(rightPermille);

    if ((leftPermille == 0) && (rightPermille == 0)) {
        Motor_stop();
        return;
    }

    /*
     * PB23 may interrupt at any time. Keep direction, compare update and PWM
     * pin connection atomic so the emergency-stop ISR cannot be followed by
     * the tail end of this function reconnecting a PWM pin.
     */
    primask = __get_PRIMASK();
    __disable_irq();

    setLeftForwardDirection();
    setRightForwardDirection();

    DL_TimerA_setCaptureCompareValue(
        MOTOR_PWM_INST, permilleToCompare(leftPermille),
        GPIO_MOTOR_PWM_C0_IDX);
    DL_TimerA_setCaptureCompareValue(
        MOTOR_PWM_INST, permilleToCompare(rightPermille),
        GPIO_MOTOR_PWM_C1_IDX);
    enablePwmOutputs();

    if (primask == 0U) {
        __enable_irq();
    }
}

void Motor_driveReversePermille(int32_t leftPermille, int32_t rightPermille)
{
    uint32_t primask;

    leftPermille = clampPermille(leftPermille);
    rightPermille = clampPermille(rightPermille);

    if ((leftPermille == 0) && (rightPermille == 0)) {
        Motor_stop();
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();

    setLeftReverseDirection();
    setRightReverseDirection();

    DL_TimerA_setCaptureCompareValue(
        MOTOR_PWM_INST, permilleToCompare(leftPermille),
        GPIO_MOTOR_PWM_C0_IDX);
    DL_TimerA_setCaptureCompareValue(
        MOTOR_PWM_INST, permilleToCompare(rightPermille),
        GPIO_MOTOR_PWM_C1_IDX);
    enablePwmOutputs();

    if (primask == 0U) {
        __enable_irq();
    }
}

void Motor_stop(void)
{
    disablePwmOutputs();
    DL_GPIO_clearPins(MOTOR_CTRL_GPIO_PORT,
        MOTOR_CTRL_GPIO_AIN1_PIN | MOTOR_CTRL_GPIO_AIN2_PIN |
        MOTOR_CTRL_GPIO_BIN1_PIN | MOTOR_CTRL_GPIO_BIN2_PIN |
        MOTOR_CTRL_GPIO_STBY_PIN);
    DL_TimerA_setCaptureCompareValue(
        MOTOR_PWM_INST, MOTOR_PWM_STOP_COMPARE, GPIO_MOTOR_PWM_C0_IDX);
    DL_TimerA_setCaptureCompareValue(
        MOTOR_PWM_INST, MOTOR_PWM_STOP_COMPARE, GPIO_MOTOR_PWM_C1_IDX);
}

void Motor_emergencyStop(void)
{
    Motor_stop();
}

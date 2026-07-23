#include "motor.h"

#include "ti_msp_dl_config.h"

#define MOTOR_A_IN1_PIN        (DL_GPIO_PIN_4)
#define MOTOR_A_IN2_PIN        (DL_GPIO_PIN_5)
#define MOTOR_B_IN1_PIN        (DL_GPIO_PIN_6)
#define MOTOR_B_IN2_PIN        (DL_GPIO_PIN_7)

#define MOTOR_SPEED_MAX        (1000U)

static void Motor_writeDirectionPins(uint32_t in1Pin, uint32_t in2Pin,
                                     Motor_Direction direction)
{
    switch (direction) {
    case MOTOR_FORWARD:
        DL_GPIO_setPins(GPIOB, in1Pin);
        DL_GPIO_clearPins(GPIOB, in2Pin);
        break;

    case MOTOR_REVERSE:
        DL_GPIO_clearPins(GPIOB, in1Pin);
        DL_GPIO_setPins(GPIOB, in2Pin);
        break;

    case MOTOR_BRAKE:
        DL_GPIO_setPins(GPIOB, in1Pin | in2Pin);
        break;

    case MOTOR_COAST:
    default:
        DL_GPIO_clearPins(GPIOB, in1Pin | in2Pin);
        break;
    }
}

static uint32_t Motor_dutyToCompare(GPTIMER_Regs *timer,
                                    uint16_t speedPermille)
{
    uint32_t periodCounts = DL_Timer_getLoadValue(timer) + 1U;

    if (speedPermille > MOTOR_SPEED_MAX) {
        speedPermille = MOTOR_SPEED_MAX;
    }

    /*
     * SysConfig must use edge-aligned PWM whose output is SET at ZERO and
     * CLEARED at CC-UP. In that configuration, compare/period equals duty.
     */
    return (periodCounts * (uint32_t) speedPermille) / MOTOR_SPEED_MAX;
}

void Motor_init(void)
{
    /* SYSCFG_DL_init() has already configured the pin directions and timers. */
    Motor_stopAll();
    DL_Timer_startCounter(TIMG0);
    DL_Timer_startCounter(TIMA1);
}

void Motor_setDirection(Motor_Id motor, Motor_Direction direction)
{
    if (motor == MOTOR_A) {
        Motor_writeDirectionPins(MOTOR_A_IN1_PIN, MOTOR_A_IN2_PIN, direction);
    } else {
        Motor_writeDirectionPins(MOTOR_B_IN1_PIN, MOTOR_B_IN2_PIN, direction);
    }
}

void Motor_setSpeed(Motor_Id motor, uint16_t speedPermille)
{
    if (motor == MOTOR_A) {
        DL_Timer_setCaptureCompareValue(
            TIMG0, Motor_dutyToCompare(TIMG0, speedPermille),
            DL_TIMER_CC_1_INDEX);
    } else {
        DL_Timer_setCaptureCompareValue(
            TIMA1, Motor_dutyToCompare(TIMA1, speedPermille),
            DL_TIMER_CC_0_INDEX);
    }
}

void Motor_run(Motor_Id motor, Motor_Direction direction,
               uint16_t speedPermille)
{
    Motor_setDirection(motor, direction);
    Motor_setSpeed(motor, speedPermille);
}

void Motor_stop(Motor_Id motor)
{
    Motor_setSpeed(motor, 0U);
    Motor_setDirection(motor, MOTOR_COAST);
}

void Motor_stopAll(void)
{
    Motor_stop(MOTOR_A);
    Motor_stop(MOTOR_B);
}

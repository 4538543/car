#include "motor.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"

#define MOTOR_DUTY_MAX (1000U)

static uint16_t Motor_clamp(uint16_t duty)
{
    return (duty > MOTOR_DUTY_MAX) ? MOTOR_DUTY_MAX : duty;
}

static uint32_t Motor_compare(GPTIMER_Regs *timer, uint16_t duty)
{
    uint32_t period = DL_Timer_getLoadValue(timer) + 1U;
    return (period * (uint32_t) Motor_clamp(duty)) / MOTOR_DUTY_MAX;
}

static void Motor_setDuty(GPTIMER_Regs *timer, DL_TIMER_CC_INDEX cc,
                          uint16_t duty)
{
    DL_Timer_setCaptureCompareValue(timer, Motor_compare(timer, duty), cc);
}

void Motor_init(void)
{
    DL_GPIO_clearPins(MOTOR_DIR_PORT,
        MOTOR_A_IN1_PIN | MOTOR_A_IN2_PIN |
        MOTOR_B_IN1_PIN | MOTOR_B_IN2_PIN | MOTOR_STBY_PIN);

    DL_Timer_startCounter(MOTOR_A_PWM_TIMER);
    DL_Timer_startCounter(MOTOR_B_PWM_TIMER);
    Motor_coastAll();
    Motor_disable();
}

void Motor_enable(void)
{
    DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_STBY_PIN);
}

void Motor_disable(void)
{
    DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_STBY_PIN);
}

void Motor_setForward(uint16_t leftPermille, uint16_t rightPermille)
{
    /* Motor A is the left rear wheel; Motor B is the right rear wheel. */
    DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_A_IN1_PIN | MOTOR_B_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_A_IN2_PIN | MOTOR_B_IN2_PIN);

    Motor_setDuty(MOTOR_A_PWM_TIMER, MOTOR_A_PWM_CC, leftPermille);
    Motor_setDuty(MOTOR_B_PWM_TIMER, MOTOR_B_PWM_CC, rightPermille);
}

void Motor_brakeAll(void)
{
    Motor_setDuty(MOTOR_A_PWM_TIMER, MOTOR_A_PWM_CC, MOTOR_DUTY_MAX);
    Motor_setDuty(MOTOR_B_PWM_TIMER, MOTOR_B_PWM_CC, MOTOR_DUTY_MAX);
    DL_GPIO_setPins(MOTOR_DIR_PORT,
        MOTOR_A_IN1_PIN | MOTOR_A_IN2_PIN |
        MOTOR_B_IN1_PIN | MOTOR_B_IN2_PIN);
}

void Motor_coastAll(void)
{
    Motor_setDuty(MOTOR_A_PWM_TIMER, MOTOR_A_PWM_CC, 0U);
    Motor_setDuty(MOTOR_B_PWM_TIMER, MOTOR_B_PWM_CC, 0U);
    DL_GPIO_clearPins(MOTOR_DIR_PORT,
        MOTOR_A_IN1_PIN | MOTOR_A_IN2_PIN |
        MOTOR_B_IN1_PIN | MOTOR_B_IN2_PIN);
}

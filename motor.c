#include "motor.h"
#include "ti_msp_dl_config.h"

static uint32_t compare(GPTIMER_Regs *t, uint16_t duty) {
    uint32_t p = DL_Timer_getLoadValue(t) + 1U;
    return p * duty / 1000U;
}
static void one(GPTIMER_Regs *t, DL_TIMER_CC_INDEX cc, uint16_t duty) {
    if (duty > 1000U) duty = 1000U;
    DL_Timer_setCaptureCompareValue(t, compare(t, duty), cc);
}
void Motor_init(void) {
    /* SysConfig's PWM default duty may be non-zero.  Establish the safe
       electrical state only after both timer counters are available. */
    DL_Timer_startCounter(PWM_LEFT_INST);
    DL_Timer_startCounter(PWM_RIGHT_INST);
    Motor_stop();
}
void Motor_drive(int16_t left, int16_t right) {
    uint16_t l = (left < 0) ? (uint16_t)-left : (uint16_t)left;
    uint16_t r = (right < 0) ? (uint16_t)-right : (uint16_t)right;
    if (left >= 0) { DL_GPIO_setPins(GPIOB, MOTOR_CTRL_MOTOR_A_IN1_PIN); DL_GPIO_clearPins(GPIOB, MOTOR_CTRL_MOTOR_A_IN2_PIN); }
    else { DL_GPIO_clearPins(GPIOB, MOTOR_CTRL_MOTOR_A_IN1_PIN); DL_GPIO_setPins(GPIOB, MOTOR_CTRL_MOTOR_A_IN2_PIN); }
    if (right >= 0) { DL_GPIO_setPins(GPIOB, MOTOR_CTRL_MOTOR_B_IN1_PIN); DL_GPIO_clearPins(GPIOB, MOTOR_CTRL_MOTOR_B_IN2_PIN); }
    else { DL_GPIO_clearPins(GPIOB, MOTOR_CTRL_MOTOR_B_IN1_PIN); DL_GPIO_setPins(GPIOB, MOTOR_CTRL_MOTOR_B_IN2_PIN); }
    DL_GPIO_setPins(GPIOB, MOTOR_CTRL_MOTOR_STBY_PIN);
    one(PWM_LEFT_INST, GPIO_PWM_LEFT_C1_IDX, l); one(PWM_RIGHT_INST, GPIO_PWM_RIGHT_C0_IDX, r);
}
void Motor_stop(void) {
    one(PWM_LEFT_INST, GPIO_PWM_LEFT_C1_IDX, 0U);
    one(PWM_RIGHT_INST, GPIO_PWM_RIGHT_C0_IDX, 0U);
    DL_GPIO_clearPins(GPIOB, MOTOR_CTRL_MOTOR_A_IN1_PIN |
                              MOTOR_CTRL_MOTOR_A_IN2_PIN |
                              MOTOR_CTRL_MOTOR_B_IN1_PIN |
                              MOTOR_CTRL_MOTOR_B_IN2_PIN |
                              MOTOR_CTRL_MOTOR_STBY_PIN);
}

#include "ti_msp_dl_config.h"

#include "buttons.h"
#include "motor.h"

#define APP_CPU_CLOCK_HZ       (80000000UL)
#define APP_LOOP_DELAY_CYCLES  (APP_CPU_CLOCK_HZ / 1000UL)

#define SPEED_80_PERCENT       (800U)
/* round(800 / 1.52) = round(526.315...) = 526 per mille */
#define SPEED_80_DIV_1_52      (526U)

static void App_handleButton(Button_Event event)
{
    switch (event) {
    case BUTTON_EQUAL:
        Motor_run(MOTOR_A, MOTOR_FORWARD, SPEED_80_PERCENT);
        Motor_run(MOTOR_B, MOTOR_FORWARD, SPEED_80_PERCENT);
        break;

    case BUTTON_A_FAST:
        Motor_run(MOTOR_A, MOTOR_FORWARD, SPEED_80_PERCENT);
        Motor_run(MOTOR_B, MOTOR_FORWARD, SPEED_80_DIV_1_52);
        break;

    case BUTTON_B_FAST:
        Motor_run(MOTOR_A, MOTOR_FORWARD, SPEED_80_DIV_1_52);
        Motor_run(MOTOR_B, MOTOR_FORWARD, SPEED_80_PERCENT);
        break;

    case BUTTON_NONE:
    default:
        break;
    }
}

int main(void)
{
    SYSCFG_DL_init();
    Motor_init();
    Buttons_init();

    while (1) {
        App_handleButton(Buttons_poll1ms());
        delay_cycles(APP_LOOP_DELAY_CYCLES);
    }
}

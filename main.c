#include "encoder.h"
#include "gyro.h"
#include "keys.h"
#include "lcd.h"
#include "line_sensor.h"
#include "mission.h"
#include "motor.h"
#include "ti_msp_dl_config.h"

#include <stdbool.h>

static volatile bool g_controlTick;

int main(void)
{
    SYSCFG_DL_init();

    /* Establish a safe motor state before initializing slower peripherals. */
    Motor_init();
    Keys_init();
    Gyro_init();
    Encoder_init();
    LineSensor_init();
    Lcd_init();
    Mission_init();

    DL_Timer_clearInterruptStatus(
        CONTROL_TIMER_INST, DL_TIMER_INTERRUPT_ZERO_EVENT);
    DL_Timer_enableInterrupt(
        CONTROL_TIMER_INST, DL_TIMER_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(CONTROL_TIMER_INST_INT_IRQN);
    NVIC_EnableIRQ(CONTROL_TIMER_INST_INT_IRQN);

    /*
     * SysConfig configures and clocks the timer, but does not start its
     * counter. Without this call the 1 ms task, key scan, LCD refresh and
     * mission state machine never run.
     */
    DL_Timer_startCounter(CONTROL_TIMER_INST);

    while (1) {
        if (g_controlTick) {
            __disable_irq();
            g_controlTick = false;
            __enable_irq();
            Mission_task1ms();
        }
    }
}

void CONTROL_TIMER_INST_IRQHandler(void)
{
    if (DL_TimerG_getPendingInterrupt(CONTROL_TIMER_INST) ==
        DL_TIMERG_IIDX_ZERO) {
        g_controlTick = true;
    }
}

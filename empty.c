#include "ti_msp_dl_config.h"

#include <stdbool.h>

#include "app_q1.h"

static volatile bool gTask1msPending;

int main(void)
{
    SYSCFG_DL_init();
    AppQ1_init();

    gTask1msPending = false;
    DL_Timer_clearInterruptStatus(
        CONTROL_TIMER_INST, DL_TIMER_INTERRUPT_ZERO_EVENT);
    DL_Timer_enableInterrupt(
        CONTROL_TIMER_INST, DL_TIMER_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(CONTROL_TIMER_INST_INT_IRQN);
    NVIC_EnableIRQ(CONTROL_TIMER_INST_INT_IRQN);

    while (1) {
        if (gTask1msPending) {
            __disable_irq();
            gTask1msPending = false;
            __enable_irq();
            AppQ1_task1ms();
        }
    }
}

void CONTROL_TIMER_INST_IRQHandler(void)
{
    if (DL_TimerG_getPendingInterrupt(CONTROL_TIMER_INST) ==
        DL_TIMERG_IIDX_ZERO) {
        gTask1msPending = true;
    }
}

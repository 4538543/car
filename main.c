#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include "gyro.h"
#include "encoder.h"
#include "keys.h"
#include "lcd.h"
#include "mission.h"
#include "motor.h"
static volatile bool tick;
int main(void){SYSCFG_DL_init();Motor_init();Keys_init();Gyro_init();Encoder_init();Lcd_init();Mission_init();DL_Timer_clearInterruptStatus(CONTROL_TIMER_INST,DL_TIMER_INTERRUPT_ZERO_EVENT);DL_Timer_enableInterrupt(CONTROL_TIMER_INST,DL_TIMER_INTERRUPT_ZERO_EVENT);NVIC_EnableIRQ(CONTROL_TIMER_INST_INT_IRQN);while(1){if(tick){__disable_irq();tick=false;__enable_irq();Mission_task1ms();}}}
void CONTROL_TIMER_INST_IRQHandler(void){if(DL_TimerG_getPendingInterrupt(CONTROL_TIMER_INST)==DL_TIMERG_IIDX_ZERO)tick=true;}

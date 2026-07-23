#include "encoder.h"
#include "ti_msp_dl_config.h"
static volatile uint32_t left,right;
void Encoder_init(void){left=right=0;DL_GPIO_clearInterruptStatus(ENCODER_IO_PORT,ENCODER_IO_ENCODER_E1A_PIN|ENCODER_IO_ENCODER_E2A_PIN);NVIC_ClearPendingIRQ(ENCODER_IO_INT_IRQN);NVIC_EnableIRQ(ENCODER_IO_INT_IRQN);}
void Encoder_reset(void){__disable_irq();left=right=0;__enable_irq();}
uint32_t Encoder_averagePulses(void){uint32_t v;__disable_irq();v=(left+right)/2U;__enable_irq();return v;}
void GROUP1_IRQHandler(void){if(DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)==ENCODER_IO_INT_IIDX){switch(DL_GPIO_getPendingInterrupt(ENCODER_IO_PORT)){case ENCODER_IO_ENCODER_E1A_IIDX:left++;break;case ENCODER_IO_ENCODER_E2A_IIDX:right++;break;default:break;}}}

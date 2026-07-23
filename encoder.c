#include "encoder.h"

#include "ti_msp_dl_config.h"

static volatile uint32_t gLeftPulses;
static volatile uint32_t gRightPulses;

void Encoder_init(void)
{
    gLeftPulses = 0U;
    gRightPulses = 0U;
    DL_GPIO_clearInterruptStatus(ENCODER_IO_PORT,
        ENCODER_IO_ENCODER_E1A_PIN | ENCODER_IO_ENCODER_E2A_PIN);
    NVIC_ClearPendingIRQ(ENCODER_IO_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_IO_INT_IRQN);
}

void Encoder_reset(void)
{
    __disable_irq();
    gLeftPulses = 0U;
    gRightPulses = 0U;
    __enable_irq();
}

void Encoder_sample(uint32_t *leftPulses, uint32_t *rightPulses)
{
    __disable_irq();
    *leftPulses = gLeftPulses;
    *rightPulses = gRightPulses;
    gLeftPulses = 0U;
    gRightPulses = 0U;
    __enable_irq();
}

void GROUP1_IRQHandler(void)
{
    if (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1) ==
        ENCODER_IO_INT_IIDX) {
        switch (DL_GPIO_getPendingInterrupt(ENCODER_IO_PORT)) {
            case ENCODER_IO_ENCODER_E1A_IIDX:
                gLeftPulses++;
                break;
            case ENCODER_IO_ENCODER_E2A_IIDX:
                gRightPulses++;
                break;
            default:
                break;
        }
    }
}

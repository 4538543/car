#include "encoder.h"

#include "ti_msp_dl_config.h"

static volatile uint32_t gLeftPulses;
static volatile uint32_t gRightPulses;
static volatile uint32_t gLeftTotal;
static volatile uint32_t gRightTotal;

void Encoder_init(void)
{
    gLeftPulses = 0U;
    gRightPulses = 0U;
    gLeftTotal = 0U;
    gRightTotal = 0U;
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
    gLeftTotal = 0U;
    gRightTotal = 0U;
    __enable_irq();
}

void Encoder_getTotal(uint32_t *leftPulses, uint32_t *rightPulses)
{
    __disable_irq();
    *leftPulses = gLeftTotal;
    *rightPulses = gRightTotal;
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
                gLeftTotal++;
                break;
            case ENCODER_IO_ENCODER_E2A_IIDX:
                gRightPulses++;
                gRightTotal++;
                break;
            default:
                break;
        }
    }
}

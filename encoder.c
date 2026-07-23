#include "encoder.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"

static volatile uint32_t g_leftPulses;
static volatile uint32_t g_rightPulses;

void Encoder_init(void)
{
    g_leftPulses = 0U;
    g_rightPulses = 0U;
    DL_GPIO_clearInterruptStatus(
        ENCODER_IO_PORT,
        ENCODER_IO_ENCODER_E1A_PIN | ENCODER_IO_ENCODER_E2A_PIN);
    NVIC_ClearPendingIRQ(ENCODER_IO_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_IO_INT_IRQN);
}

void Encoder_reset(void)
{
    __disable_irq();
    g_leftPulses = 0U;
    g_rightPulses = 0U;
    __enable_irq();
}

uint32_t Encoder_averagePulses(void)
{
    uint32_t average;

    __disable_irq();
    average = (g_leftPulses + g_rightPulses) / 2U;
    __enable_irq();
    return average;
}

uint32_t Encoder_averageDistanceMm(void)
{
    /*
     * wheel circumference = pi * diameter
     * pi is represented as 31416 / 10000 to avoid floating point.
     */
    uint64_t numerator =
        (uint64_t)Encoder_averagePulses() * 31416U * WHEEL_DIAMETER_MM;
    uint64_t denominator =
        (uint64_t)ENCODER_PULSES_PER_REV * 10000U;

    return (uint32_t)(numerator / denominator);
}

void GROUP1_IRQHandler(void)
{
    if (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1) !=
        ENCODER_IO_INT_IIDX) {
        return;
    }

    switch (DL_GPIO_getPendingInterrupt(ENCODER_IO_PORT)) {
        case ENCODER_IO_ENCODER_E1A_IIDX:
            g_leftPulses++;
            break;
        case ENCODER_IO_ENCODER_E2A_IIDX:
            g_rightPulses++;
            break;
        default:
            break;
    }
}

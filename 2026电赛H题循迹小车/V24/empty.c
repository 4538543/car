#include <stdbool.h>
#include <stdint.h>

#include "ab_mission.h"
#include "app_time.h"
#include "buttons.h"
#include "display.h"
#include "encoder.h"
#include "gyro.h"
#include "gray_sensor.h"
#include "motor.h"
#include "oled_time.h"
#include "stm32_link.h"
#include "straight_control.h"
#include "ti_msp_dl_config.h"

static volatile uint32_t gPending1msTicks;

static bool takeOneMillisecondTick(void)
{
    bool available = false;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (gPending1msTicks > 0U) {
        gPending1msTicks--;
        available = true;
    }

    if (primask == 0U) {
        __enable_irq();
    }
    return available;
}

static void enableApplicationInterrupts(void)
{
    const uint32_t encoderMask =
        ENCODER_GPIO_E1A_PIN | ENCODER_GPIO_E1B_PIN |
        ENCODER_GPIO_E2A_PIN | ENCODER_GPIO_E2B_PIN;
    const uint32_t keyBMask =
        KEY_GPIO_START_AB_PASS_PIN |
        KEY_GPIO_START_LAP_PASS_PIN |
        KEY_GPIO_EMERGENCY_STOP_PIN;

    DL_GPIO_clearInterruptStatus(
        GPIOA, encoderMask | KEY_GPIO_START_LAP_NOW_PIN);
    DL_GPIO_clearInterruptStatus(GPIOB, keyBMask);

    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_ClearPendingIRQ(KEY_GPIO_GPIOB_INT_IRQN);
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(KEY_GPIO_GPIOB_INT_IRQN);
}

int main(void)
{
    SYSCFG_DL_init();

    Motor_init();
    Encoder_init();
    Buttons_init();
    AppTime_init();
    StraightControl_init();
    GraySensor_init();
    ABMission_init();
    Gyro_init();
    Stm32Link_init();
    Display_init();
    OledTime_init();

    gPending1msTicks = 0U;
    enableApplicationInterrupts();

    while (1) {
        if (takeOneMillisecondTick()) {
            uint32_t buttonEvents = Buttons_task1ms();

            if ((buttonEvents & BUTTON_EVENT_STOP) != 0U) {
                ABMission_requestEmergencyStop();
            } else if ((buttonEvents &
                        BUTTON_EVENT_START_LAP_NOW) != 0U) {
                ABMission_startLapImmediate();
            } else if ((buttonEvents &
                        BUTTON_EVENT_START_AB_PASS) != 0U) {
                ABMission_startAbPass();
            } else if ((buttonEvents &
                        BUTTON_EVENT_START_LAP_PASS) != 0U) {
                ABMission_startLapPass();
            }

            Gyro_task1ms();
            AppTime_task1ms();
            ABMission_task1ms();
            Stm32Link_task1ms();
            Display_task1ms();
            OledTime_task1ms();
        } else {
            __WFI();
        }
    }
}

void SysTick_Handler(void)
{
    if (gPending1msTicks < UINT32_MAX) {
        gPending1msTicks++;
    }
}

void GYRO_UART_INST_IRQHandler(void)
{
    Gyro_uartIrqHandler();
}

void GROUP1_IRQHandler(void)
{
    const uint32_t encoderMask =
        ENCODER_GPIO_E1A_PIN | ENCODER_GPIO_E1B_PIN |
        ENCODER_GPIO_E2A_PIN | ENCODER_GPIO_E2B_PIN;
    const uint32_t gpioAMask =
        encoderMask | KEY_GPIO_START_LAP_NOW_PIN;
    const uint32_t gpioBMask =
        KEY_GPIO_START_AB_PASS_PIN |
        KEY_GPIO_START_LAP_PASS_PIN |
        KEY_GPIO_EMERGENCY_STOP_PIN;
    uint32_t gpioAStatus =
        DL_GPIO_getEnabledInterruptStatus(GPIOA, gpioAMask);
    uint32_t gpioBStatus =
        DL_GPIO_getEnabledInterruptStatus(GPIOB, gpioBMask);

    /*
     * Clear the latched events before doing any processing. If a new encoder
     * edge arrives while this ISR is running, it remains pending for the next
     * entry instead of being cleared accidentally at the end of this entry.
     */
    if (gpioAStatus != 0U) {
        DL_GPIO_clearInterruptStatus(GPIOA, gpioAStatus);
    }
    if (gpioBStatus != 0U) {
        DL_GPIO_clearInterruptStatus(GPIOB, gpioBStatus);
    }

    if ((gpioAStatus & encoderMask) != 0U) {
        Encoder_onGpioInterrupt(gpioAStatus);
    }

    if ((gpioBStatus & KEY_GPIO_EMERGENCY_STOP_PIN) != 0U) {
        ABMission_requestEmergencyStopFromISR();
    }
}

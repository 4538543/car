/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for MOTOR_PWM */
#define MOTOR_PWM_INST                                                     TIMA0
#define MOTOR_PWM_INST_IRQHandler                               TIMA0_IRQHandler
#define MOTOR_PWM_INST_INT_IRQN                                 (TIMA0_INT_IRQn)
#define MOTOR_PWM_INST_CLK_FREQ                                         32000000
/* GPIO defines for channel 0 */
#define GPIO_MOTOR_PWM_C0_PORT                                             GPIOB
#define GPIO_MOTOR_PWM_C0_PIN                                      DL_GPIO_PIN_8
#define GPIO_MOTOR_PWM_C0_IOMUX                                  (IOMUX_PINCM25)
#define GPIO_MOTOR_PWM_C0_IOMUX_FUNC                 IOMUX_PINCM25_PF_TIMA0_CCP0
#define GPIO_MOTOR_PWM_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_MOTOR_PWM_C1_PORT                                             GPIOB
#define GPIO_MOTOR_PWM_C1_PIN                                      DL_GPIO_PIN_9
#define GPIO_MOTOR_PWM_C1_IOMUX                                  (IOMUX_PINCM26)
#define GPIO_MOTOR_PWM_C1_IOMUX_FUNC                 IOMUX_PINCM26_PF_TIMA0_CCP1
#define GPIO_MOTOR_PWM_C1_IDX                                DL_TIMER_CC_1_INDEX



/* Defines for GYRO_UART */
#define GYRO_UART_INST                                                     UART0
#define GYRO_UART_INST_FREQUENCY                                        32000000
#define GYRO_UART_INST_IRQHandler                               UART0_IRQHandler
#define GYRO_UART_INST_INT_IRQN                                   UART0_INT_IRQn
#define GPIO_GYRO_UART_RX_PORT                                             GPIOA
#define GPIO_GYRO_UART_TX_PORT                                             GPIOA
#define GPIO_GYRO_UART_RX_PIN                                     DL_GPIO_PIN_11
#define GPIO_GYRO_UART_TX_PIN                                     DL_GPIO_PIN_10
#define GPIO_GYRO_UART_IOMUX_RX                                  (IOMUX_PINCM22)
#define GPIO_GYRO_UART_IOMUX_TX                                  (IOMUX_PINCM21)
#define GPIO_GYRO_UART_IOMUX_RX_FUNC                   IOMUX_PINCM22_PF_UART0_RX
#define GPIO_GYRO_UART_IOMUX_TX_FUNC                   IOMUX_PINCM21_PF_UART0_TX
#define GYRO_UART_BAUD_RATE                                             (115200)
#define GYRO_UART_IBRD_32_MHZ_115200_BAUD                                   (17)
#define GYRO_UART_FBRD_32_MHZ_115200_BAUD                                   (23)
/* Defines for STM32_UART */
#define STM32_UART_INST                                                    UART1
#define STM32_UART_INST_FREQUENCY                                       32000000
#define STM32_UART_INST_IRQHandler                              UART1_IRQHandler
#define STM32_UART_INST_INT_IRQN                                  UART1_INT_IRQn
#define GPIO_STM32_UART_RX_PORT                                            GPIOA
#define GPIO_STM32_UART_TX_PORT                                            GPIOB
#define GPIO_STM32_UART_RX_PIN                                     DL_GPIO_PIN_9
#define GPIO_STM32_UART_TX_PIN                                     DL_GPIO_PIN_4
#define GPIO_STM32_UART_IOMUX_RX                                 (IOMUX_PINCM20)
#define GPIO_STM32_UART_IOMUX_TX                                 (IOMUX_PINCM17)
#define GPIO_STM32_UART_IOMUX_RX_FUNC                  IOMUX_PINCM20_PF_UART1_RX
#define GPIO_STM32_UART_IOMUX_TX_FUNC                  IOMUX_PINCM17_PF_UART1_TX
#define STM32_UART_BAUD_RATE                                            (115200)
#define STM32_UART_IBRD_32_MHZ_115200_BAUD                                  (17)
#define STM32_UART_FBRD_32_MHZ_115200_BAUD                                  (23)





/* Defines for BLK: GPIOA.16 with pinCMx 38 on package pin 9 */
#define LCD_GPIO_BLK_PORT                                                (GPIOA)
#define LCD_GPIO_BLK_PIN                                        (DL_GPIO_PIN_16)
#define LCD_GPIO_BLK_IOMUX                                       (IOMUX_PINCM38)
/* Defines for DC: GPIOB.15 with pinCMx 32 on package pin 3 */
#define LCD_GPIO_DC_PORT                                                 (GPIOB)
#define LCD_GPIO_DC_PIN                                         (DL_GPIO_PIN_15)
#define LCD_GPIO_DC_IOMUX                                        (IOMUX_PINCM32)
/* Defines for SDO: GPIOB.20 with pinCMx 48 on package pin 19 */
#define LCD_GPIO_SDO_PORT                                                (GPIOB)
#define LCD_GPIO_SDO_PIN                                        (DL_GPIO_PIN_20)
#define LCD_GPIO_SDO_IOMUX                                       (IOMUX_PINCM48)
/* Defines for CS: GPIOB.17 with pinCMx 43 on package pin 14 */
#define LCD_GPIO_CS_PORT                                                 (GPIOB)
#define LCD_GPIO_CS_PIN                                         (DL_GPIO_PIN_17)
#define LCD_GPIO_CS_IOMUX                                        (IOMUX_PINCM43)
/* Defines for LCD_CLK: GPIOA.17 with pinCMx 39 on package pin 10 */
#define LCD_GPIO_LCD_CLK_PORT                                            (GPIOA)
#define LCD_GPIO_LCD_CLK_PIN                                    (DL_GPIO_PIN_17)
#define LCD_GPIO_LCD_CLK_IOMUX                                   (IOMUX_PINCM39)
/* Defines for SDI: GPIOB.16 with pinCMx 33 on package pin 4 */
#define LCD_GPIO_SDI_PORT                                                (GPIOB)
#define LCD_GPIO_SDI_PIN                                        (DL_GPIO_PIN_16)
#define LCD_GPIO_SDI_IOMUX                                       (IOMUX_PINCM33)
/* Defines for START_AB: GPIOA.18 with pinCMx 40 on package pin 11 */
#define KEY_GPIO_START_AB_PORT                                           (GPIOA)
// groups represented: ["ENCODER_GPIO","KEY_GPIO"]
// pins affected: ["E1A","E1B","E2A","E2B","START_AB"]
#define GPIO_MULTIPLE_GPIOA_INT_IRQN                            (GPIOA_INT_IRQn)
#define GPIO_MULTIPLE_GPIOA_INT_IIDX            (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define KEY_GPIO_START_AB_IIDX                              (DL_GPIO_IIDX_DIO18)
#define KEY_GPIO_START_AB_PIN                                   (DL_GPIO_PIN_18)
#define KEY_GPIO_START_AB_IOMUX                                  (IOMUX_PINCM40)
/* Defines for START_LAP: GPIOB.22 with pinCMx 50 on package pin 21 */
#define KEY_GPIO_START_LAP_PORT                                          (GPIOB)
// pins affected by this interrupt request:["START_LAP","EMERGENCY_STOP"]
#define KEY_GPIO_GPIOB_INT_IRQN                                 (GPIOB_INT_IRQn)
#define KEY_GPIO_GPIOB_INT_IIDX                 (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define KEY_GPIO_START_LAP_IIDX                             (DL_GPIO_IIDX_DIO22)
#define KEY_GPIO_START_LAP_PIN                                  (DL_GPIO_PIN_22)
#define KEY_GPIO_START_LAP_IOMUX                                 (IOMUX_PINCM50)
/* Defines for EMERGENCY_STOP: GPIOB.23 with pinCMx 51 on package pin 22 */
#define KEY_GPIO_EMERGENCY_STOP_PORT                                     (GPIOB)
#define KEY_GPIO_EMERGENCY_STOP_IIDX                        (DL_GPIO_IIDX_DIO23)
#define KEY_GPIO_EMERGENCY_STOP_PIN                             (DL_GPIO_PIN_23)
#define KEY_GPIO_EMERGENCY_STOP_IOMUX                            (IOMUX_PINCM51)
/* Defines for SPARE: GPIOB.24 with pinCMx 52 on package pin 23 */
#define KEY_GPIO_SPARE_PORT                                              (GPIOB)
#define KEY_GPIO_SPARE_PIN                                      (DL_GPIO_PIN_24)
#define KEY_GPIO_SPARE_IOMUX                                     (IOMUX_PINCM52)
/* Port definition for Pin Group MOTOR_CTRL_GPIO */
#define MOTOR_CTRL_GPIO_PORT                                             (GPIOB)

/* Defines for AIN1: GPIOB.10 with pinCMx 27 on package pin 62 */
#define MOTOR_CTRL_GPIO_AIN1_PIN                                (DL_GPIO_PIN_10)
#define MOTOR_CTRL_GPIO_AIN1_IOMUX                               (IOMUX_PINCM27)
/* Defines for AIN2: GPIOB.11 with pinCMx 28 on package pin 63 */
#define MOTOR_CTRL_GPIO_AIN2_PIN                                (DL_GPIO_PIN_11)
#define MOTOR_CTRL_GPIO_AIN2_IOMUX                               (IOMUX_PINCM28)
/* Defines for BIN1: GPIOB.12 with pinCMx 29 on package pin 64 */
#define MOTOR_CTRL_GPIO_BIN1_PIN                                (DL_GPIO_PIN_12)
#define MOTOR_CTRL_GPIO_BIN1_IOMUX                               (IOMUX_PINCM29)
/* Defines for BIN2: GPIOB.13 with pinCMx 30 on package pin 1 */
#define MOTOR_CTRL_GPIO_BIN2_PIN                                (DL_GPIO_PIN_13)
#define MOTOR_CTRL_GPIO_BIN2_IOMUX                               (IOMUX_PINCM30)
/* Defines for STBY: GPIOB.14 with pinCMx 31 on package pin 2 */
#define MOTOR_CTRL_GPIO_STBY_PIN                                (DL_GPIO_PIN_14)
#define MOTOR_CTRL_GPIO_STBY_IOMUX                               (IOMUX_PINCM31)
/* Port definition for Pin Group ENCODER_GPIO */
#define ENCODER_GPIO_PORT                                                (GPIOA)

/* Defines for E1A: GPIOA.21 with pinCMx 46 on package pin 17 */
#define ENCODER_GPIO_E1A_IIDX                               (DL_GPIO_IIDX_DIO21)
#define ENCODER_GPIO_E1A_PIN                                    (DL_GPIO_PIN_21)
#define ENCODER_GPIO_E1A_IOMUX                                   (IOMUX_PINCM46)
/* Defines for E1B: GPIOA.22 with pinCMx 47 on package pin 18 */
#define ENCODER_GPIO_E1B_IIDX                               (DL_GPIO_IIDX_DIO22)
#define ENCODER_GPIO_E1B_PIN                                    (DL_GPIO_PIN_22)
#define ENCODER_GPIO_E1B_IOMUX                                   (IOMUX_PINCM47)
/* Defines for E2A: GPIOA.23 with pinCMx 53 on package pin 24 */
#define ENCODER_GPIO_E2A_IIDX                               (DL_GPIO_IIDX_DIO23)
#define ENCODER_GPIO_E2A_PIN                                    (DL_GPIO_PIN_23)
#define ENCODER_GPIO_E2A_IOMUX                                   (IOMUX_PINCM53)
/* Defines for E2B: GPIOA.24 with pinCMx 54 on package pin 25 */
#define ENCODER_GPIO_E2B_IIDX                               (DL_GPIO_IIDX_DIO24)
#define ENCODER_GPIO_E2B_PIN                                    (DL_GPIO_PIN_24)
#define ENCODER_GPIO_E2B_IOMUX                                   (IOMUX_PINCM54)
/* Port definition for Pin Group GRAY_GPIO */
#define GRAY_GPIO_PORT                                                   (GPIOB)

/* Defines for GRAY_CLK: GPIOB.2 with pinCMx 15 on package pin 50 */
#define GRAY_GPIO_GRAY_CLK_PIN                                   (DL_GPIO_PIN_2)
#define GRAY_GPIO_GRAY_CLK_IOMUX                                 (IOMUX_PINCM15)
/* Defines for DAT: GPIOB.3 with pinCMx 16 on package pin 51 */
#define GRAY_GPIO_DAT_PIN                                        (DL_GPIO_PIN_3)
#define GRAY_GPIO_DAT_IOMUX                                      (IOMUX_PINCM16)




/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_MOTOR_PWM_init(void);
void SYSCFG_DL_GYRO_UART_init(void);
void SYSCFG_DL_STM32_UART_init(void);

void SYSCFG_DL_SYSTICK_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */

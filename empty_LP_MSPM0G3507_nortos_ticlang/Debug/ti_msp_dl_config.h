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



/* Defines for PWM_LEFT */
#define PWM_LEFT_INST                                                      TIMG0
#define PWM_LEFT_INST_IRQHandler                                TIMG0_IRQHandler
#define PWM_LEFT_INST_INT_IRQN                                  (TIMG0_INT_IRQn)
#define PWM_LEFT_INST_CLK_FREQ                                          32000000
/* GPIO defines for channel 1 */
#define GPIO_PWM_LEFT_C1_PORT                                              GPIOA
#define GPIO_PWM_LEFT_C1_PIN                                      DL_GPIO_PIN_13
#define GPIO_PWM_LEFT_C1_IOMUX                                   (IOMUX_PINCM35)
#define GPIO_PWM_LEFT_C1_IOMUX_FUNC                  IOMUX_PINCM35_PF_TIMG0_CCP1
#define GPIO_PWM_LEFT_C1_IDX                                 DL_TIMER_CC_1_INDEX

/* Defines for PWM_RIGHT */
#define PWM_RIGHT_INST                                                     TIMA1
#define PWM_RIGHT_INST_IRQHandler                               TIMA1_IRQHandler
#define PWM_RIGHT_INST_INT_IRQN                                 (TIMA1_INT_IRQn)
#define PWM_RIGHT_INST_CLK_FREQ                                         32000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_RIGHT_C0_PORT                                             GPIOA
#define GPIO_PWM_RIGHT_C0_PIN                                     DL_GPIO_PIN_15
#define GPIO_PWM_RIGHT_C0_IOMUX                                  (IOMUX_PINCM37)
#define GPIO_PWM_RIGHT_C0_IOMUX_FUNC                 IOMUX_PINCM37_PF_TIMA1_CCP0
#define GPIO_PWM_RIGHT_C0_IDX                                DL_TIMER_CC_0_INDEX



/* Defines for CONTROL_TIMER */
#define CONTROL_TIMER_INST                                              (TIMG12)
#define CONTROL_TIMER_INST_IRQHandler                          TIMG12_IRQHandler
#define CONTROL_TIMER_INST_INT_IRQN                            (TIMG12_INT_IRQn)
#define CONTROL_TIMER_INST_LOAD_VALUE                                   (31999U)



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





/* Port definition for Pin Group START_KEY */
#define START_KEY_PORT                                                   (GPIOA)

/* Defines for KEY1: GPIOA.18 with pinCMx 40 on package pin 11 */
#define START_KEY_KEY1_PIN                                      (DL_GPIO_PIN_18)
#define START_KEY_KEY1_IOMUX                                     (IOMUX_PINCM40)
/* Port definition for Pin Group BLUE_LED_PORT */
#define BLUE_LED_PORT_PORT                                               (GPIOA)

/* Defines for BLUE_LED: GPIOA.7 with pinCMx 14 on package pin 49 */
#define BLUE_LED_PORT_BLUE_LED_PIN                               (DL_GPIO_PIN_7)
#define BLUE_LED_PORT_BLUE_LED_IOMUX                             (IOMUX_PINCM14)
/* Port definition for Pin Group BUZZER_PORT */
#define BUZZER_PORT_PORT                                                 (GPIOB)

/* Defines for BUZZER: GPIOB.26 with pinCMx 57 on package pin 28 */
#define BUZZER_PORT_BUZZER_PIN                                  (DL_GPIO_PIN_26)
#define BUZZER_PORT_BUZZER_IOMUX                                 (IOMUX_PINCM57)
/* Port definition for Pin Group OLED_SCL_GPIO */
#define OLED_SCL_GPIO_PORT                                               (GPIOB)

/* Defines for OLED_SCL: GPIOB.16 with pinCMx 33 on package pin 4 */
#define OLED_SCL_GPIO_OLED_SCL_PIN                              (DL_GPIO_PIN_16)
#define OLED_SCL_GPIO_OLED_SCL_IOMUX                             (IOMUX_PINCM33)
/* Port definition for Pin Group OLED_SDA_GPIO */
#define OLED_SDA_GPIO_PORT                                               (GPIOA)

/* Defines for OLED_SDA: GPIOA.17 with pinCMx 39 on package pin 10 */
#define OLED_SDA_GPIO_OLED_SDA_PIN                              (DL_GPIO_PIN_17)
#define OLED_SDA_GPIO_OLED_SDA_IOMUX                             (IOMUX_PINCM39)
/* Port definition for Pin Group MOTOR_CTRL */
#define MOTOR_CTRL_PORT                                                  (GPIOB)

/* Defines for MOTOR_A_IN1: GPIOB.4 with pinCMx 17 on package pin 52 */
#define MOTOR_CTRL_MOTOR_A_IN1_PIN                               (DL_GPIO_PIN_4)
#define MOTOR_CTRL_MOTOR_A_IN1_IOMUX                             (IOMUX_PINCM17)
/* Defines for MOTOR_A_IN2: GPIOB.5 with pinCMx 18 on package pin 53 */
#define MOTOR_CTRL_MOTOR_A_IN2_PIN                               (DL_GPIO_PIN_5)
#define MOTOR_CTRL_MOTOR_A_IN2_IOMUX                             (IOMUX_PINCM18)
/* Defines for MOTOR_B_IN1: GPIOB.6 with pinCMx 23 on package pin 58 */
#define MOTOR_CTRL_MOTOR_B_IN1_PIN                               (DL_GPIO_PIN_6)
#define MOTOR_CTRL_MOTOR_B_IN1_IOMUX                             (IOMUX_PINCM23)
/* Defines for MOTOR_B_IN2: GPIOB.7 with pinCMx 24 on package pin 59 */
#define MOTOR_CTRL_MOTOR_B_IN2_PIN                               (DL_GPIO_PIN_7)
#define MOTOR_CTRL_MOTOR_B_IN2_IOMUX                             (IOMUX_PINCM24)
/* Defines for MOTOR_STBY: GPIOB.12 with pinCMx 29 on package pin 64 */
#define MOTOR_CTRL_MOTOR_STBY_PIN                               (DL_GPIO_PIN_12)
#define MOTOR_CTRL_MOTOR_STBY_IOMUX                              (IOMUX_PINCM29)
/* Port definition for Pin Group GRAY_SERIAL */
#define GRAY_SERIAL_PORT                                                 (GPIOB)

/* Defines for GRAY_CLK: GPIOB.8 with pinCMx 25 on package pin 60 */
#define GRAY_SERIAL_GRAY_CLK_PIN                                 (DL_GPIO_PIN_8)
#define GRAY_SERIAL_GRAY_CLK_IOMUX                               (IOMUX_PINCM25)
/* Defines for GRAY_DAT: GPIOB.9 with pinCMx 26 on package pin 61 */
#define GRAY_SERIAL_GRAY_DAT_PIN                                 (DL_GPIO_PIN_9)
#define GRAY_SERIAL_GRAY_DAT_IOMUX                               (IOMUX_PINCM26)
/* Port definition for Pin Group ENCODER_IO */
#define ENCODER_IO_PORT                                                  (GPIOB)

/* Defines for ENCODER_E1A: GPIOB.17 with pinCMx 43 on package pin 14 */
// pins affected by this interrupt request:["ENCODER_E1A","ENCODER_E2A"]
#define ENCODER_IO_INT_IRQN                                     (GPIOB_INT_IRQn)
#define ENCODER_IO_INT_IIDX                     (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define ENCODER_IO_ENCODER_E1A_IIDX                         (DL_GPIO_IIDX_DIO17)
#define ENCODER_IO_ENCODER_E1A_PIN                              (DL_GPIO_PIN_17)
#define ENCODER_IO_ENCODER_E1A_IOMUX                             (IOMUX_PINCM43)
/* Defines for ENCODER_E1B: GPIOB.20 with pinCMx 48 on package pin 19 */
#define ENCODER_IO_ENCODER_E1B_PIN                              (DL_GPIO_PIN_20)
#define ENCODER_IO_ENCODER_E1B_IOMUX                             (IOMUX_PINCM48)
/* Defines for ENCODER_E2A: GPIOB.21 with pinCMx 49 on package pin 20 */
#define ENCODER_IO_ENCODER_E2A_IIDX                         (DL_GPIO_IIDX_DIO21)
#define ENCODER_IO_ENCODER_E2A_PIN                              (DL_GPIO_PIN_21)
#define ENCODER_IO_ENCODER_E2A_IOMUX                             (IOMUX_PINCM49)
/* Defines for ENCODER_E2B: GPIOB.27 with pinCMx 58 on package pin 29 */
#define ENCODER_IO_ENCODER_E2B_PIN                              (DL_GPIO_PIN_27)
#define ENCODER_IO_ENCODER_E2B_IOMUX                             (IOMUX_PINCM58)
/* Port definition for Pin Group MOTOR_TEST_KEYS */
#define MOTOR_TEST_KEYS_PORT                                             (GPIOB)

/* Defines for MOTOR_TEST_ALT: GPIOB.22 with pinCMx 50 on package pin 21 */
#define MOTOR_TEST_KEYS_MOTOR_TEST_ALT_PIN                      (DL_GPIO_PIN_22)
#define MOTOR_TEST_KEYS_MOTOR_TEST_ALT_IOMUX                     (IOMUX_PINCM50)
/* Defines for MOTOR_TEST_STOP: GPIOB.23 with pinCMx 51 on package pin 22 */
#define MOTOR_TEST_KEYS_MOTOR_TEST_STOP_PIN                     (DL_GPIO_PIN_23)
#define MOTOR_TEST_KEYS_MOTOR_TEST_STOP_IOMUX                    (IOMUX_PINCM51)
/* Defines for MOTOR_TEST_DISTANCE: GPIOB.24 with pinCMx 52 on package pin 23 */
#define MOTOR_TEST_KEYS_MOTOR_TEST_DISTANCE_PIN                 (DL_GPIO_PIN_24)
#define MOTOR_TEST_KEYS_MOTOR_TEST_DISTANCE_IOMUX                (IOMUX_PINCM52)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_LEFT_init(void);
void SYSCFG_DL_PWM_RIGHT_init(void);
void SYSCFG_DL_CONTROL_TIMER_init(void);
void SYSCFG_DL_GYRO_UART_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */

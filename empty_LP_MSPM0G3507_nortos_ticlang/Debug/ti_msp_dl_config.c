/*
 * Copyright (c) 2023, Texas Instruments Incorporated
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
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gPWM_RIGHTBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_PWM_LEFT_init();
    SYSCFG_DL_PWM_RIGHT_init();
    SYSCFG_DL_CONTROL_TIMER_init();
    SYSCFG_DL_GYRO_UART_init();
    /* Ensure backup structures have no valid state */
	gPWM_RIGHTBackup.backupRdy 	= false;



}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_saveConfiguration(PWM_RIGHT_INST, &gPWM_RIGHTBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(PWM_RIGHT_INST, &gPWM_RIGHTBackup, false);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerG_reset(PWM_LEFT_INST);
    DL_TimerA_reset(PWM_RIGHT_INST);
    DL_TimerG_reset(CONTROL_TIMER_INST);
    DL_UART_Main_reset(GYRO_UART_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerG_enablePower(PWM_LEFT_INST);
    DL_TimerA_enablePower(PWM_RIGHT_INST);
    DL_TimerG_enablePower(CONTROL_TIMER_INST);
    DL_UART_Main_enablePower(GYRO_UART_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_LEFT_C1_IOMUX,GPIO_PWM_LEFT_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_LEFT_C1_PORT, GPIO_PWM_LEFT_C1_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_RIGHT_C0_IOMUX,GPIO_PWM_RIGHT_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_RIGHT_C0_PORT, GPIO_PWM_RIGHT_C0_PIN);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_GYRO_UART_IOMUX_TX, GPIO_GYRO_UART_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_GYRO_UART_IOMUX_RX, GPIO_GYRO_UART_IOMUX_RX_FUNC);

    DL_GPIO_initDigitalInputFeatures(START_KEY_KEY1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutput(BLUE_LED_PORT_BLUE_LED_IOMUX);

    DL_GPIO_initDigitalOutput(BUZZER_PORT_BUZZER_IOMUX);

    DL_GPIO_initDigitalInputFeatures(OLED_SCL_GPIO_OLED_SCL_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(OLED_SDA_GPIO_OLED_SDA_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutput(MOTOR_CTRL_MOTOR_A_IN1_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_CTRL_MOTOR_A_IN2_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_CTRL_MOTOR_B_IN1_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_CTRL_MOTOR_B_IN2_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_CTRL_MOTOR_STBY_IOMUX);

    DL_GPIO_initDigitalOutput(GRAY_SERIAL_GRAY_CLK_IOMUX);

    DL_GPIO_initDigitalInputFeatures(GRAY_SERIAL_GRAY_DAT_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(ENCODER_IO_ENCODER_E1A_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(ENCODER_IO_ENCODER_E1B_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(ENCODER_IO_ENCODER_E2A_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(ENCODER_IO_ENCODER_E2B_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(MOTOR_TEST_KEYS_MOTOR_TEST_ALT_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(MOTOR_TEST_KEYS_MOTOR_TEST_STOP_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(MOTOR_TEST_KEYS_MOTOR_TEST_DISTANCE_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_setPins(GPIOA, BLUE_LED_PORT_BLUE_LED_PIN);
    DL_GPIO_enableOutput(GPIOA, BLUE_LED_PORT_BLUE_LED_PIN);
    DL_GPIO_clearPins(GPIOB, BUZZER_PORT_BUZZER_PIN |
		MOTOR_CTRL_MOTOR_A_IN1_PIN |
		MOTOR_CTRL_MOTOR_A_IN2_PIN |
		MOTOR_CTRL_MOTOR_B_IN1_PIN |
		MOTOR_CTRL_MOTOR_B_IN2_PIN |
		MOTOR_CTRL_MOTOR_STBY_PIN |
		GRAY_SERIAL_GRAY_CLK_PIN);
    DL_GPIO_enableOutput(GPIOB, BUZZER_PORT_BUZZER_PIN |
		MOTOR_CTRL_MOTOR_A_IN1_PIN |
		MOTOR_CTRL_MOTOR_A_IN2_PIN |
		MOTOR_CTRL_MOTOR_B_IN1_PIN |
		MOTOR_CTRL_MOTOR_B_IN2_PIN |
		MOTOR_CTRL_MOTOR_STBY_PIN |
		GRAY_SERIAL_GRAY_CLK_PIN);
    DL_GPIO_setUpperPinsPolarity(GPIOB, DL_GPIO_PIN_17_EDGE_RISE |
		DL_GPIO_PIN_21_EDGE_RISE);
    DL_GPIO_clearInterruptStatus(GPIOB, ENCODER_IO_ENCODER_E1A_PIN |
		ENCODER_IO_ENCODER_E2A_PIN);
    DL_GPIO_enableInterrupt(GPIOB, ENCODER_IO_ENCODER_E1A_PIN |
		ENCODER_IO_ENCODER_E2A_PIN);

}


SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    /* Set default configuration */
    DL_SYSCTL_disableHFXT();
    DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_1);
    DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);

}


/*
 * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gPWM_LEFTClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gPWM_LEFTConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 1600,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_LEFT_init(void) {

    DL_TimerG_setClockConfig(
        PWM_LEFT_INST, (DL_TimerG_ClockConfig *) &gPWM_LEFTClockConfig);

    DL_TimerG_initPWMMode(
        PWM_LEFT_INST, (DL_TimerG_PWMConfig *) &gPWM_LEFTConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerG_setCounterControl(PWM_LEFT_INST,DL_TIMER_CZC_CCCTL1_ZCOND,DL_TIMER_CAC_CCCTL1_ACOND,DL_TIMER_CLC_CCCTL1_LCOND);

    DL_TimerG_setCaptureCompareOutCtl(PWM_LEFT_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_LEFT_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_LEFT_INST, 0, DL_TIMER_CC_1_INDEX);

    DL_TimerG_enableClock(PWM_LEFT_INST);


    
    DL_TimerG_setCCPDirection(PWM_LEFT_INST , DL_TIMER_CC1_OUTPUT );


}
/*
 * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerA_ClockConfig gPWM_RIGHTClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerA_PWMConfig gPWM_RIGHTConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 1600,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_RIGHT_init(void) {

    DL_TimerA_setClockConfig(
        PWM_RIGHT_INST, (DL_TimerA_ClockConfig *) &gPWM_RIGHTClockConfig);

    DL_TimerA_initPWMMode(
        PWM_RIGHT_INST, (DL_TimerA_PWMConfig *) &gPWM_RIGHTConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerA_setCounterControl(PWM_RIGHT_INST,DL_TIMER_CZC_CCCTL0_ZCOND,DL_TIMER_CAC_CCCTL0_ACOND,DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerA_setCaptureCompareOutCtl(PWM_RIGHT_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_0_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(PWM_RIGHT_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_0_INDEX);
    DL_TimerA_setCaptureCompareValue(PWM_RIGHT_INST, 0, DL_TIMER_CC_0_INDEX);

    DL_TimerA_enableClock(PWM_RIGHT_INST);


    
    DL_TimerA_setCCPDirection(PWM_RIGHT_INST , DL_TIMER_CC0_OUTPUT );


}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gCONTROL_TIMERClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * CONTROL_TIMER_INST_LOAD_VALUE = (1ms * 32000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gCONTROL_TIMERTimerConfig = {
    .period     = CONTROL_TIMER_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_CONTROL_TIMER_init(void) {

    DL_TimerG_setClockConfig(CONTROL_TIMER_INST,
        (DL_TimerG_ClockConfig *) &gCONTROL_TIMERClockConfig);

    DL_TimerG_initTimerMode(CONTROL_TIMER_INST,
        (DL_TimerG_TimerConfig *) &gCONTROL_TIMERTimerConfig);
    DL_TimerG_enableClock(CONTROL_TIMER_INST);





}


static const DL_UART_Main_ClockConfig gGYRO_UARTClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gGYRO_UARTConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_GYRO_UART_init(void)
{
    DL_UART_Main_setClockConfig(GYRO_UART_INST, (DL_UART_Main_ClockConfig *) &gGYRO_UARTClockConfig);

    DL_UART_Main_init(GYRO_UART_INST, (DL_UART_Main_Config *) &gGYRO_UARTConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(GYRO_UART_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(GYRO_UART_INST, GYRO_UART_IBRD_32_MHZ_115200_BAUD, GYRO_UART_FBRD_32_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(GYRO_UART_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);


    DL_UART_Main_enable(GYRO_UART_INST);
}


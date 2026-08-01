#include "Stepper.h"
#include "misc.h"

/*
 * D36A channel A:
 *   PA5 -> EN1, high enable / low sleep
 *   PA6 -> ST1, TIM3_CH1 hardware pulse, rising-edge valid
 *   PA7 -> DIR1, high is the tested positive direction
 *
 * TIM2 provides the real 1 ms motion-planner tick. TIM3 emits exactly one
 * PWM2 pulse per one-pulse cycle. Position is counted only after that physical
 * pulse completes, so OLED position is no longer advanced by a blocking loop.
 */
#define STEPPER_GPIO                GPIOA
#define STEPPER_EN_PIN              GPIO_Pin_5
#define STEPPER_STEP_PIN            GPIO_Pin_6
#define STEPPER_DIR_PIN             GPIO_Pin_7
#define STEPPER_TIMER_BASE_HZ        1000000UL
#define STEPPER_PULSE_HIGH_US        10U
#define STEPPER_DIR_SETUP_US         1000U

static volatile uint8_t g_enabled;
static volatile uint8_t g_pulse_running;
static volatile int8_t g_active_direction;
static volatile int32_t g_command_position;
static volatile int32_t g_target_position;
static volatile int32_t g_command_speed_milli_sps;
static volatile int32_t g_requested_speed_sps;
static volatile uint32_t g_tick_ms;

static int32_t Stepper_Abs32(int32_t value)
{
	return (value < 0) ? -value : value;
}

static int8_t Stepper_Sign32(int32_t value)
{
	if (value > 0)
	{
		return 1;
	}
	if (value < 0)
	{
		return -1;
	}
	return 0;
}

static int32_t Stepper_Clamp32(
	int32_t value, int32_t minimum, int32_t maximum)
{
	if (value < minimum)
	{
		return minimum;
	}
	if (value > maximum)
	{
		return maximum;
	}
	return value;
}

static void Stepper_ConfigureStepPin(uint8_t timer_output)
{
	GPIO_InitTypeDef gpio;

	gpio.GPIO_Pin = STEPPER_STEP_PIN;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = (timer_output != 0U) ?
		GPIO_Mode_AF_PP : GPIO_Mode_Out_PP;
	GPIO_Init(STEPPER_GPIO, &gpio);
	if (timer_output == 0U)
	{
		GPIO_ResetBits(STEPPER_GPIO, STEPPER_STEP_PIN);
	}
}

static void Stepper_WriteDirection(int8_t direction)
{
	if (direction > 0)
	{
		GPIO_SetBits(STEPPER_GPIO, STEPPER_DIR_PIN);
	}
	else
	{
		GPIO_ResetBits(STEPPER_GPIO, STEPPER_DIR_PIN);
	}
}

static void Stepper_DisarmPulse(void)
{
	TIM3->CR1 &= (uint16_t)~TIM_CR1_CEN;
	TIM3->CNT = 0U;
	TIM3->SR = (uint16_t)~(TIM_SR_UIF | TIM_SR_CC1IF);
	g_pulse_running = 0U;
}

static void Stepper_ArmPulse(
	uint32_t period_us, uint32_t low_before_rising_us)
{
	if (period_us <= STEPPER_PULSE_HIGH_US)
	{
		period_us = STEPPER_PULSE_HIGH_US + 1U;
	}
	if (period_us > 65535U)
	{
		period_us = 65535U;
	}
	if (low_before_rising_us == 0U)
	{
		low_before_rising_us = 1U;
	}
	if (low_before_rising_us >= period_us)
	{
		low_before_rising_us =
			period_us - STEPPER_PULSE_HIGH_US;
	}

	TIM3->CR1 &= (uint16_t)~TIM_CR1_CEN;
	TIM3->ARR = (uint16_t)(period_us - 1U);
	TIM3->CCR1 = (uint16_t)low_before_rising_us;
	TIM3->EGR = TIM_EGR_UG;
	TIM3->CNT = 0U;
	TIM3->SR = (uint16_t)~(TIM_SR_UIF | TIM_SR_CC1IF);
	g_pulse_running = 1U;
	TIM3->CR1 |= TIM_CR1_CEN;
}

static uint8_t Stepper_DirectionAllowed(int8_t direction)
{
	if ((direction > 0) &&
		((g_command_position >= g_target_position) ||
		 (g_command_position >= STEPPER_SOFT_LIMIT_STEPS)))
	{
		return 0U;
	}
	if ((direction < 0) &&
		((g_command_position <= g_target_position) ||
		 (g_command_position <= -STEPPER_SOFT_LIMIT_STEPS)))
	{
		return 0U;
	}
	return 1U;
}

static void Stepper_StartForRequestedSpeed(void)
{
	int8_t requested_direction;

	if ((g_enabled == 0U) || (g_requested_speed_sps == 0) ||
		(g_pulse_running != 0U))
	{
		return;
	}

	requested_direction = Stepper_Sign32(g_requested_speed_sps);
	if (Stepper_DirectionAllowed(requested_direction) == 0U)
	{
		g_requested_speed_sps = 0;
		g_command_speed_milli_sps = 0;
		return;
	}

	g_active_direction = requested_direction;
	Stepper_WriteDirection(g_active_direction);
	Stepper_ArmPulse(
		STEPPER_DIR_SETUP_US + STEPPER_PULSE_HIGH_US,
		STEPPER_DIR_SETUP_US);
}

void Stepper_Init(void)
{
	GPIO_InitTypeDef gpio;
	TIM_TimeBaseInitTypeDef timebase;
	TIM_OCInitTypeDef output_compare;
	NVIC_InitTypeDef nvic;
	uint16_t prescaler;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(
		RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3,
		ENABLE);

	GPIO_ResetBits(
		STEPPER_GPIO,
		STEPPER_EN_PIN | STEPPER_STEP_PIN | STEPPER_DIR_PIN);
	gpio.GPIO_Pin = STEPPER_EN_PIN | STEPPER_DIR_PIN;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(STEPPER_GPIO, &gpio);
	Stepper_ConfigureStepPin(1U);

	/* TIM3: 1 MHz, PWM2 one-pulse STEP generator on PA6/CH1. */
	prescaler = (uint16_t)(
		(SystemCoreClock / STEPPER_TIMER_BASE_HZ) - 1U);
	timebase.TIM_Prescaler = prescaler;
	timebase.TIM_CounterMode = TIM_CounterMode_Up;
	timebase.TIM_Period = 1009U;
	timebase.TIM_ClockDivision = TIM_CKD_DIV1;
	timebase.TIM_RepetitionCounter = 0U;
	TIM_TimeBaseInit(TIM3, &timebase);

	output_compare.TIM_OCMode = TIM_OCMode_PWM2;
	output_compare.TIM_OutputState = TIM_OutputState_Enable;
	output_compare.TIM_Pulse = STEPPER_DIR_SETUP_US;
	output_compare.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &output_compare);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	TIM_SelectOnePulseMode(TIM3, TIM_OPMode_Single);
	TIM_UpdateRequestConfig(TIM3, TIM_UpdateSource_Regular);
	TIM3->CR1 &= (uint16_t)~TIM_CR1_CEN;
	TIM3->EGR = TIM_EGR_UG;
	TIM3->CNT = 0U;
	TIM3->SR = (uint16_t)~(TIM_SR_UIF | TIM_SR_CC1IF);
	TIM3->DIER = TIM_DIER_UIE;

	nvic.NVIC_IRQChannel = TIM3_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0U;
	nvic.NVIC_IRQChannelSubPriority = 0U;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	/* TIM2: fixed 1 ms motion-planner interrupt. */
	timebase.TIM_Prescaler = prescaler;
	timebase.TIM_CounterMode = TIM_CounterMode_Up;
	timebase.TIM_Period = 999U;
	timebase.TIM_ClockDivision = TIM_CKD_DIV1;
	timebase.TIM_RepetitionCounter = 0U;
	TIM_TimeBaseInit(TIM2, &timebase);
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	nvic.NVIC_IRQChannel = TIM2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 1U;
	nvic.NVIC_IRQChannelSubPriority = 0U;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	g_enabled = 0U;
	g_pulse_running = 0U;
	g_active_direction = 1;
	g_command_position = 0;
	g_target_position = 0;
	g_command_speed_milli_sps = 0;
	g_requested_speed_sps = 0;
	g_tick_ms = 0U;
	TIM_Cmd(TIM2, ENABLE);
}

void Stepper_SetEnabled(uint8_t enabled)
{
	if (enabled != 0U)
	{
		Stepper_ConfigureStepPin(1U);
		TIM_CCxCmd(TIM3, TIM_Channel_1, TIM_CCx_Enable);
		GPIO_SetBits(STEPPER_GPIO, STEPPER_EN_PIN);
		g_enabled = 1U;
	}
	else
	{
		g_requested_speed_sps = 0;
		g_command_speed_milli_sps = 0;
		g_target_position = g_command_position;
		Stepper_DisarmPulse();
		TIM_CCxCmd(TIM3, TIM_Channel_1, TIM_CCx_Disable);
		Stepper_ConfigureStepPin(0U);
		GPIO_ResetBits(STEPPER_GPIO, STEPPER_EN_PIN);
		g_enabled = 0U;
	}
}

uint8_t Stepper_IsEnabled(void)
{
	return g_enabled;
}

uint8_t Stepper_MoveRelative(int32_t steps)
{
	int32_t target;

	if ((g_enabled == 0U) || (steps == 0))
	{
		return 0U;
	}
	target = g_target_position + steps;
	return Stepper_SetTargetPosition(target);
}

uint8_t Stepper_SetTargetPosition(int32_t target_steps)
{
	if ((g_enabled == 0U) ||
		(target_steps > STEPPER_SOFT_LIMIT_STEPS) ||
		(target_steps < -STEPPER_SOFT_LIMIT_STEPS))
	{
		return 0U;
	}
	g_target_position = target_steps;
	return 1U;
}

void Stepper_Stop(void)
{
	g_target_position = g_command_position;
	g_requested_speed_sps = 0;
	g_command_speed_milli_sps = 0;
}

void Stepper_OnTim2TickIrq(void)
{
	int32_t error;
	int32_t target_speed_sps;
	int32_t target_speed_milli_sps;
	int32_t speed_delta_milli_sps;
	int8_t target_sign;
	int8_t current_sign;

	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == RESET)
	{
		return;
	}
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	g_tick_ms++;

	if (g_enabled == 0U)
	{
		g_requested_speed_sps = 0;
		g_command_speed_milli_sps = 0;
		return;
	}

	error = g_target_position - g_command_position;
	target_sign = Stepper_Sign32(error);
	current_sign = Stepper_Sign32(g_command_speed_milli_sps);

	if (error == 0)
	{
		target_speed_sps = 0;
	}
	else if ((current_sign != 0) && (target_sign != current_sign))
	{
		/* Always decelerate to zero before changing DIR. */
		target_speed_sps = 0;
	}
	else
	{
		target_speed_sps =
			Stepper_Abs32(error) *
			STEPPER_FOLLOW_GAIN_SPS_PER_STEP;
		target_speed_sps = Stepper_Clamp32(
			target_speed_sps,
			STEPPER_MIN_SPEED_SPS,
			STEPPER_MAX_SPEED_SPS);
		target_speed_sps *= target_sign;
	}

	target_speed_milli_sps = target_speed_sps * 1000;
	/*
	 * speed is stored in 1/1000 step/s. For a 1 ms planner interval,
	 * acceleration [step/s^2] is numerically the allowed milli-step/s
	 * change per tick.
	 */
	speed_delta_milli_sps = STEPPER_ACCEL_SPS2;
	if (g_command_speed_milli_sps < target_speed_milli_sps)
	{
		g_command_speed_milli_sps += speed_delta_milli_sps;
		if (g_command_speed_milli_sps > target_speed_milli_sps)
		{
			g_command_speed_milli_sps = target_speed_milli_sps;
		}
	}
	else if (g_command_speed_milli_sps > target_speed_milli_sps)
	{
		g_command_speed_milli_sps -= speed_delta_milli_sps;
		if (g_command_speed_milli_sps < target_speed_milli_sps)
		{
			g_command_speed_milli_sps = target_speed_milli_sps;
		}
	}

	/*
	 * Round toward zero. Speeds below the configured minimum are treated as
	 * stopped during acceleration/deceleration; the planner keeps ramping.
	 */
	g_requested_speed_sps =
		g_command_speed_milli_sps / 1000;
	if (Stepper_Abs32(g_requested_speed_sps) <
		STEPPER_MIN_SPEED_SPS)
	{
		g_requested_speed_sps = 0;
	}

	if (g_pulse_running == 0U)
	{
		Stepper_StartForRequestedSpeed();
	}
}

void Stepper_OnTim3PulseIrq(void)
{
	int32_t speed_sps;
	int8_t requested_direction;
	uint32_t period_us;

	if ((TIM3->SR & TIM_SR_UIF) == 0U)
	{
		return;
	}
	TIM3->SR = (uint16_t)~(TIM_SR_UIF | TIM_SR_CC1IF);
	if (g_pulse_running == 0U)
	{
		return;
	}

	g_pulse_running = 0U;
	g_command_position += (int32_t)g_active_direction;

	if ((g_command_position >= STEPPER_SOFT_LIMIT_STEPS) ||
		(g_command_position <= -STEPPER_SOFT_LIMIT_STEPS) ||
		(g_command_position == g_target_position))
	{
		g_requested_speed_sps = 0;
		g_command_speed_milli_sps = 0;
		return;
	}

	speed_sps = g_requested_speed_sps;
	requested_direction = Stepper_Sign32(speed_sps);
	if ((speed_sps == 0) ||
		(requested_direction != g_active_direction) ||
		(Stepper_DirectionAllowed(requested_direction) == 0U))
	{
		return;
	}

	period_us = (STEPPER_TIMER_BASE_HZ +
		(uint32_t)Stepper_Abs32(speed_sps) - 1U) /
		(uint32_t)Stepper_Abs32(speed_sps);
	Stepper_ArmPulse(
		period_us, period_us - STEPPER_PULSE_HIGH_US);
}

int32_t Stepper_GetCommandPosition(void)
{
	return g_command_position;
}

int32_t Stepper_GetTargetPosition(void)
{
	return g_target_position;
}

int32_t Stepper_GetCommandSpeedSps(void)
{
	return g_requested_speed_sps;
}

uint8_t Stepper_IsRunning(void)
{
	return g_pulse_running;
}

uint32_t Stepper_GetTickMs(void)
{
	return g_tick_ms;
}

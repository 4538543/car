#include "stm32f10x.h"
#include "Delay.h"

/*
 * Eight-key module connections used in phase 1:
 *   K1 -> PB12, K2 -> PB13, K3 -> PB14
 * The module is powered from 3.3 V and has onboard pull-up resistors.
 * A pressed key therefore reads as logic low.
 */
void Key_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =
		GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static uint8_t Key_ReadPressed(uint16_t pin)
{
	if (GPIO_ReadInputDataBit(GPIOB, pin) != Bit_RESET)
	{
		return 0;
	}

	Delay_ms(20);
	if (GPIO_ReadInputDataBit(GPIOB, pin) != Bit_RESET)
	{
		return 0;
	}

	while (GPIO_ReadInputDataBit(GPIOB, pin) == Bit_RESET)
	{
		/* Phase-1 test waits for release to produce one event per press. */
	}
	Delay_ms(20);
	return 1;
}

uint8_t Key_GetNum(void)
{
	if (Key_ReadPressed(GPIO_Pin_12))
	{
		return 1;
	}
	if (Key_ReadPressed(GPIO_Pin_13))
	{
		return 2;
	}
	if (Key_ReadPressed(GPIO_Pin_14))
	{
		return 3;
	}

	return 0;
}

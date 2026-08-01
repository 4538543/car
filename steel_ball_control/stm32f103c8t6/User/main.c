#include "stm32f10x.h"
#include "OLED.h"
#include "Key.h"
#include "Stepper.h"
#include "MaixcamUart.h"

int main(void)
{
	uint8_t key;
	uint32_t last_display_ms;
	MaixcamBallData ball;

	OLED_Init();
	Key_Init();
	Stepper_Init();
	MaixcamUart_Init();

	OLED_Clear();
	OLED_ShowString(1, 1, "TIMER STEP TEST");
	OLED_ShowString(2, 1, "P:+000 T:+000");
	OLED_ShowString(3, 1, "V:+000 EN:0");
	OLED_ShowString(4, 1, "K1+ K2- K3EN");
	last_display_ms = 0U;

	while (1)
	{
		/* Keep draining vision UART while motor timing runs in hardware. */
		MaixcamUart_Poll(&ball);

		key = Key_GetNum();
		if (key == 1U)
		{
			Stepper_MoveRelative(STEPPER_TEST_STEPS);
		}
		else if (key == 2U)
		{
			Stepper_MoveRelative(-STEPPER_TEST_STEPS);
		}
		else if (key == 3U)
		{
			Stepper_SetEnabled(!Stepper_IsEnabled());
		}

		if ((uint32_t)(Stepper_GetTickMs() - last_display_ms) >=
			100U)
		{
			last_display_ms = Stepper_GetTickMs();
			OLED_ShowSignedNum(
				2, 3, Stepper_GetCommandPosition(), 3);
			OLED_ShowSignedNum(
				2, 10, Stepper_GetTargetPosition(), 3);
			OLED_ShowSignedNum(
				3, 3, Stepper_GetCommandSpeedSps(), 3);
			OLED_ShowNum(
				3, 11, Stepper_IsEnabled(), 1);
		}
	}
}

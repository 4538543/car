#include "stm32f10x.h"
#include "OLED.h"
#include "Key.h"
#include "Stepper.h"
#include "MaixcamUart.h"
#include "DemoControl.h"

static char *StateText(DemoState state)
{
	switch (state)
	{
		case DEMO_STATE_WAIT_VISION:
			return "WAIT ";
		case DEMO_STATE_TO_PLUS5:
			return "TO+5 ";
		case DEMO_STATE_TO_MINUS5:
			return "TO-5 ";
		case DEMO_STATE_HOLD_MINUS5:
			return "HOLD ";
		case DEMO_STATE_HOLD_CENTER:
			return "CENTR";
		case DEMO_STATE_FAULT:
			return "FAULT";
		default:
			return "IDLE ";
	}
}

int main(void)
{
	uint8_t key;
	uint32_t now_ms;
	uint32_t last_display_ms;
	uint16_t last_received_position;
	uint8_t has_received_position;
	MaixcamBallData ball;
	DemoStatus status;

	OLED_Init();
	Key_Init();
	Stepper_Init();
	MaixcamUart_Init();
	DemoControl_Init();

	OLED_Clear();
	OLED_ShowString(1, 1, "S:IDLE  E:0");
	OLED_ShowString(2, 1, "B:---- T:----");
	OLED_ShowString(3, 1, "M:+000 P:+000");
	OLED_ShowString(4, 1, "R:00000 E:000");
	last_display_ms = 0U;
	last_received_position = 0U;
	has_received_position = 0U;

	while (1)
	{
		now_ms = Stepper_GetTickMs();
		while (MaixcamUart_Poll(&ball) != 0U)
		{
			/*
			 * Display the latest decoded coordinate directly. Do not let the
			 * task-layer validity decision hide UART data that was received
			 * correctly (including TRACK_HELD frames).
			 */
			if (ball.position <= 1000U)
			{
				last_received_position = ball.position;
				has_received_position = 1U;
			}
			DemoControl_OnVisionFrame(&ball, now_ms);
		}
		DemoControl_Service(now_ms);

		key = Key_GetNum();
		if (key == 1U)
		{
			if (DemoControl_IsActive() != 0U)
			{
				DemoControl_Abort();
			}
			else
			{
				DemoControl_Start(now_ms);
			}
		}
		else if (key == 2U)
		{
			if (DemoControl_IsActive() != 0U)
			{
				DemoControl_Abort();
			}
			else
			{
				DemoControl_StartCenter(now_ms);
			}
		}
		else if (key == 3U)
		{
			if (DemoControl_IsActive() != 0U)
			{
				DemoControl_Abort();
			}
			Stepper_SetEnabled(!Stepper_IsEnabled());
		}

		if ((uint32_t)(now_ms - last_display_ms) >= 100U)
		{
			last_display_ms = now_ms;
			status = DemoControl_GetStatus();
			OLED_ShowString(1, 3, StateText(status.state));
			OLED_ShowNum(1, 11, Stepper_IsEnabled(), 1);
			if (has_received_position != 0U)
			{
				OLED_ShowNum(
					2, 3,
					last_received_position, 4);
			}
			else
			{
				OLED_ShowString(2, 3, "----");
			}
			OLED_ShowNum(
				2, 10, status.target_normalized, 4);
			OLED_ShowSignedNum(
				3, 3, status.desired_motor_steps, 3);
			OLED_ShowSignedNum(
				3, 10,
				Stepper_GetCommandPosition(), 3);
			OLED_ShowNum(
				4, 3,
				MaixcamUart_GetValidFrameCount() % 100000U, 5);
			OLED_ShowNum(
				4, 11,
				MaixcamUart_GetCrcErrorCount() % 1000U, 3);
		}
	}
}

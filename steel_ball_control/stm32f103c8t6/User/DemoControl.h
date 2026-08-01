#ifndef __DEMO_CONTROL_H
#define __DEMO_CONTROL_H

#include "stm32f10x.h"
#include "MaixcamUart.h"

typedef enum
{
	DEMO_STATE_IDLE = 0,
	DEMO_STATE_WAIT_VISION,
	DEMO_STATE_TO_PLUS5,
	DEMO_STATE_TO_MINUS5,
	DEMO_STATE_HOLD_MINUS5,
	DEMO_STATE_HOLD_CENTER,
	DEMO_STATE_HOLD_CAPTURED,
	DEMO_STATE_FAULT
} DemoState;

typedef struct
{
	DemoState state;
	int32_t ball_position_milli_cm;
	int32_t ball_velocity_milli_cmps;
	int32_t target_milli_cm;
	int32_t desired_motor_steps;
	uint16_t ball_position_normalized;
	uint16_t target_normalized;
	uint32_t elapsed_ms;
	uint8_t vision_valid;
	uint8_t passed_5_seconds;
} DemoStatus;

void DemoControl_Init(void);
void DemoControl_Start(uint32_t now_ms);
void DemoControl_StartCenter(uint32_t now_ms);
void DemoControl_StartCapturedTarget(
	uint16_t target_normalized, uint32_t now_ms);
void DemoControl_Abort(void);
void DemoControl_OnVisionFrame(
	const MaixcamBallData *ball, uint32_t now_ms);
void DemoControl_Service(uint32_t now_ms);
DemoStatus DemoControl_GetStatus(void);
uint8_t DemoControl_IsActive(void);

#endif

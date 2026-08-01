#ifndef __BALL_CONTROL_H
#define __BALL_CONTROL_H

#include "stm32f10x.h"

typedef struct
{
	int32_t filtered_position_milli_cm;
	int32_t filtered_velocity_milli_cmps;
	int32_t target_velocity_milli_cmps;
	int32_t desired_command_position_steps;
} BallControlOutput;

void BallControl_Reset(int32_t measured_position_milli_cm);
void BallControl_ResetIntegrators(void);
BallControlOutput BallControl_Update(
	int32_t measured_position_milli_cm,
	int32_t target_position_milli_cm,
	int32_t outer_kp_milli_per_s,
	int32_t inner_kp_milli_step_per_cmps,
	uint32_t dt_ms);

#endif

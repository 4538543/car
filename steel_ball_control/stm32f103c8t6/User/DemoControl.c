#include "AppConfig.h"
#include "BallControl.h"
#include "DemoControl.h"
#include "Stepper.h"

#define FLAG_VALID          0x01U
#define FLAG_CALIBRATED     0x04U

static DemoStatus g_status;
static uint32_t g_start_ms;
static uint32_t g_last_valid_local_ms;
static uint32_t g_last_camera_timestamp_ms;
static uint32_t g_arrival_stable_ms;
static uint8_t g_recovery_frames;
static uint8_t g_has_sample;
static DemoState g_state_after_vision;

static int32_t Abs32(int32_t value)
{
	return (value < 0) ? -value : value;
}

static int32_t PositionToMilliCm(uint16_t position)
{
	return ((int32_t)position -
		APP_VISION_CENTER_POSITION) *
		APP_VISION_MILLI_CM_PER_UNIT;
}

static uint8_t FrameUsable(const MaixcamBallData *ball)
{
	if ((ball == 0) ||
		(ball->available == 0U) ||
		((ball->flags & FLAG_VALID) == 0U) ||
		((ball->flags & FLAG_CALIBRATED) == 0U) ||
		(ball->position > 1000U))
	{
		return 0U;
	}
	return 1U;
}

static void EnterFault(void)
{
	g_status.state = DEMO_STATE_FAULT;
	g_status.vision_valid = 0U;
	g_status.desired_motor_steps = 0;
	g_status.target_milli_cm = 0;
	g_status.target_normalized =
		APP_VISION_CENTER_POSITION;
	BallControl_ResetIntegrators();
	Stepper_SetTargetPosition(0);
}

void DemoControl_Init(void)
{
	g_status.state = DEMO_STATE_IDLE;
	g_status.ball_position_milli_cm = 0;
	g_status.ball_velocity_milli_cmps = 0;
	g_status.target_milli_cm = 0;
	g_status.desired_motor_steps = 0;
	g_status.ball_position_normalized =
		APP_VISION_CENTER_POSITION;
	g_status.target_normalized =
		APP_VISION_CENTER_POSITION;
	g_status.elapsed_ms = 0U;
	g_status.vision_valid = 0U;
	g_status.passed_5_seconds = 0U;
	g_start_ms = 0U;
	g_last_valid_local_ms = 0U;
	g_last_camera_timestamp_ms = 0U;
	g_arrival_stable_ms = 0U;
	g_recovery_frames = 0U;
	g_has_sample = 0U;
	g_state_after_vision = DEMO_STATE_TO_PLUS5;
}

void DemoControl_Start(uint32_t now_ms)
{
	if (Stepper_IsEnabled() == 0U)
	{
		return;
	}

	g_status.state = DEMO_STATE_WAIT_VISION;
	g_status.target_milli_cm =
		APP_TARGET_PLUS5_MILLI_CM;
	g_status.target_normalized =
		APP_VISION_PLUS5_POSITION;
	g_status.desired_motor_steps = 0;
	g_status.elapsed_ms = 0U;
	g_status.vision_valid = 0U;
	g_status.passed_5_seconds = 0U;
	g_start_ms = now_ms;
	g_last_valid_local_ms = now_ms;
	g_arrival_stable_ms = 0U;
	g_recovery_frames = 0U;
	g_has_sample = 0U;
	g_state_after_vision = DEMO_STATE_TO_PLUS5;
	BallControl_ResetIntegrators();
	Stepper_SetTargetPosition(0);
}

void DemoControl_StartCenter(uint32_t now_ms)
{
	if (Stepper_IsEnabled() == 0U)
	{
		return;
	}

	g_status.state = DEMO_STATE_WAIT_VISION;
	g_status.target_milli_cm = 0;
	g_status.target_normalized =
		APP_VISION_CENTER_POSITION;
	g_status.desired_motor_steps = 0;
	g_status.elapsed_ms = 0U;
	g_status.vision_valid = 0U;
	g_status.passed_5_seconds = 0U;
	g_start_ms = now_ms;
	g_last_valid_local_ms = now_ms;
	g_arrival_stable_ms = 0U;
	g_recovery_frames = 0U;
	g_has_sample = 0U;
	g_state_after_vision = DEMO_STATE_HOLD_CENTER;
	BallControl_ResetIntegrators();
	Stepper_SetTargetPosition(0);
}

void DemoControl_StartCapturedTarget(
	uint16_t target_normalized, uint32_t now_ms)
{
	if ((Stepper_IsEnabled() == 0U) ||
		(target_normalized > 1000U))
	{
		return;
	}

	g_status.state = DEMO_STATE_WAIT_VISION;
	g_status.target_milli_cm =
		PositionToMilliCm(target_normalized);
	g_status.target_normalized = target_normalized;
	g_status.desired_motor_steps = 0;
	g_status.elapsed_ms = 0U;
	g_status.vision_valid = 0U;
	g_status.passed_5_seconds = 0U;
	g_start_ms = now_ms;
	g_last_valid_local_ms = now_ms;
	g_arrival_stable_ms = 0U;
	g_recovery_frames = 0U;
	g_has_sample = 0U;
	g_state_after_vision = DEMO_STATE_HOLD_CAPTURED;
	BallControl_ResetIntegrators();
	Stepper_SetTargetPosition(0);
}

void DemoControl_Abort(void)
{
	g_status.state = DEMO_STATE_IDLE;
	g_status.target_milli_cm = 0;
	g_status.target_normalized =
		APP_VISION_CENTER_POSITION;
	g_status.desired_motor_steps = 0;
	g_status.vision_valid = 0U;
	g_arrival_stable_ms = 0U;
	g_recovery_frames = 0U;
	g_has_sample = 0U;
	BallControl_ResetIntegrators();
	Stepper_SetTargetPosition(0);
}

void DemoControl_OnVisionFrame(
	const MaixcamBallData *ball, uint32_t now_ms)
{
	BallControlOutput output;
	int32_t measured_milli_cm;
	int32_t position_error;
	uint32_t dt_ms;

	if (FrameUsable(ball) == 0U)
	{
		g_status.vision_valid = 0U;
		g_recovery_frames = 0U;
		if ((g_status.state == DEMO_STATE_TO_PLUS5) ||
			(g_status.state == DEMO_STATE_TO_MINUS5) ||
			(g_status.state == DEMO_STATE_HOLD_MINUS5) ||
			(g_status.state == DEMO_STATE_HOLD_CENTER) ||
			(g_status.state == DEMO_STATE_HOLD_CAPTURED))
		{
			/* Do not hold an old tilt while vision is uncertain. */
			g_status.desired_motor_steps = 0;
			Stepper_SetTargetPosition(0);
		}
		return;
	}

	g_status.vision_valid = 1U;
	g_last_valid_local_ms = now_ms;
	g_status.ball_position_normalized = ball->position;
	measured_milli_cm = PositionToMilliCm(ball->position);

	if (g_status.state == DEMO_STATE_WAIT_VISION)
	{
		if (g_recovery_frames < APP_VISION_RECOVERY_FRAMES)
		{
			g_recovery_frames++;
		}
		if (g_recovery_frames < APP_VISION_RECOVERY_FRAMES)
		{
			return;
		}

		BallControl_Reset(measured_milli_cm);
		g_last_camera_timestamp_ms = ball->timestamp_ms;
		g_has_sample = 1U;
		g_status.ball_position_milli_cm =
			measured_milli_cm;
		g_status.ball_velocity_milli_cmps = 0;
		g_status.state = g_state_after_vision;
		g_arrival_stable_ms = 0U;
	}

	if ((g_status.state != DEMO_STATE_TO_PLUS5) &&
		(g_status.state != DEMO_STATE_TO_MINUS5) &&
		(g_status.state != DEMO_STATE_HOLD_MINUS5) &&
		(g_status.state != DEMO_STATE_HOLD_CENTER) &&
		(g_status.state != DEMO_STATE_HOLD_CAPTURED))
	{
		return;
	}

	if (g_has_sample == 0U)
	{
		dt_ms = APP_CONTROL_SAMPLE_FALLBACK_DT_MS;
		BallControl_Reset(measured_milli_cm);
		g_has_sample = 1U;
	}
	else
	{
		dt_ms =
			(uint32_t)(ball->timestamp_ms -
				g_last_camera_timestamp_ms);
		if ((dt_ms == 0U) ||
			(dt_ms > APP_CONTROL_SAMPLE_RESET_MS))
		{
			dt_ms = APP_CONTROL_SAMPLE_FALLBACK_DT_MS;
			BallControl_Reset(measured_milli_cm);
		}
		else if (dt_ms < APP_CONTROL_SAMPLE_DT_MIN_MS)
		{
			dt_ms = APP_CONTROL_SAMPLE_DT_MIN_MS;
		}
	}
	g_last_camera_timestamp_ms = ball->timestamp_ms;

	output = BallControl_Update(
		measured_milli_cm,
		g_status.target_milli_cm,
		(g_status.state == DEMO_STATE_HOLD_CENTER) ?
			APP_OUTER_KP_MILLI_PER_S :
		(g_status.state == DEMO_STATE_HOLD_CAPTURED) ?
			APP_TASK3_OUTER_KP_MILLI_PER_S :
			APP_TASK1_OUTER_KP_MILLI_PER_S,
		(g_status.state == DEMO_STATE_HOLD_CENTER) ?
			APP_INNER_KP_MILLI_STEP_PER_CMPS :
		(g_status.state == DEMO_STATE_HOLD_CAPTURED) ?
			APP_TASK3_INNER_KP_MILLI_STEP_PER_CMPS :
			APP_TASK1_INNER_KP_MILLI_STEP_PER_CMPS,
		dt_ms);
	g_status.ball_position_milli_cm =
		output.filtered_position_milli_cm;
	g_status.ball_velocity_milli_cmps =
		output.filtered_velocity_milli_cmps;
	g_status.desired_motor_steps =
		output.desired_command_position_steps;

	/*
	 * Verified physical sign:
	 * positive STEP raises the motor end and makes Maix position increase.
	 * target-position error can therefore drive STEP with no sign inversion.
	 */
	if (Stepper_SetTargetPosition(
			g_status.desired_motor_steps) == 0U)
	{
		EnterFault();
		return;
	}

	position_error =
		g_status.target_milli_cm -
		g_status.ball_position_milli_cm;
	if (Abs32(position_error) <=
		APP_DEMO_ARRIVAL_TOL_MILLI_CM)
	{
		g_arrival_stable_ms += dt_ms;
	}
	else
	{
		g_arrival_stable_ms = 0U;
	}

	if ((g_status.state == DEMO_STATE_TO_PLUS5) &&
		(g_arrival_stable_ms >= APP_DEMO_PLUS_SETTLE_MS))
	{
		g_status.state = DEMO_STATE_TO_MINUS5;
		g_status.target_milli_cm =
			APP_TARGET_MINUS5_MILLI_CM;
		g_status.target_normalized =
			APP_VISION_MINUS5_POSITION;
		g_status.desired_motor_steps = 0;
		g_arrival_stable_ms = 0U;
		BallControl_ResetIntegrators();
		Stepper_SetTargetPosition(0);
	}
	else if ((g_status.state == DEMO_STATE_TO_MINUS5) &&
		(g_arrival_stable_ms >= APP_DEMO_MINUS_SETTLE_MS))
	{
		g_status.state = DEMO_STATE_HOLD_MINUS5;
		g_status.target_milli_cm =
			APP_TARGET_MINUS5_MILLI_CM;
		g_status.target_normalized =
			APP_VISION_MINUS5_POSITION;
		g_arrival_stable_ms = 0U;
		if (g_status.elapsed_ms <=
			APP_DEMO_PASS_TIME_MS)
		{
			g_status.passed_5_seconds = 1U;
		}
	}
}

void DemoControl_Service(uint32_t now_ms)
{
	if (g_status.state == DEMO_STATE_IDLE)
	{
		return;
	}

	g_status.elapsed_ms =
		(uint32_t)(now_ms - g_start_ms);
	if ((g_status.state == DEMO_STATE_TO_PLUS5) ||
		(g_status.state == DEMO_STATE_TO_MINUS5) ||
		(g_status.state == DEMO_STATE_HOLD_MINUS5) ||
		(g_status.state == DEMO_STATE_HOLD_CENTER) ||
		(g_status.state == DEMO_STATE_HOLD_CAPTURED))
	{
		if ((uint32_t)(now_ms -
			g_last_valid_local_ms) >
			APP_VISION_TIMEOUT_MS)
		{
			EnterFault();
			return;
		}
	}

	if ((g_status.state != DEMO_STATE_HOLD_MINUS5) &&
		(g_status.state != DEMO_STATE_HOLD_CENTER) &&
		(g_status.state != DEMO_STATE_HOLD_CAPTURED) &&
		(g_status.elapsed_ms >
		 APP_DEMO_TOTAL_TIMEOUT_MS))
	{
		EnterFault();
	}
}

DemoStatus DemoControl_GetStatus(void)
{
	return g_status;
}

uint8_t DemoControl_IsActive(void)
{
	return (g_status.state != DEMO_STATE_IDLE) ? 1U : 0U;
}

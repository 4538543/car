#include "AppConfig.h"
#include "BallControl.h"

static int32_t g_filtered_position_milli_cm;
static int32_t g_previous_position_milli_cm;
static int32_t g_filtered_velocity_milli_cmps;
static int32_t g_previous_velocity_milli_cmps;
static int64_t g_position_integral_mcm_ms;
static int64_t g_velocity_integral_mcmps_ms;
static uint8_t g_initialized;

static int32_t Abs32(int32_t value)
{
	return (value < 0) ? -value : value;
}

static int32_t Clamp32(int32_t value, int32_t low, int32_t high)
{
	if (value < low)
	{
		return low;
	}
	if (value > high)
	{
		return high;
	}
	return value;
}

static int64_t Clamp64(int64_t value, int64_t low, int64_t high)
{
	if (value < low)
	{
		return low;
	}
	if (value > high)
	{
		return high;
	}
	return value;
}

static int32_t RoundMilliStep(int64_t value_milli_step)
{
	if (value_milli_step >= 0)
	{
		return (int32_t)((value_milli_step + 500LL) / 1000LL);
	}
	return (int32_t)((value_milli_step - 500LL) / 1000LL);
}

void BallControl_ResetIntegrators(void)
{
	g_position_integral_mcm_ms = 0;
	g_velocity_integral_mcmps_ms = 0;
}

void BallControl_Reset(int32_t measured_position_milli_cm)
{
	g_filtered_position_milli_cm = measured_position_milli_cm;
	g_previous_position_milli_cm = measured_position_milli_cm;
	g_filtered_velocity_milli_cmps = 0;
	g_previous_velocity_milli_cmps = 0;
	BallControl_ResetIntegrators();
	g_initialized = 1U;
}

BallControlOutput BallControl_Update(
	int32_t measured_position_milli_cm,
	int32_t target_position_milli_cm,
	const BallControlGains *gains,
	uint32_t dt_ms)
{
	BallControlOutput output;
	int32_t position_delta;
	int32_t raw_velocity;
	int32_t measured_acceleration;
	int32_t position_error_milli_cm;
	int32_t velocity_error_milli_cmps;
	int64_t outer_p_milli_cmps;
	int64_t outer_i_milli_cmps;
	int64_t outer_d_milli_cmps;
	int64_t outer_unsaturated_milli_cmps;
	int64_t target_velocity_milli_cmps;
	int64_t inner_p_milli_step;
	int64_t inner_i_milli_step;
	int64_t inner_d_milli_step;
	int64_t inner_unsaturated_milli_step;
	int64_t position_integral_candidate;
	int64_t velocity_integral_candidate;
	int64_t beam_limit_milli_step;
	int32_t desired_steps;

	if ((g_initialized == 0U) || (dt_ms == 0U))
	{
		BallControl_Reset(measured_position_milli_cm);
		if (dt_ms == 0U)
		{
			dt_ms = APP_CONTROL_SAMPLE_DT_MIN_MS;
		}
	}

	position_delta =
		measured_position_milli_cm -
		g_filtered_position_milli_cm;
	g_filtered_position_milli_cm +=
		(int32_t)(((int64_t)position_delta *
			APP_POSITION_FILTER_ALPHA_PERMILLE) / 1000LL);

	raw_velocity = (int32_t)(
		((int64_t)(g_filtered_position_milli_cm -
			g_previous_position_milli_cm) * 1000LL) /
		(int64_t)dt_ms);
	g_previous_position_milli_cm =
		g_filtered_position_milli_cm;
	g_filtered_velocity_milli_cmps +=
		(int32_t)(((int64_t)(raw_velocity -
			g_filtered_velocity_milli_cmps) *
			APP_VELOCITY_FILTER_ALPHA_PERMILLE) / 1000LL);

	measured_acceleration = (int32_t)(
		((int64_t)(g_filtered_velocity_milli_cmps -
			g_previous_velocity_milli_cmps) * 1000LL) /
		(int64_t)dt_ms);
	g_previous_velocity_milli_cmps =
		g_filtered_velocity_milli_cmps;
	measured_acceleration = Clamp32(
		measured_acceleration,
		-APP_MEASURED_ACCEL_LIMIT_MCMPS2,
		APP_MEASURED_ACCEL_LIMIT_MCMPS2);

	position_error_milli_cm =
		target_position_milli_cm -
		g_filtered_position_milli_cm;

	if ((Abs32(position_error_milli_cm) <=
		 APP_ZERO_POSITION_DEADBAND_MILLI_CM) &&
		(Abs32(g_filtered_velocity_milli_cmps) <=
		 APP_ZERO_SPEED_DEADBAND_MILLI_CMPS))
	{
		BallControl_ResetIntegrators();
		target_velocity_milli_cmps = 0;
		desired_steps = 0;
	}
	else
	{
		position_integral_candidate = Clamp64(
			g_position_integral_mcm_ms +
				((int64_t)position_error_milli_cm *
				 (int64_t)dt_ms),
			-APP_OUTER_INTEGRAL_LIMIT_MCM_MS,
			APP_OUTER_INTEGRAL_LIMIT_MCM_MS);

		outer_p_milli_cmps =
			((int64_t)gains->outer_kp_milli_per_s *
			 (int64_t)position_error_milli_cm) / 1000LL;
		outer_i_milli_cmps =
			((int64_t)gains->outer_ki_milli_per_s2 *
			 position_integral_candidate) / 1000000LL;
		outer_d_milli_cmps =
			((int64_t)gains->outer_kd_milli *
			 (int64_t)g_filtered_velocity_milli_cmps) / 1000LL;

		outer_unsaturated_milli_cmps =
			outer_p_milli_cmps +
			outer_i_milli_cmps -
			outer_d_milli_cmps;
		if (((outer_unsaturated_milli_cmps >
			  APP_TARGET_VELOCITY_LIMIT_MCMPS) &&
			 (position_error_milli_cm > 0)) ||
			((outer_unsaturated_milli_cmps <
			  -APP_TARGET_VELOCITY_LIMIT_MCMPS) &&
			 (position_error_milli_cm < 0)))
		{
			position_integral_candidate =
				g_position_integral_mcm_ms;
			outer_i_milli_cmps =
				((int64_t)gains->outer_ki_milli_per_s2 *
				 position_integral_candidate) / 1000000LL;
			outer_unsaturated_milli_cmps =
				outer_p_milli_cmps +
				outer_i_milli_cmps -
				outer_d_milli_cmps;
		}
		g_position_integral_mcm_ms =
			position_integral_candidate;
		target_velocity_milli_cmps = Clamp32(
			(int32_t)outer_unsaturated_milli_cmps,
			-APP_TARGET_VELOCITY_LIMIT_MCMPS,
			APP_TARGET_VELOCITY_LIMIT_MCMPS);

		velocity_error_milli_cmps =
			(int32_t)target_velocity_milli_cmps -
			g_filtered_velocity_milli_cmps;
		velocity_integral_candidate = Clamp64(
			g_velocity_integral_mcmps_ms +
				((int64_t)velocity_error_milli_cmps *
				 (int64_t)dt_ms),
			-APP_INNER_INTEGRAL_LIMIT_MCMPS_MS,
			APP_INNER_INTEGRAL_LIMIT_MCMPS_MS);

		inner_p_milli_step =
			((int64_t)gains->inner_kp_milli_step_per_cmps *
			 (int64_t)velocity_error_milli_cmps) / 1000LL;
		inner_i_milli_step =
			((int64_t)gains->inner_ki_milli_step_per_cm *
			 velocity_integral_candidate) / 1000000LL;
		inner_d_milli_step =
			((int64_t)gains->inner_kd_milli_step_per_cmps2 *
			 (int64_t)measured_acceleration) / 1000LL;

		inner_unsaturated_milli_step =
			inner_p_milli_step +
			inner_i_milli_step -
			inner_d_milli_step;
		beam_limit_milli_step =
			(int64_t)APP_BEAM_COMMAND_LIMIT_STEPS * 1000LL;
		if (((inner_unsaturated_milli_step >
			  beam_limit_milli_step) &&
			 (velocity_error_milli_cmps > 0)) ||
			((inner_unsaturated_milli_step <
			  -beam_limit_milli_step) &&
			 (velocity_error_milli_cmps < 0)))
		{
			velocity_integral_candidate =
				g_velocity_integral_mcmps_ms;
			inner_i_milli_step =
				((int64_t)gains->inner_ki_milli_step_per_cm *
				 velocity_integral_candidate) / 1000000LL;
			inner_unsaturated_milli_step =
				inner_p_milli_step +
				inner_i_milli_step -
				inner_d_milli_step;
		}
		g_velocity_integral_mcmps_ms =
			velocity_integral_candidate;
		desired_steps =
			RoundMilliStep(inner_unsaturated_milli_step);
		desired_steps = Clamp32(
			desired_steps,
			-APP_BEAM_COMMAND_LIMIT_STEPS,
			APP_BEAM_COMMAND_LIMIT_STEPS);
	}

	output.filtered_position_milli_cm =
		g_filtered_position_milli_cm;
	output.filtered_velocity_milli_cmps =
		g_filtered_velocity_milli_cmps;
	output.target_velocity_milli_cmps =
		(int32_t)target_velocity_milli_cmps;
	output.desired_command_position_steps = desired_steps;
	return output;
}

#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

/*
 * MaixCAM2 five-point calibration:
 *   300 = -5 cm, 500 = center, 700 = +5 cm
 * Therefore one normalized unit equals 0.025 cm = 25 milli-cm.
 */
#define APP_VISION_CENTER_POSITION             500
#define APP_VISION_MINUS5_POSITION             300
#define APP_VISION_PLUS5_POSITION              700
#define APP_VISION_MILLI_CM_PER_UNIT           25L
#define APP_VISION_TIMEOUT_MS                  500U
#define APP_VISION_RECOVERY_FRAMES             3U

/*
 * PID values copied unchanged from the reference X42S implementation.
 * Gain values use the same fixed-point units as BallControl.c.
 */
#define APP_OUTER_KP_MILLI_PER_S               1500L
#define APP_OUTER_KI_MILLI_PER_S2              150L
#define APP_OUTER_KD_MILLI                     100L
#define APP_OUTER_INTEGRAL_LIMIT_MCM_MS        60000000LL
#define APP_TARGET_VELOCITY_LIMIT_MCMPS        12000L

#define APP_INNER_KP_MILLI_STEP_PER_CMPS       6000L
#define APP_INNER_KI_MILLI_STEP_PER_CM         1200L
#define APP_INNER_KD_MILLI_STEP_PER_CMPS2      20L
#define APP_INNER_INTEGRAL_LIMIT_MCMPS_MS      10000000LL
#define APP_MEASURED_ACCEL_LIMIT_MCMPS2        100000L

#define APP_POSITION_FILTER_ALPHA_PERMILLE     700L
#define APP_VELOCITY_FILTER_ALPHA_PERMILLE     500L
#define APP_ZERO_POSITION_DEADBAND_MILLI_CM    50L
#define APP_ZERO_SPEED_DEADBAND_MILLI_CMPS     200L
#define APP_CONTROL_SAMPLE_DT_MIN_MS           5U
#define APP_CONTROL_SAMPLE_FALLBACK_DT_MS      10U
#define APP_CONTROL_SAMPLE_RESET_MS            250U

/* PID may request only +/-60 STEP although the hardware fence is +/-100. */
#define APP_BEAM_COMMAND_LIMIT_STEPS           60

#define APP_TARGET_PLUS5_MILLI_CM              5000L
#define APP_TARGET_MINUS5_MILLI_CM            (-5000L)
#define APP_DEMO_ARRIVAL_TOL_MILLI_CM          800L
#define APP_DEMO_PLUS_SETTLE_MS                100U
#define APP_DEMO_MINUS_SETTLE_MS               300U
#define APP_DEMO_TOTAL_TIMEOUT_MS              8000U
#define APP_DEMO_PASS_TIME_MS                  5000U

#endif

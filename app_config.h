#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

#define CONTROL_PERIOD_MS            1U
#define HEADING_PERIOD_MS           10U
#define LCD_PERIOD_MS              100U
#define KEY_DEBOUNCE_MS             20U
#define START_GUARD_MS             300U
#define BLACK_CONFIRM_MS            20U
#define BLACK_SENSOR_MIN             2U
#define MISSION_TIMEOUT_MS       20000U
#define Q1_MIN_DISTANCE_MM        1000U
#define Q3_DIAGONAL_DISTANCE_MM   1269U
#define SLOWDOWN_DISTANCE_MM       250U
#define Q3_STAGE2_REMAINING_MM     300U
#define Q3_STAGE3_REMAINING_MM     120U
#define BRAKE_REVERSE_MS           100U
#define BRAKE_REVERSE_SPEED        280U
#define Q3_POINT_HOLD_TIMEOUT_MS  3000U

/* Encoder is counted on each channel-A rising edge. */
#define WHEEL_DIAMETER_MM           65U
#define ENCODER_PULSES_PER_REV    1040U
#define ENCODER_X4_SCALE             4U
#define DRIVE_SPEED                360U
#define APPROACH_SPEED             160U
#define Q3_STAGE2_SPEED            260U
#define Q3_STAGE3_SPEED            160U
#define TURN_SPEED                 260U
#define TURN_MIN_SPEED             115U
#define HEADING_KP_NUM               8
#define HEADING_KP_DEN              10
#define HEADING_KI_DEN             500
#define HEADING_KD_NUM               2
#define HEADING_INTEGRAL_LIMIT    3000
#define HEADING_LIMIT              140
#define TURN_TOLERANCE_DECI_DEG     20
#define TURN_STABLE_MS             180U

#define Q3_AC_TARGET_DECI_DEG       380
#define Q3_B_START_DECI_DEG        1800
#define Q3_BD_TARGET_DECI_DEG      1420

#endif

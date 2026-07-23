#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

#define CONTROL_PERIOD_MS            1U
#define HEADING_PERIOD_MS           10U
#define LCD_PERIOD_MS              100U
#define KEY_DEBOUNCE_MS             20U
#define START_GUARD_MS             300U
#define BLACK_CONFIRM_MS            20U
#define BLACK_SENSOR_MIN             5U
#define MISSION_TIMEOUT_MS       20000U
#define Q1_MIN_DISTANCE_MM        1050U

/* Calibrate on the actual floor: measured encoder pulses per travelled metre. */
#define ENCODER_PULSES_PER_M       800U
#define DRIVE_SPEED                360U
#define TURN_SPEED                 260U
#define HEADING_KP                   9
#define HEADING_LIMIT              110
#define TURN_TOLERANCE_DECI_DEG     20
#define TURN_STABLE_MS             180U

#endif

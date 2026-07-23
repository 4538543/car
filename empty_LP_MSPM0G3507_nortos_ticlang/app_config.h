#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include <stdint.h>

/* ---------- Mechanical calibration (change these first on the real car) ---------- */
#define ENCODER_PULSES_PER_REV             (250.0f)
#define WHEEL_DIAMETER_MM                    (65.0f)
#define STRAIGHT_AB_CD_MM                  (1000.0f)
#define STRAIGHT_AC_BD_MM                  (1280.625f)
#define Q3_EXIT_TURN_DEG                     (38.000f)
/* With the installed gyro convention, clockwise/right rotation is positive:
 * A->C is +38 degrees; after leaving C->B at 180 degrees, a left 38
 * degree turn gives the B->D heading of 142 degrees. */
#define COURSE_AB_HEADING_DEG                 (0.000f)
#define COURSE_CD_HEADING_DEG               (180.000f)
#define DIAGONAL_AC_HEADING_DEG              (38.000f)
#define DIAGONAL_BD_HEADING_DEG             (142.000f)
#define Q4_LAPS                               (4U)

/* Cubic Bezier common-tangent transition between the two semicircles.
 * P0/P3 are 1000 x 800 mm apart; P1/P2 provide horizontal end tangents. */
#define TRANSITION_WIDTH_MM                  (1000.0f)
#define TRANSITION_HEIGHT_MM                  (800.0f)
#define TRANSITION_TANGENT_MM                 (150.0f)
#define TRANSITION_CURVE_LENGTH_MM           (1290.8f)
#define CURVE_BASE_PERMILLE                    (350)
#define CURVE_APPROACH_PERMILLE                (275)
#define CURVE_HEADING_KP                         (9)
#define CURVE_CORRECTION_LIMIT                 (188)
#define CURVE_CORRECTION_SLEW                   (15)
#define CURVE_SIGN_DETECT_DEG                    (1.0f)
#define CURVE_SIGN_DETECT_CORRECTION             (90)

/* If a positive yaw error steers the car farther away, change this to -1. */
#define GYRO_STEER_SIGN                        (1)

/* ---------- Scheduling and safety ---------- */
#define LINE_READ_PERIOD_MS                    (5U)
#define STRAIGHT_CONTROL_PERIOD_MS            (10U)
#define BUTTON_DEBOUNCE_MS                    (20U)
#define BRAKE_TIME_MS                         (80U)
#define STRAIGHT_TIMEOUT_MS                (18000U)
#define TURN_TIMEOUT_MS                     (4500U)
#define GYRO_REQUIRED_AGE_MS                  (500U)

/* ---------- Straight segment ---------- */
#define STRAIGHT_BASE_PERMILLE                (375)
#define STRAIGHT_MAX_PERMILLE                 (750)
#define STRAIGHT_START_PERMILLE               (213)
#define STRAIGHT_RAMP_TIME_MS                 (600U)
#define Q1_STRAIGHT_BASE_PERMILLE             ARC_BASE_PERMILLE
#define Q1_STRAIGHT_APPROACH_PERMILLE         ARC_EXIT_APPROACH_PERMILLE
#define ENCODER_KP                              (8)
#define ENCODER_KI                              (1)
#define ENCODER_I_LIMIT                        (80)
#define GYRO_HEADING_KP                         (5) /* permille / degree */
#define GYRO_RATE_KD                            (0) /* start without rate term */
#define STRAIGHT_CORRECTION_LIMIT             (113)
#define STRAIGHT_CORRECTION_SLEW                 (8)
#define STRAIGHT_APPROACH_PERCENT               (75U)
#define STRAIGHT_APPROACH_PERMILLE              (275)

/* Leave the black start point, confirm white floor, then the next black line
 * immediately starts endpoint alignment. */
#define ENDPOINT_WHITE_MAX                       (0U)
#define ENDPOINT_WHITE_CONFIRM_MS               (35U)
#define ENDPOINT_BLACK_MIN                      (1U)
#define ENDPOINT_CONFIRM_MS                    (10U)
#define STRAIGHT_ENDPOINT_ARM_PERCENT           (50U)
#define CURVE_ENDPOINT_ARM_PERCENT              (65U)

/* ---------- Semicircle gray tracking (teammate controller) ---------- */
#define ARC_CONTROL_PERIOD_MS                    (5U)
#define ARC_BASE_PERMILLE                      (300)
#define ARC_MAX_PERMILLE                       (750)
#define ARC_GRAY_KP                            (105)
#define ARC_GRAY_KI                              (0)
#define ARC_GRAY_KD                             (50)
#define ARC_GAIN_SCALE                        (1000)
#define ARC_I_LIMIT                          (20000)
#define ARC_TURN_LIMIT                         (300)
#define ARC_TRACK_TURN_LIMIT                   (250)
#define ARC_ERROR_DEADBAND                      (350)
#define ARC_ERROR_STEP_LIMIT                    (900)
#define ARC_CORRECTION_SLEW                      (28)
#define ARC_CORRECTION_RELEASE_SLEW              (65)
#define ARC_D_FILTER_OLD_WEIGHT                   (3)
#define ARC_D_FILTER_DIVISOR                      (4)
#define ARC_RADIUS_MM                          (400.0f)
#define CAR_WHEEL_TRACK_MM                     (120.0f)
/* Ideal differential geometry gives about 56 permille. Rear-wheel slip and
 * the forward-mounted sensor need more curvature on the real chassis. */
#define ARC_FEEDFORWARD_GAIN_PERCENT            (125)
#define ARC_FEEDFORWARD ((int32_t)(ARC_BASE_PERMILLE * \
    CAR_WHEEL_TRACK_MM * ARC_FEEDFORWARD_GAIN_PERCENT / \
    (2.0f * ARC_RADIUS_MM * 100.0f)))
#define ARC_ACQUIRE_CONFIRM_MS                  (30U)
#define ARC_ACQUIRE_TIMEOUT_MS                (1200U)
#define ARC_END_CONFIRM_MS                      (60U)
#define ARC_LOST_TIMEOUT_MS                    (500U)
#define ARC_RECOVERY_PERMILLE                  (200)
#define ARC_RECOVERY_TURN                      (240)
#define ARC_RECOVERY_PIVOT_PERMILLE            (180U)
#define ARC_FINISH_DISTANCE_PERCENT             (75U)
#define ARC_FINISH_YAW_DEG                     (150.0f)
#define ARC_TIMEOUT_MS                        (12000U)

/* Bumpless diagonal-to-arc entry and late-arc heading fusion. Gray tracking
 * remains primary; absolute yaw contributes only near the exit. */
#define ARC_ENTRY_START_PERMILLE                (200)
#define ARC_ENTRY_CAPTURE_TURN                  (163)
#define ARC_ENTRY_BLEND_MS                      (220U)
#define ARC_EXIT_SLOW_START_DEG                 (140.0f)
#define ARC_EXIT_SLOW_FULL_DEG                  (170.0f)
#define ARC_EXIT_APPROACH_PERMILLE              (190)
#define ARC_LARGE_ERROR_START                   (1800)
#define ARC_LARGE_ERROR_SLOWDOWN                  (70)
#define ARC_HEADING_BLEND_START_DEG             (145.0f)
#define ARC_HEADING_BLEND_FULL_DEG              (178.0f)
#define ARC_EXIT_YAW_KP                           (2.0f)
#define ARC_EXIT_YAW_KD                           (6.0f)
#define ARC_EXIT_YAW_LIMIT                       (25)
#define ARC_DIRECT_EXIT_TOLERANCE_DEG             (3.0f)

/* Installed gyro yaw decreases during a physical right/clockwise turn.
 * Convert it so the task course convention is right/clockwise positive. */
#define GYRO_ABSOLUTE_YAW_SIGN                  (-1.0f)
#define ARC_EXIT_HEADING_0_DEG                   (0.0f)
#define ARC_EXIT_HEADING_180_DEG               (180.0f)
#define ARC_ALIGN_BASE_PERMILLE                 (200)
#define ARC_ALIGN_MAX_PERMILLE                  (750)
#define ARC_ALIGN_KP                             (6.0f)
#define ARC_ALIGN_KD                            (12.0f)
#define ARC_ALIGN_TURN_LIMIT                    (120)
#define ARC_ALIGN_TOLERANCE_DEG                  (2.0f)
#define ARC_ALIGN_RESTART_ERROR_DEG              (5.0f)
#define ARC_ALIGN_STABLE_MS                      (25U)
#define ARC_ALIGN_TIMEOUT_MS                   (2500U)
#define ARC_PIVOT_MIN_PERMILLE                  (105U)
#define ARC_PIVOT_MAX_PERMILLE                  (170U)
#define ARC_PIVOT_SLOW_ZONE_DEG                  (20.0f)

/* ---------- Forward-only gyro turn ---------- */
#define TURN_OUTER_PERMILLE                    (325U)
#define TURN_INNER_PERMILLE                      (0U)
#define TURN_CORRECT_MIN_PERMILLE               (144U)
#define TURN_SLOW_ZONE_DEG                       (25.0f)
#define TURN_TOLERANCE_DEG                       (4.0f)
#define TURN_RESTART_ERROR_DEG                   (7.0f)
#define TURN_RATE_TOLERANCE_DPS                 (12.0f)
#define TURN_SETTLE_MS                          (35U)
#define TURN_SIGN_DETECT_DEG                    (1.0f)

/* Absolute pivot before diagonal segments. Small-angle turns need early
 * slowdown; otherwise inertia carries the chassis beyond the 38 degree line. */
#define ABS_TURN_MAX_PERMILLE                  (260U)
#define ABS_TURN_MID_PERMILLE                  (190U)
#define ABS_TURN_MIN_PERMILLE                  (115U)
#define ABS_TURN_FAST_ZONE_DEG                  (24.0f)
#define ABS_TURN_FINE_ZONE_DEG                  (12.0f)
#define ABS_TURN_TOLERANCE_DEG                   (3.0f)
#define ABS_TURN_RESTART_ERROR_DEG               (7.0f)
#define ABS_TURN_RATE_TOLERANCE_DPS             (14.0f)
#define ABS_TURN_SETTLE_MS                      (20U)
#define ABS_TURN_PREDICTION_S                    (0.120f)

/* Auxiliary-board serial output: bit0=probe1; 1=white, 0=black. */
#define LINE_SERIAL_PORT                       (GPIOB)
#define LINE_CLK_PIN                           (DL_GPIO_PIN_8)
#define LINE_DAT_PIN                           (DL_GPIO_PIN_9)

/* Software I2C OLED (SSD1306, address 0x3C). */
#define OLED_SCL_PORT                           (GPIOB)
#define OLED_SCL_PIN                            (DL_GPIO_PIN_16)
#define OLED_SDA_PORT                           (GPIOA)
#define OLED_SDA_PIN                            (DL_GPIO_PIN_17)
#define OLED_I2C_ADDRESS                        (0x3CU)
#define OLED_REFRESH_MS                         (100U)


/* Buttons: PA18 closes to 3.3 V; PB22/PB23/PB24 close to GND. */
#define Q1_KEY_PORT                            (GPIOA)
#define Q1_KEY_PIN                             (DL_GPIO_PIN_18)
#define MODE_KEY_PORT                          (GPIOB)
#define Q2_KEY_PIN                             (DL_GPIO_PIN_22)
#define Q3_KEY_PIN                             (DL_GPIO_PIN_23)
#define Q4_KEY_PIN                             (DL_GPIO_PIN_24)

/* On-board blue LED PA7 active-low; buzzer PB26 active-high. */
#define INDICATOR_LED_PORT                     (GPIOA)
#define INDICATOR_LED_PIN                      (DL_GPIO_PIN_7)
#define INDICATOR_BUZZER_PORT                  (GPIOB)
#define INDICATOR_BUZZER_PIN                   (DL_GPIO_PIN_26)

/* TB6612: A=left rear wheel, B=right rear wheel. */
#define MOTOR_DIR_PORT                         (GPIOB)
#define MOTOR_A_IN1_PIN                        (DL_GPIO_PIN_4)
#define MOTOR_A_IN2_PIN                        (DL_GPIO_PIN_5)
#define MOTOR_B_IN1_PIN                        (DL_GPIO_PIN_6)
#define MOTOR_B_IN2_PIN                        (DL_GPIO_PIN_7)
#define MOTOR_STBY_PIN                         (DL_GPIO_PIN_12)
#define MOTOR_A_PWM_TIMER                      (TIMG0)
#define MOTOR_A_PWM_CC                         (DL_TIMER_CC_1_INDEX)
#define MOTOR_B_PWM_TIMER                      (TIMA1)
#define MOTOR_B_PWM_CC                         (DL_TIMER_CC_0_INDEX)

#endif

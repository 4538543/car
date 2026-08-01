#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * Mechanical parameters.
 *
 * The wheel circumference still uses the nominal 65 mm diameter. After the
 * first floor test, only the count thresholds below need to be calibrated.
 */
#define WHEEL_DIAMETER_MM                  (65U)
#define ENCODER_LINES_PER_MOTOR_REV        (13U)
#define ENCODER_QUADRATURE_MULTIPLIER       (4U)
#define MOTOR_GEAR_RATIO                   (28U)
#define ENCODER_COUNTS_PER_WHEEL_REV     (1456U)

#define WHEEL_TRACK_MM                    (210U)
#define VEHICLE_LENGTH_MM                 (350U)
#define GRAY_TO_AXLE_DISTANCE_MM          (130U)
#define GRAY_ARRAY_WIDTH_MM                (84U)
#define GRAY_SENSOR_PITCH_MM               (12U)
#define TRACK_LINE_WIDTH_MM                (18U)
#define GRAY_INSTALL_HEIGHT_MM             (30U)

/*
 * Ganwei MSPM0L1306 auxiliary-board serial interface.
 *
 * The official example shifts the first sample into bit 0 and uses a 5 us
 * CLK-high time. Its digital convention is white=1 and black=0.
 * If the sensor board is installed in the opposite left/right orientation,
 * change only GRAY_BIT0_IS_LEFT.
 */
#define GRAY_SERIAL_HIGH_US                   (5U)
#define GRAY_SERIAL_LOW_US                    (1U)
#define GRAY_BLACK_LEVEL                      (0U)
#define GRAY_BIT0_IS_LEFT                     (1)
#define GRAY_MAX_ACTIVE_SENSORS               (4U)
#define GRAY_CENTER_ACCEPT_SENSOR_MASK        (0x18U)
#define GRAY_CENTER_ACCEPT_MAX_ACTIVE_SENSORS  (2U)
#define GRAY_FINISH_MIN_ACTIVE_SENSORS         (3U)
#define GRAY_FINISH_MAX_ACTIVE_SENSORS         (8U)
#define GRAY_FINISH_MIN_CONSECUTIVE_SENSORS    (1U)

/*
 * A -> B is 1500 mm. Both A and B use the grayscale board as the alignment
 * reference, so the 130 mm sensor-to-axle offset must NOT be subtracted.
 */
#define AB_SLOW_DOWN_COUNT               (8556U)  /* about 1200 mm */
#define AB_CURVE_ARM_COUNT               (9626U)  /* about 1350 mm */
#define AB_BRAKE_START_COUNT            (10338U)  /* about 1450 mm */
#define AB_TARGET_COUNT                 (10695U)  /* theoretical 1500 mm */
#define AB_TIMEOUT_MS                   (20000U)

/*
 * One measured complete lap is 3267 mm.
 * Encoder distance now only arms finish-marker recognition and provides a
 * missed-marker safety cutoff. The first transverse marker after launch is
 * recorded only as the lap reference. The next qualified marker completes
 * the lap; PA18 brakes immediately, while PB23 continues for body clearance.
 */
#define LAP_DISTANCE_STOP_COUNT         (25650U)  /* about 3600 mm */
#define LAP_DISTANCE_STOP_MIN_ANGLE_CENTIDEG (33000)
#define LAP_SLOW_DOWN_COUNT             (21387U)  /* about 3000 mm */
#define LAP_START_MARKER_SEARCH_COUNT     (3565U)  /* about 500 mm */
#define LAP_FINISH_ARM_RELATIVE_COUNT    (21031U)  /* about 2950 mm after start line */
#define LAP_EDGE_FINISH_ARM_RELATIVE_COUNT (22439U) /* about 3150 mm */
#define LAP_MISSED_MARKER_RELATIVE_COUNT (24937U)  /* about 3500 mm after start line */
#define LAP_FINISH_CONFIRM_10MS_TICKS       (1U)
#define LAP_EDGE_FINISH_CONFIRM_10MS_TICKS  (2U)
#define LAP_OUTERMOST_SENSOR_MASK          (0x81U)
#define MARKER_NORMAL_LINE_RECENT_10MS_TICKS (100U)
#define LAP_WIDE_FINISH_ARM_TURN_CENTIDEG (24000U) /* 240 degrees */
#define LAP_EDGE_FINISH_ARM_TURN_CENTIDEG (33000U) /* 330 degrees */
#define LAP_HARD_STOP_TURN_CENTIDEG       (43000U) /* 430 degrees */
#define LAP_TIMEOUT_MS                  (50000U)

/*
 * MSPM0 -> STM32 telemetry UART.
 *
 * A readable ASCII telemetry line is sent every 20 ms:
 *   $T,WZ=<0.01dps>,ALPHA=<0.01dps2>,ACC=<mm/s2>,SPD=<mm/s>\r\n
 * Event lines use:
 *   $E,ACCEL / DECEL / CURVE / STRAIGHT
 */
#define STM32_TELEMETRY_PERIOD_MS          (20U)
#define STM32_TX_QUEUE_SIZE               (256U)
#define STM32_CURVE_RATE_ENTER_CENTIDPS    (800)
#define STM32_CURVE_RATE_EXIT_CENTIDPS     (400)
#define STM32_CURVE_CONFIRM_MS              (80U)
#define STM32_STRAIGHT_CONFIRM_MS          (150U)
#define STM32_SEGMENT_MIN_SPEED_MMPS        (50U)

/*
 * The sensor bar is treated as the front reference of the 350 mm vehicle.
 * After B/A is detected, travel one full vehicle length before freezing the
 * timer, then continue another 150 mm before deceleration.
 */
#define PASS_BODY_CLEAR_COUNT             (2496U)  /* about 350 mm */
#define PASS_EXTRA_RUN_COUNT              (1069U)  /* about 150 mm */
#define PASS_BRAKE_AFTER_COUNT \
    (PASS_BODY_CLEAR_COUNT + PASS_EXTRA_RUN_COUNT)
#define PASS_HARD_STOP_COUNT              (4278U)  /* about 600 mm total */
#define PASS_BODY_TIMEOUT_MIN_MS          (2000U)
#define PASS_BODY_TIMEOUT_MAX_MS          (6000U)
#define PASS_BODY_TIMEOUT_MARGIN_MS        (500U)

/*
 * Conservative first-test outputs. Units are per mille (0...1000).
 * 300 per mille is intentionally well below the motor's rated speed.
 */
#define AB_CRUISE_PWM_PERMILLE            (420)
#define AB_APPROACH_PWM_PERMILLE           (210)
#define LAP_CRUISE_PWM_PERMILLE            (380)
#define LAP_APPROACH_PWM_PERMILLE          (210)
#define PASS_AFTER_POINT_PWM_PERMILLE      (190)
#define STRAIGHT_RAMP_UP_STEP_PER_10MS       (2)
#define STRAIGHT_RAMP_DOWN_STEP_PER_10MS     (6)

/*
 * After the forward PWM has ramped down to zero, wait briefly before
 * reversing direction. Apply only a small reverse torque for a short time;
 * this is braking, not a reverse-driving phase.
 */
#define AB_BRAKE_ZERO_WAIT_MS              (20U)
#define AB_REVERSE_BRAKE_PWM_PERMILLE       (80)
#define AB_REVERSE_BRAKE_TIME_MS           (40U)

/*
 * Encoder synchronization controller and grayscale line PID.
 *
 * Gray line error uses 100 units per sensor pitch. Positive error means the
 * line is on the left, so positive correction slows the left wheel and speeds
 * the right wheel.
 */
#define STRAIGHT_KP_POSITION                 (1)
#define STRAIGHT_KP_SPEED                    (2)
#define STRAIGHT_POSITION_ERROR_DEADBAND      (8)
#define STRAIGHT_SPEED_ERROR_DEADBAND         (2)
#define STRAIGHT_MAX_CORRECTION_PERMILLE     (50)
#define STRAIGHT_SYNC_DISABLE_LINE_ERROR     (150U)

#define LINE_PID_KP_NUM                     (26)
#define LINE_PID_KI_NUM                      (1)
#define LINE_PID_KD_NUM                     (18)
#define LINE_PID_NEAR_KP_NUM                (16)
#define LINE_PID_NEAR_KD_NUM                (12)
#define LINE_PID_NEAR_ERROR_LIMIT           (150U)
#define LINE_PID_SCALE                     (100)
#define LINE_PID_INTEGRAL_LIMIT           (1200)
#define LINE_PID_MAX_CORRECTION_PERMILLE   (150)
#define DRIVE_MAX_CORRECTION_PERMILLE      (170)
#define LINE_CURVE_SPEED_ERROR_THRESHOLD   (200U)
#define LINE_CURVE_BASE_LIMIT_PERMILLE      (240)
#define LINE_STEERING_FULL_RAMP_PERMILLE    (100)
#define LINE_START_CENTER_ACCEPT_END_PERMILLE (100)
#define LINE_LOST_MIN_ERROR                 (180)
#define LINE_LOST_HOLD_10MS_TICKS            (8U)
#define LINE_LOST_RECOVERY_CORRECTION_PERMILLE (140)

/*
 * Digital probes otherwise make the vehicle alternate between tangent travel
 * and a large correction. Once a persistent outer error identifies a curve,
 * retain a small differential feed-forward until an opposite error confirms
 * that the straight has been reached.
 */
#define CURVE_ASSIST_ENTER_ERROR             (180)
#define CURVE_ASSIST_ENTER_10MS_TICKS          (3U)
#define CURVE_ASSIST_FEEDFORWARD_PERMILLE     (36)
#define CURVE_ASSIST_RELEASE_ERROR            (100)
#define CURVE_ASSIST_RELEASE_10MS_TICKS         (5U)

/*
 * Only after the encoder distance reaches AB_CURVE_ARM_COUNT may a persistent
 * grayscale displacement be interpreted as entry into the B-end arc.
 * Three 10 ms samples reject a single noisy reading.
 */
#define GRAY_CURVE_ERROR_THRESHOLD          (140)
#define GRAY_CURVE_CONFIRM_10MS_TICKS         (3U)

/*
 * Stop if either encoder produces no edges for 300 ms while PWM is high
 * enough that the vehicle should already be moving.
 */
#define ENCODER_FAULT_CHECK_PWM_PERMILLE    (180)
#define ENCODER_FAULT_CONFIRM_10MS_TICKS     (30U)

/*
 * Wiring-dependent options.
 *
 * TB6612 direction truth table used here:
 *   IN1=1, IN2=0 for the side whose option is 1;
 *   IN1=0, IN2=1 for the side whose option is 0.
 *
 * Preserve the latest real-vehicle direction choices. If motor wiring changes
 * again, change only the corresponding option; never swap encoder wires.
 */
#define MOTOR_LEFT_FORWARD_IN1_HIGH           (1)
#define MOTOR_RIGHT_FORWARD_IN1_HIGH          (1)

/*
 * Quadrature direction signs. Distance control uses magnitudes, but keeping
 * signed counts correct will be useful for later reversing and turning.
 */
#define ENCODER_LEFT_DIRECTION_SIGN            (1)
#define ENCODER_RIGHT_DIRECTION_SIGN           (1)

#endif

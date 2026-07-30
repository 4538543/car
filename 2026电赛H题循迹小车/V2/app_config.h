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
#define GRAY_FINISH_MIN_ACTIVE_SENSORS         (4U)
#define GRAY_FINISH_MAX_ACTIVE_SENSORS         (6U)
#define GRAY_FINISH_MIN_CONSECUTIVE_SENSORS    (4U)
#define GRAY_FINISH_CENTER_SENSOR_MASK        (0x18U)

/*
 * A -> B is 1500 mm. Both A and B use the grayscale board as the alignment
 * reference, so the 130 mm sensor-to-axle offset must NOT be subtracted.
 */
#define AB_SLOW_DOWN_COUNT               (8556U)  /* about 1200 mm */
#define AB_CURVE_ARM_COUNT               (9626U)  /* about 1350 mm */
#define AB_BRAKE_START_COUNT            (10338U)  /* about 1450 mm */
#define AB_TARGET_COUNT                 (10695U)  /* theoretical 1500 mm */
#define AB_TIMEOUT_MS                   (12000U)

/*
 * One complete stadium-shaped lap:
 *   2 * 1500 mm + 2 * pi * 500 mm = about 6142 mm.
 * Encoder distance now only arms finish-marker recognition and provides a
 * missed-marker safety cutoff. Normal completion is triggered by the 50 mm
 * transverse marker, followed by forward overtravel that moves the marker
 * underneath the chassis instead of stopping it at the sensor bar.
 */
#define LAP_SLOW_DOWN_COUNT             (41347U)  /* about 5800 mm */
#define LAP_FINISH_ARM_COUNT            (38491U)  /* about 5400 mm */
#define LAP_MISSED_MARKER_STOP_COUNT    (47756U)  /* about 6700 mm */
#define LAP_FINISH_CONFIRM_10MS_TICKS       (2U)
#define LAP_FINISH_BRAKE_AFTER_COUNT       (927U)  /* about 130 mm */
#define LAP_FINISH_STOP_AFTER_COUNT       (1283U)  /* about 180 mm */
#define LAP_TIMEOUT_MS                  (45000U)

/*
 * Conservative first-test outputs. Units are per mille (0...1000).
 * 300 per mille is intentionally well below the motor's rated speed.
 */
#define AB_CRUISE_PWM_PERMILLE            (300)
#define AB_APPROACH_PWM_PERMILLE           (180)
#define LAP_CRUISE_PWM_PERMILLE            (220)
#define LAP_APPROACH_PWM_PERMILLE          (170)
#define LAP_FINISH_COVER_PWM_PERMILLE      (150)
#define STRAIGHT_RAMP_UP_STEP_PER_10MS       (5)
#define STRAIGHT_RAMP_DOWN_STEP_PER_10MS     (8)

/*
 * After the forward PWM has ramped down to zero, wait briefly before
 * reversing direction. Apply only a small reverse torque for a short time;
 * this is braking, not a reverse-driving phase.
 */
#define AB_BRAKE_ZERO_WAIT_MS              (20U)
#define AB_REVERSE_BRAKE_PWM_PERMILLE      (120)
#define AB_REVERSE_BRAKE_TIME_MS           (70U)

/*
 * Encoder synchronization controller and grayscale line PID.
 *
 * Gray line error uses 100 units per sensor pitch. Positive error means the
 * line is on the left, so positive correction slows the left wheel and speeds
 * the right wheel.
 */
#define STRAIGHT_KP_POSITION                 (1)
#define STRAIGHT_KP_SPEED                    (3)
#define STRAIGHT_MAX_CORRECTION_PERMILLE    (70)

#define LINE_PID_KP_NUM                     (26)
#define LINE_PID_KI_NUM                      (1)
#define LINE_PID_KD_NUM                     (30)
#define LINE_PID_SCALE                     (100)
#define LINE_PID_INTEGRAL_LIMIT           (1200)
#define LINE_PID_MAX_CORRECTION_PERMILLE   (150)
#define DRIVE_MAX_CORRECTION_PERMILLE      (170)
#define LINE_CURVE_SPEED_ERROR_THRESHOLD   (120U)
#define LINE_CURVE_BASE_LIMIT_PERMILLE      (180)
#define LINE_LOST_MIN_ERROR                 (180)
#define LINE_LOST_HOLD_10MS_TICKS            (8U)
#define LINE_LOST_RECOVERY_CORRECTION_PERMILLE (140)

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

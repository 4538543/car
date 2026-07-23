#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include <stdint.h>

/*
 * Vehicle orientation:
 *   Motor A = left rear wheel
 *   Motor B = right rear wheel
 * Sensor orientation when viewed from the rear of the car:
 *   CH1 = far left, CH8 = far right.
 */

#define APP_CONTROL_PERIOD_MS             (5U)
#define APP_SPEED_PERIOD_MS              (10U)
#define APP_START_GUARD_MS               (300U)
#define APP_MAX_RUN_MS                 (15000U)
#define APP_BRAKE_TIME_MS                 (80U)

/* Safe first-run speed. Increase only after the car tracks reliably. */
#define APP_BASE_SPEED_PERMILLE           (350U)
#define APP_MAX_SPEED_PERMILLE            (600U)

/* Encoder balance controller, executed every 10 ms. */
#define SPEED_BALANCE_KP                  (12)
#define SPEED_BALANCE_KI                   (1)
#define SPEED_BALANCE_I_LIMIT             (80)
#define SPEED_BALANCE_OUT_LIMIT          (160)

/* Reserved for later line-following questions; Q1 does not use these. */
#define APP_STEERING_SIGN                  (1)
#define LINE_KP                           (105)
#define LINE_KD                           (180)
#define LINE_TURN_LIMIT                   (260)

/* Stop after at least this many probes see black for the confirm time. */
#define Q1_BLACK_SENSOR_MIN                (5U)
#define Q1_BLACK_CONFIRM_MS               (15U)

/* Auxiliary-board serial output: probe bits are 0 on black, 1 on white. */
#define LINE_SERIAL_PORT                  (GPIOB)
#define LINE_CLK_PIN                      (DL_GPIO_PIN_8)
#define LINE_DAT_PIN                      (DL_GPIO_PIN_9)

/* On-board KEY1 is PA18, active low. */
#define START_KEY_PORT                    (GPIOA)
#define START_KEY_PIN                     (DL_GPIO_PIN_18)

/* On-board blue LED PA7 is active low; buzzer PB26 is active high. */
#define INDICATOR_LED_PORT                (GPIOA)
#define INDICATOR_LED_PIN                 (DL_GPIO_PIN_7)
#define INDICATOR_BUZZER_PORT             (GPIOB)
#define INDICATOR_BUZZER_PIN              (DL_GPIO_PIN_26)

/* TB6612 control pins. */
#define MOTOR_DIR_PORT                    (GPIOB)
#define MOTOR_A_IN1_PIN                   (DL_GPIO_PIN_4)
#define MOTOR_A_IN2_PIN                   (DL_GPIO_PIN_5)
#define MOTOR_B_IN1_PIN                   (DL_GPIO_PIN_6)
#define MOTOR_B_IN2_PIN                   (DL_GPIO_PIN_7)
#define MOTOR_STBY_PIN                    (DL_GPIO_PIN_12)

#define MOTOR_A_PWM_TIMER                 (TIMG0)
#define MOTOR_A_PWM_CC                    (DL_TIMER_CC_1_INDEX)
#define MOTOR_B_PWM_TIMER                 (TIMA1)
#define MOTOR_B_PWM_CC                    (DL_TIMER_CC_0_INDEX)

#endif

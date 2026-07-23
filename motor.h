#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>

typedef enum {
    MOTOR_A = 0,
    MOTOR_B
} Motor_Id;

typedef enum {
    MOTOR_COAST = 0,
    MOTOR_FORWARD,
    MOTOR_REVERSE,
    MOTOR_BRAKE
} Motor_Direction;

/* speedPermille: 0...1000, 800 means 80.0% PWM duty cycle. */
void Motor_init(void);
void Motor_setDirection(Motor_Id motor, Motor_Direction direction);
void Motor_setSpeed(Motor_Id motor, uint16_t speedPermille);
void Motor_run(Motor_Id motor, Motor_Direction direction,
               uint16_t speedPermille);
void Motor_stop(Motor_Id motor);
void Motor_stopAll(void);

#endif

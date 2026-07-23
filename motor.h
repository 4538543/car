#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>
void Motor_init(void);
void Motor_drive(int16_t left, int16_t right);
void Motor_stop(void);
#endif

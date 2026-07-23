#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>

void Motor_init(void);
void Motor_enable(void);
void Motor_disable(void);
void Motor_setForward(uint16_t leftPermille, uint16_t rightPermille);
void Motor_brakeAll(void);
void Motor_coastAll(void);

#endif

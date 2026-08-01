#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

void Motor_init(void);
void Motor_driveForwardPermille(int32_t leftPermille, int32_t rightPermille);
void Motor_driveReversePermille(int32_t leftPermille, int32_t rightPermille);
void Motor_stop(void);
void Motor_emergencyStop(void);

#endif

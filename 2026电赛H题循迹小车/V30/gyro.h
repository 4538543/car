#ifndef GYRO_H
#define GYRO_H

#include <stdbool.h>
#include <stdint.h>

void Gyro_init(void);
void Gyro_task1ms(void);
void Gyro_uartIrqHandler(void);

bool Gyro_hasFreshAngularRate(void);
int32_t Gyro_getAngularRateCentiDps(void);
int32_t Gyro_getYawCentiDeg(void);
uint32_t Gyro_getValidFrameCount(void);

#endif

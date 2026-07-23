#ifndef GYRO_H
#define GYRO_H
#include <stdbool.h>
#include <stdint.h>
void Gyro_init(void); int16_t Gyro_yawDeciDeg(void); bool Gyro_valid(void); void GYRO_UART_INST_IRQHandler(void);
#endif

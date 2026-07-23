#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

void Encoder_init(void);
void Encoder_reset(void);
uint32_t Encoder_averagePulses(void);
uint32_t Encoder_averageDistanceMm(void);
void GROUP1_IRQHandler(void);

#endif

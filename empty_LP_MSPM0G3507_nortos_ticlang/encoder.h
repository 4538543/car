#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdint.h>

void Encoder_init(void);
void Encoder_reset(void);
void Encoder_sample(uint32_t *leftPulses, uint32_t *rightPulses);
void Encoder_getTotal(uint32_t *leftPulses, uint32_t *rightPulses);

#endif

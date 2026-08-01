#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

typedef struct {
    int32_t left;
    int32_t right;
} EncoderSnapshot;

void Encoder_init(void);
void Encoder_reset(void);
void Encoder_onGpioInterrupt(uint32_t gpioAStatus);
void Encoder_getSnapshot(EncoderSnapshot *snapshot);
uint32_t Encoder_getAverageMagnitude(void);
uint32_t Encoder_countsToMillimeters(uint32_t counts);
uint32_t Encoder_countsToMillimetersPerSecond(
    uint32_t counts, uint32_t intervalMs);

#endif

#ifndef STRAIGHT_CONTROL_H
#define STRAIGHT_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

void StraightControl_init(void);
void StraightControl_start(int32_t targetBasePermille,
    uint16_t lineGainPercent, int32_t rampUpStep,
    int32_t rampDownStep);
void StraightControl_setTargetBase(int32_t targetBasePermille);
void StraightControl_setEncoderSynchronizationEnabled(bool enabled);
void StraightControl_task10ms(void);
void StraightControl_stop(void);
bool StraightControl_hasEncoderFault(void);
int32_t StraightControl_getRampedBase(void);
uint32_t StraightControl_getSpeedMmPerSecond(void);
bool StraightControl_hasValidLine(void);
int16_t StraightControl_getLineError(void);

#endif

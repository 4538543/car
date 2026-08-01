#ifndef GRAY_SENSOR_H
#define GRAY_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t raw;
    uint8_t blackMask;
    uint8_t activeCount;
    int16_t lineError;
    bool lineValid;
    bool finishMarkerCandidate;
} GraySensorSample;

void GraySensor_init(void);
void GraySensor_sample(void);
void GraySensor_getSample(GraySensorSample *sample);
bool GraySensor_hasLine(void);
int16_t GraySensor_getLineError(void);
uint8_t GraySensor_getRaw(void);
uint8_t GraySensor_getBlackMask(void);
uint8_t GraySensor_getActiveCount(void);
bool GraySensor_isFinishMarkerCandidate(void);

#endif

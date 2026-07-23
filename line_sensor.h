#ifndef LINE_SENSOR_H_
#define LINE_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t rawBits;
    uint8_t blackMask;
    uint8_t blackCount;
    int16_t error;
    bool lineFound;
    bool valid;
} LineSensor_Frame;

void LineSensor_init(void);
bool LineSensor_read(LineSensor_Frame *frame);

#endif

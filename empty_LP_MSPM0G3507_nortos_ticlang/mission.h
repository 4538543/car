#ifndef MISSION_H_
#define MISSION_H_

#include <stdint.h>

void Mission_init(void);
void Mission_task1ms(void);

extern volatile uint8_t gMissionMode;
extern volatile uint8_t gMissionState;
extern volatile uint8_t gMissionFault;
extern volatile uint8_t gMissionLap;
extern volatile uint8_t gMissionPhase;

#endif

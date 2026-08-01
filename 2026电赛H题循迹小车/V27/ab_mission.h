#ifndef AB_MISSION_H
#define AB_MISSION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    AB_MISSION_IDLE = 0,
    AB_MISSION_CRUISE,
    AB_MISSION_APPROACH,
    AB_MISSION_PASS_BODY,
    AB_MISSION_POST_RUN,
    AB_MISSION_DECEL,
    AB_MISSION_BRAKE_WAIT,
    AB_MISSION_REVERSE_BRAKE,
    AB_MISSION_DONE,
    AB_MISSION_ABORTED,
    AB_MISSION_FAULT
} ABMissionState;

typedef enum {
    AB_MISSION_MODE_NONE = 0,
    AB_MISSION_MODE_LAP_IMMEDIATE,
    AB_MISSION_MODE_AB_PASS,
    AB_MISSION_MODE_LAP_PASS
} ABMissionMode;

void ABMission_init(void);
void ABMission_startLapImmediate(void);
void ABMission_startAbPass(void);
void ABMission_startLapPass(void);
void ABMission_task1ms(void);
void ABMission_requestEmergencyStop(void);
void ABMission_requestEmergencyStopFromISR(void);
void ABMission_notifyRightCurve(void);
ABMissionState ABMission_getState(void);
ABMissionMode ABMission_getMode(void);
uint32_t ABMission_getAverageCount(void);
uint32_t ABMission_getDistanceMm(void);
uint32_t ABMission_getElapsedMs(void);

#endif

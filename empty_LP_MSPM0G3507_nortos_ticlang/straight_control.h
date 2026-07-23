#ifndef STRAIGHT_CONTROL_H_
#define STRAIGHT_CONTROL_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STRAIGHT_RUNNING = 0,
    STRAIGHT_REACHED,
    STRAIGHT_TIMEOUT
} Straight_Result;

typedef enum {
    TRANSITION_CURVE_AC = 1,
    TRANSITION_CURVE_BD = -1
} TransitionCurve_Direction;

void StraightControl_start(float distanceMm);
void StraightControl_startQ1Test(float distanceMm);
/* Locks to a course-absolute yaw (0...360 degrees) instead of start yaw. */
void StraightControl_startAbsolute(float distanceMm, float targetYawDeg);
void StraightControl_startTransition(TransitionCurve_Direction direction);
Straight_Result StraightControl_task1ms(void);
void StraightControl_stop(void);

extern volatile uint32_t gStraightLeftPulses;
extern volatile uint32_t gStraightRightPulses;
extern volatile uint32_t gStraightTargetPulses;
extern volatile int16_t gStraightCorrection;
extern volatile uint8_t gStraightRawGray;
extern volatile float gStraightTargetYawDebug;
extern volatile uint8_t gStraightEndpointEligibleDebug;
extern volatile int8_t gCurveYawSignDebug;
extern volatile float gCurveYawErrorDebug;

#endif

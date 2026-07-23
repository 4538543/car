#ifndef TURN_CONTROL_H_
#define TURN_CONTROL_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TURN_RUNNING = 0,
    TURN_REACHED,
    TURN_FAULT
} Turn_Result;

/* direction: +1 for the B->D turn, -1 for the A->C turn. */
bool TurnControl_start(int8_t direction, float angleDeg);
/* Fast zero-translation pivot to a course-absolute yaw (0...360 degrees). */
bool TurnControl_startAbsolute(float targetYawDeg);
Turn_Result TurnControl_task1ms(void);

extern volatile float gTurnYawDelta;
extern volatile float gTurnTargetDeg;
extern volatile float gTurnAbsoluteError;

#endif

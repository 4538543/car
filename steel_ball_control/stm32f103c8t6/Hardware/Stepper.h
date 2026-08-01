#ifndef __STEPPER_H
#define __STEPPER_H

#include "stm32f10x.h"

/*
 * Conservative open-loop limits for the first installed-mechanism test.
 * These values are command-pulse limits, not encoder feedback.
 */
#define STEPPER_TEST_STEPS                 20
#define STEPPER_SOFT_LIMIT_STEPS           100
#define STEPPER_MAX_SPEED_SPS              200
#define STEPPER_MIN_SPEED_SPS              20
#define STEPPER_ACCEL_SPS2                 500
#define STEPPER_FOLLOW_GAIN_SPS_PER_STEP   10

void Stepper_Init(void);
void Stepper_SetEnabled(uint8_t enabled);
uint8_t Stepper_IsEnabled(void);

/* Non-blocking: publishes a new target and returns immediately. */
uint8_t Stepper_MoveRelative(int32_t steps);
uint8_t Stepper_SetTargetPosition(int32_t target_steps);
void Stepper_Stop(void);

int32_t Stepper_GetCommandPosition(void);
int32_t Stepper_GetTargetPosition(void);
int32_t Stepper_GetCommandSpeedSps(void);
uint8_t Stepper_IsRunning(void);
uint32_t Stepper_GetTickMs(void);

/* Called only by the interrupt vector file. */
void Stepper_OnTim2TickIrq(void);
void Stepper_OnTim3PulseIrq(void);

#endif

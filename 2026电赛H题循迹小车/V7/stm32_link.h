#ifndef STM32_LINK_H
#define STM32_LINK_H

#include <stdint.h>

typedef enum {
    STM32_EVENT_ACCELERATING = 0,
    STM32_EVENT_DECELERATING,
    STM32_EVENT_ENTER_CURVE,
    STM32_EVENT_ENTER_STRAIGHT
} Stm32LinkEvent;

void Stm32Link_init(void);
void Stm32Link_task1ms(void);
void Stm32Link_notifyEvent(Stm32LinkEvent event);

int32_t Stm32Link_getAngularAccelerationCentiDps2(void);
int32_t Stm32Link_getLongitudinalAccelerationMmps2(void);
uint32_t Stm32Link_getDroppedFrameCount(void);

#endif

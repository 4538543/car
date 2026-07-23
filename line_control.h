#ifndef LINE_CONTROL_H_
#define LINE_CONTROL_H_

#include <stdint.h>

void LineControl_reset(void);
int16_t LineControl_step(int16_t lineError);
void LineControl_drive(int16_t turn);

#endif

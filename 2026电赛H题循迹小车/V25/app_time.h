#ifndef APP_TIME_H
#define APP_TIME_H

#include <stdbool.h>
#include <stdint.h>

void AppTime_init(void);
void AppTime_start(void);
void AppTime_stop(void);
void AppTime_task1ms(void);
bool AppTime_isRunning(void);
uint32_t AppTime_getElapsedMs(void);

#endif

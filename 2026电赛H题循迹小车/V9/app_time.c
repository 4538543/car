#include "app_time.h"

static bool gRunning;
static uint32_t gElapsedMs;

void AppTime_init(void)
{
    gRunning = false;
    gElapsedMs = 0U;
}

void AppTime_start(void)
{
    gElapsedMs = 0U;
    gRunning = true;
}

void AppTime_stop(void)
{
    gRunning = false;
}

void AppTime_task1ms(void)
{
    if (gRunning && (gElapsedMs < UINT32_MAX)) {
        gElapsedMs++;
    }
}

bool AppTime_isRunning(void)
{
    return gRunning;
}

uint32_t AppTime_getElapsedMs(void)
{
    return gElapsedMs;
}

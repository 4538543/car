#ifndef Q3_MISSION_H
#define Q3_MISSION_H

typedef enum {
    Q3_MISSION_RUNNING,
    Q3_MISSION_DONE,
    Q3_MISSION_FAULT
} Q3Mission_Result;

void Q3Mission_init(void);
void Q3Mission_start(void);
/* Reserved for the future CB line-tracing module; no second key is needed. */
void Q3Mission_continueFromB(void);
Q3Mission_Result Q3Mission_task1ms(void);

#endif

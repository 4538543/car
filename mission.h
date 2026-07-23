#ifndef MISSION_H
#define MISSION_H
void Mission_init(void); void Mission_task1ms(void);
/* Called by the future B->C line-following module after it confirms point C. */
void Mission_beginQ2_CD(void);
#endif

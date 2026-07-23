#ifndef TASK_BUTTONS_H_
#define TASK_BUTTONS_H_

typedef enum {
    TASK_BUTTON_NONE = 0,
    TASK_BUTTON_Q1,
    TASK_BUTTON_Q2,
    TASK_BUTTON_Q3,
    TASK_BUTTON_Q4
} TaskButton_Event;

void TaskButtons_init(void);
TaskButton_Event TaskButtons_poll1ms(void);

#endif

#ifndef KEYS_H
#define KEYS_H
#include <stdint.h>
typedef enum { KEY_NONE, KEY_Q1, KEY_Q2, KEY_Q3, KEY_Q4 } Key_Event;
void Keys_init(void); Key_Event Keys_task1ms(void);
#endif

#ifndef LED_H_
#define LED_H_

#include <stdint.h>

/**
 * @brief 初始化LED状态
 *
 * GPIO本身已经由SYSCFG_DL_init()初始化，
 * 这里主要用于设置LED的初始状态。
 */
void LED_Init(void);

/**
 * @brief 点亮LED
 */
void LED_On(void);

/**
 * @brief 熄灭LED
 */
void LED_Off(void);

/**
 * @brief 翻转LED状态
 */
void LED_Toggle(void);

#endif /* LED_H_ */
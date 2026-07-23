#include "indicator.h"
#include "ti_msp_dl_config.h"
static uint16_t remain; static uint8_t phase;
void Indicator_init(void){remain=0;phase=0;DL_GPIO_setPins(GPIOA,BLUE_LED_PORT_BLUE_LED_PIN);DL_GPIO_clearPins(GPIOB,BUZZER_PORT_BUZZER_PIN);}
static void play(uint16_t ms){remain=ms;phase=1;}
void Indicator_point(void){play(100U);} void Indicator_done(void){play(300U);} void Indicator_fault(void){play(700U);}
void Indicator_task1ms(void){if(!remain)return; remain--; if(phase){DL_GPIO_clearPins(GPIOA,BLUE_LED_PORT_BLUE_LED_PIN);DL_GPIO_setPins(GPIOB,BUZZER_PORT_BUZZER_PIN);} if(!remain){DL_GPIO_setPins(GPIOA,BLUE_LED_PORT_BLUE_LED_PIN);DL_GPIO_clearPins(GPIOB,BUZZER_PORT_BUZZER_PIN);phase=0;}}

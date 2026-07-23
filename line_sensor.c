#include "line_sensor.h"
#include "app_config.h"
#include "ti_msp_dl_config.h"
bool LineSensor_blackLine(void){uint8_t i,n=0; for(i=0;i<8;i++){DL_GPIO_clearPins(GPIOB,GRAY_SERIAL_GRAY_CLK_PIN);delay_cycles(32);if((DL_GPIO_readPins(GPIOB,GRAY_SERIAL_GRAY_DAT_PIN)&GRAY_SERIAL_GRAY_DAT_PIN)==0U)n++;DL_GPIO_setPins(GPIOB,GRAY_SERIAL_GRAY_CLK_PIN);delay_cycles(160);}return n>=BLACK_SENSOR_MIN;}

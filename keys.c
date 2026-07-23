#include "keys.h"
#include "app_config.h"
#include "ti_msp_dl_config.h"
typedef struct { uint8_t count; uint8_t held; } K;
static K state[4];
static uint16_t releaseMs;
static uint8_t armed;
void Keys_init(void) { uint8_t i; for(i=0;i<4;i++) {state[i].count=0;state[i].held=0;} releaseMs=0U;armed=0U; }
Key_Event Keys_task1ms(void) {
 uint32_t b=DL_GPIO_readPins(GPIOB, MISSION_KEYS_KEY_Q2_PIN|MISSION_KEYS_KEY_Q3_PIN|MISSION_KEYS_KEY_Q4_PIN);
 uint8_t raw[4]={(DL_GPIO_readPins(GPIOA,START_KEY_KEY1_PIN)==0U),(b&MISSION_KEYS_KEY_Q2_PIN)!=0U,(b&MISSION_KEYS_KEY_Q3_PIN)!=0U,(b&MISSION_KEYS_KEY_Q4_PIN)!=0U}; uint8_t i;
 if(!armed){if(!raw[0]&&!raw[1]&&!raw[2]&&!raw[3]){if(releaseMs<KEY_STARTUP_RELEASE_MS)releaseMs++;if(releaseMs>=KEY_STARTUP_RELEASE_MS)armed=1U;}else releaseMs=0U;return KEY_NONE;}
 for(i=0;i<4;i++){if(raw[i]){if(state[i].count<KEY_DEBOUNCE_MS)state[i].count++; if(!state[i].held&&state[i].count==KEY_DEBOUNCE_MS){state[i].held=1;return (Key_Event)(i+1U);}}else{state[i].count=0;state[i].held=0;}}
 return KEY_NONE;
}

#include "mission.h"
#include "app_config.h"
#include "keys.h"
#include "gyro.h"
#include "indicator.h"
#include "line_sensor.h"
#include "lcd.h"
#include "motor.h"
#include "encoder.h"
typedef enum {IDLE,Q1_RUN,Q2_AB,Q2_CD,FINISHED,FAULT} State; static State s;static uint32_t ms,blackMs,lcdMs;static int16_t target;
static int16_t wrap(int16_t e){while(e>1800)e-=3600;while(e<-1800)e+=3600;return e;}
static void start(State n,int16_t t){s=n;target=t;ms=blackMs=0;Encoder_reset();Indicator_point();}
static void stop(bool bad){Motor_stop();s=bad?FAULT:FINISHED;if(bad)Indicator_fault();else Indicator_done();}
void Mission_init(void){s=IDLE;ms=blackMs=lcdMs=0;}
void Mission_beginQ2_CD(void){if(s==FINISHED||s==IDLE)start(Q2_CD,1800);}
void Mission_task1ms(void){bool permitBlack;Key_Event k=Keys_task1ms();Indicator_task1ms();if(++lcdMs>=LCD_PERIOD_MS){lcdMs=0;Lcd_showYaw(Gyro_yawDeciDeg());}if(s==IDLE){if(k==KEY_Q1)start(Q1_RUN,0);else if(k==KEY_Q2){if(Gyro_valid())start(Q2_AB,0);else Indicator_fault();}return;}if(s==FINISHED||s==FAULT){if(k!=KEY_NONE)s=IDLE;return;}if(++ms>MISSION_TIMEOUT_MS){stop(true);return;}if(s==Q1_RUN){Motor_drive(DRIVE_SPEED,DRIVE_SPEED);}else if((ms%HEADING_PERIOD_MS)==0U){int16_t e=wrap((int16_t)(target-Gyro_yawDeciDeg()));int16_t c=(int16_t)(e*HEADING_KP/10);if(c>HEADING_LIMIT)c=HEADING_LIMIT;if(c<-HEADING_LIMIT)c=-HEADING_LIMIT;Motor_drive((int16_t)(DRIVE_SPEED-c),(int16_t)(DRIVE_SPEED+c));}permitBlack=(s==Q1_RUN)?(Encoder_averagePulses()>=((uint32_t)Q1_MIN_DISTANCE_MM*ENCODER_PULSES_PER_M/1000U)):(ms>START_GUARD_MS);if(permitBlack&&LineSensor_blackLine()){if(blackMs<BLACK_CONFIRM_MS)blackMs++;if(blackMs>=BLACK_CONFIRM_MS)stop(false);}else blackMs=0;}

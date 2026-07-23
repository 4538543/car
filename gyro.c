#include "gyro.h"
#include "ti_msp_dl_config.h"
static volatile int16_t yaw,offset; static volatile bool valid,zeroCaptured; static uint8_t p,buf[4];
void Gyro_init(void){p=0;valid=false;zeroCaptured=false;yaw=offset=0;NVIC_ClearPendingIRQ(GYRO_UART_INST_INT_IRQN);NVIC_EnableIRQ(GYRO_UART_INST_INT_IRQN);}
int16_t Gyro_yawDeciDeg(void){return yaw;} bool Gyro_valid(void){return valid;}
void GYRO_UART_INST_IRQHandler(void){uint8_t x;if(DL_UART_Main_getPendingInterrupt(GYRO_UART_INST)!=DL_UART_MAIN_IIDX_RX)return;x=DL_UART_Main_receiveData(GYRO_UART_INST);if(p==0){if(x==0x5A)p=1;return;}if(p==1){if(x==0xBB)p=2;else p=0;return;}buf[p-2]=x;p++;if(p==5){uint8_t sum=(uint8_t)(0x5A+0xBB+buf[0]+buf[1]);if(sum==buf[2]){int16_t raw=(int16_t)((int32_t)(int16_t)(((uint16_t)buf[1]<<8)|buf[0])*1800/32768);if(!zeroCaptured){offset=raw;zeroCaptured=true;}yaw=(int16_t)(raw-offset);if(yaw>1800)yaw-=3600;if(yaw<-1800)yaw+=3600;valid=true;}p=0;}}

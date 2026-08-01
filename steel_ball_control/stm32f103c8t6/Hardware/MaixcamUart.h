#ifndef __MAIXCAM_UART_H
#define __MAIXCAM_UART_H

#include "stm32f10x.h"

typedef struct
{
	uint16_t frame_id;
	uint32_t timestamp_ms;
	uint16_t position;
	uint16_t center_x;
	uint16_t center_y;
	uint16_t target_position;
	uint8_t confidence;
	uint8_t candidate_count;
	uint8_t flags;
	uint8_t available;
	uint8_t target_available;
} MaixcamBallData;

void MaixcamUart_Init(void);
void MaixcamUart_RxIRQHandler(void);
uint8_t MaixcamUart_Poll(MaixcamBallData *ball);
uint32_t MaixcamUart_GetRxByteCount(void);
uint32_t MaixcamUart_GetValidFrameCount(void);
uint32_t MaixcamUart_GetCrcErrorCount(void);
uint32_t MaixcamUart_GetOverflowCount(void);

#endif

#include "MaixcamUart.h"
#include "misc.h"

/*
 * MaixCAM2 main.py protocol:
 *   A21/UART4_TX -> STM32 PA10/USART1_RX
 *   115200 baud, 8 data bits, no parity, 1 stop bit
 *   A5 5A + 22-byte payload + CRC16-CCITT (little endian)
 */
#define MAIXCAM_PAYLOAD_SIZE       22U
#define MAIXCAM_FRAME_SIZE         26U
#define MAIXCAM_RX_BUFFER_SIZE     128U

static uint8_t g_frame[MAIXCAM_FRAME_SIZE];
static uint8_t g_frame_index;
static volatile uint8_t g_rx_buffer[MAIXCAM_RX_BUFFER_SIZE];
static volatile uint8_t g_rx_write;
static volatile uint8_t g_rx_read;
static volatile uint32_t g_rx_byte_count;
static volatile uint32_t g_valid_frame_count;
static volatile uint32_t g_crc_error_count;
static volatile uint32_t g_overflow_count;

static uint16_t ReadU16Le(const uint8_t *data)
{
	return (uint16_t)((uint16_t)data[0] |
		((uint16_t)data[1] << 8));
}

static uint32_t ReadU32Le(const uint8_t *data)
{
	return (uint32_t)data[0] |
		((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) |
		((uint32_t)data[3] << 24);
}

static uint16_t Crc16Ccitt(const uint8_t *data, uint8_t length)
{
	uint16_t crc;
	uint8_t index;
	uint8_t bit;

	crc = 0xFFFFU;
	for (index = 0U; index < length; index++)
	{
		crc ^= (uint16_t)data[index] << 8;
		for (bit = 0U; bit < 8U; bit++)
		{
			if ((crc & 0x8000U) != 0U)
			{
				crc = (uint16_t)((crc << 1) ^ 0x1021U);
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}

static uint8_t TryReadByte(uint8_t *value)
{
	if (g_rx_read == g_rx_write)
	{
		return 0U;
	}

	*value = g_rx_buffer[g_rx_read];
	g_rx_read = (uint8_t)((g_rx_read + 1U) %
		MAIXCAM_RX_BUFFER_SIZE);
	return 1U;
}

void MaixcamUart_Init(void)
{
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef usart;
	NVIC_InitTypeDef nvic;

	g_frame_index = 0U;
	g_rx_write = 0U;
	g_rx_read = 0U;
	g_rx_byte_count = 0U;
	g_valid_frame_count = 0U;
	g_crc_error_count = 0U;
	g_overflow_count = 0U;

	RCC_APB2PeriphClockCmd(
		RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1,
		ENABLE);

	gpio.GPIO_Pin = GPIO_Pin_10;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	usart.USART_BaudRate = 115200U;
	usart.USART_WordLength = USART_WordLength_8b;
	usart.USART_StopBits = USART_StopBits_1;
	usart.USART_Parity = USART_Parity_No;
	usart.USART_HardwareFlowControl =
		USART_HardwareFlowControl_None;
	usart.USART_Mode = USART_Mode_Rx;
	USART_Init(USART1, &usart);

	nvic.NVIC_IRQChannel = USART1_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 1U;
	nvic.NVIC_IRQChannelSubPriority = 0U;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);
}

void MaixcamUart_RxIRQHandler(void)
{
	uint8_t next;
	uint8_t value;

	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		value = (uint8_t)USART_ReceiveData(USART1);
		g_rx_byte_count++;
		next = (uint8_t)((g_rx_write + 1U) %
			MAIXCAM_RX_BUFFER_SIZE);
		if (next != g_rx_read)
		{
			g_rx_buffer[g_rx_write] = value;
			g_rx_write = next;
		}
		else
		{
			g_overflow_count++;
		}
	}
}

uint8_t MaixcamUart_Poll(MaixcamBallData *ball)
{
	uint8_t value;
	uint16_t received_crc;
	uint16_t calculated_crc;

	if (ball == 0)
	{
		return 0U;
	}

	while (TryReadByte(&value) != 0U)
	{
		if (g_frame_index == 0U)
		{
			if (value == 0xA5U)
			{
				g_frame[g_frame_index++] = value;
			}
			continue;
		}

		if (g_frame_index == 1U)
		{
			if (value == 0x5AU)
			{
				g_frame[g_frame_index++] = value;
			}
			else
			{
				g_frame_index = (value == 0xA5U) ? 1U : 0U;
				if (g_frame_index != 0U)
				{
					g_frame[0] = value;
				}
			}
			continue;
		}

		g_frame[g_frame_index++] = value;
		if (g_frame_index == MAIXCAM_FRAME_SIZE)
		{
			g_frame_index = 0U;
			received_crc = ReadU16Le(&g_frame[24]);
			calculated_crc = Crc16Ccitt(
				&g_frame[2], MAIXCAM_PAYLOAD_SIZE);

			if ((received_crc == calculated_crc) &&
				(g_frame[2] == 2U))
			{
				ball->flags = g_frame[3];
				ball->frame_id = ReadU16Le(&g_frame[4]);
				ball->timestamp_ms = ReadU32Le(&g_frame[6]);
				ball->position = ReadU16Le(&g_frame[10]);
				ball->center_x = ReadU16Le(&g_frame[12]);
				ball->center_y = ReadU16Le(&g_frame[14]);
				ball->confidence = g_frame[20];
				ball->candidate_count = g_frame[21];
				ball->target_position =
					ReadU16Le(&g_frame[22]);
				ball->available =
					((ball->flags & 0x01U) != 0U) &&
					(ball->position != 0xFFFFU);
				ball->target_available =
					((ball->flags & 0x10U) != 0U) &&
					(ball->target_position != 0xFFFFU);
				g_valid_frame_count++;
				return 1U;
			}

			g_crc_error_count++;
		}
	}
	return 0U;
}

uint32_t MaixcamUart_GetRxByteCount(void)
{
	return g_rx_byte_count;
}

uint32_t MaixcamUart_GetValidFrameCount(void)
{
	return g_valid_frame_count;
}

uint32_t MaixcamUart_GetCrcErrorCount(void)
{
	return g_crc_error_count;
}

uint32_t MaixcamUart_GetOverflowCount(void)
{
	return g_overflow_count;
}

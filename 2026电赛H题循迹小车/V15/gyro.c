#include "gyro.h"

#include <limits.h>

#include "ti_msp_dl_config.h"

#define GYRO_FRAME_HEADER                  (0x5AU)
#define GYRO_TYPE_ANGULAR_RATE_Z           (0xAAU)
#define GYRO_TYPE_YAW                      (0xBBU)
#define GYRO_FRESH_TIMEOUT_MS              (500U)

typedef enum {
    PARSER_WAIT_HEADER = 0,
    PARSER_WAIT_TYPE,
    PARSER_WAIT_LOW,
    PARSER_WAIT_HIGH,
    PARSER_WAIT_CHECKSUM
} ParserState;

static volatile int32_t gAngularRateCentiDps;
static volatile int32_t gYawCentiDeg;
static volatile int32_t gIntegratedAngleCentiDeg;
static volatile uint32_t gAngularRateAgeMs;
static volatile uint32_t gValidFrameCount;
static int32_t gAngleResidualCentiDpsMs;

static ParserState gParserState;
static uint8_t gFrameType;
static uint8_t gFrameLow;
static uint8_t gFrameHigh;

static int16_t makeSignedRaw(uint8_t low, uint8_t high)
{
    return (int16_t) (((uint16_t) high << 8U) | (uint16_t) low);
}

static void acceptFrame(void)
{
    int32_t raw = (int32_t) makeSignedRaw(gFrameLow, gFrameHigh);

    if (gFrameType == GYRO_TYPE_ANGULAR_RATE_Z) {
        /* Module range is +/-2000 dps. Result unit is 0.01 dps. */
        gAngularRateCentiDps =
            (int32_t) (((int64_t) raw * 200000LL) / 32768LL);
        gAngularRateAgeMs = 0U;
        gValidFrameCount++;
    } else if (gFrameType == GYRO_TYPE_YAW) {
        /* Module yaw range is +/-180 degrees. Result unit is 0.01 degree. */
        gYawCentiDeg =
            (int32_t) (((int64_t) raw * 18000LL) / 32768LL);
        gValidFrameCount++;
    } else {
        /* A checksum-valid frame of another type is intentionally ignored. */
    }
}

static void parseByte(uint8_t byte)
{
    switch (gParserState) {
        case PARSER_WAIT_HEADER:
            if (byte == GYRO_FRAME_HEADER) {
                gParserState = PARSER_WAIT_TYPE;
            }
            break;

        case PARSER_WAIT_TYPE:
            gFrameType = byte;
            gParserState = PARSER_WAIT_LOW;
            break;

        case PARSER_WAIT_LOW:
            gFrameLow = byte;
            gParserState = PARSER_WAIT_HIGH;
            break;

        case PARSER_WAIT_HIGH:
            gFrameHigh = byte;
            gParserState = PARSER_WAIT_CHECKSUM;
            break;

        case PARSER_WAIT_CHECKSUM: {
            uint8_t expected = (uint8_t) (
                GYRO_FRAME_HEADER + gFrameType + gFrameLow + gFrameHigh);

            if (byte == expected) {
                acceptFrame();
                gParserState = PARSER_WAIT_HEADER;
            } else if (byte == GYRO_FRAME_HEADER) {
                /* The bad checksum byte can also be the next frame header. */
                gParserState = PARSER_WAIT_TYPE;
            } else {
                gParserState = PARSER_WAIT_HEADER;
            }
            break;
        }

        default:
            gParserState = PARSER_WAIT_HEADER;
            break;
    }
}

void Gyro_init(void)
{
    gAngularRateCentiDps = 0;
    gYawCentiDeg = 0;
    gIntegratedAngleCentiDeg = 0;
    gAngularRateAgeMs = UINT32_MAX;
    gValidFrameCount = 0U;
    gAngleResidualCentiDpsMs = 0;
    gParserState = PARSER_WAIT_HEADER;
    gFrameType = 0U;
    gFrameLow = 0U;
    gFrameHigh = 0U;

    while (!DL_UART_Main_isRXFIFOEmpty(GYRO_UART_INST)) {
        (void) DL_UART_Main_receiveData(GYRO_UART_INST);
    }

    NVIC_ClearPendingIRQ(GYRO_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(GYRO_UART_INST_INT_IRQN);
}

void Gyro_task1ms(void)
{
    if (gAngularRateAgeMs < UINT32_MAX) {
        gAngularRateAgeMs++;
    }

    if (Gyro_hasFreshAngularRate()) {
        gAngleResidualCentiDpsMs += gAngularRateCentiDps;
        while (gAngleResidualCentiDpsMs >= 1000) {
            gIntegratedAngleCentiDeg++;
            gAngleResidualCentiDpsMs -= 1000;
        }
        while (gAngleResidualCentiDpsMs <= -1000) {
            gIntegratedAngleCentiDeg--;
            gAngleResidualCentiDpsMs += 1000;
        }
    } else {
        gAngleResidualCentiDpsMs = 0;
    }
}

void Gyro_uartIrqHandler(void)
{
    if (DL_UART_Main_getPendingInterrupt(GYRO_UART_INST) ==
        DL_UART_MAIN_IIDX_RX) {
        while (!DL_UART_Main_isRXFIFOEmpty(GYRO_UART_INST)) {
            parseByte((uint8_t) DL_UART_Main_receiveData(GYRO_UART_INST));
        }
    }
}

bool Gyro_hasFreshAngularRate(void)
{
    return (gValidFrameCount > 0U) &&
           (gAngularRateAgeMs <= GYRO_FRESH_TIMEOUT_MS);
}

int32_t Gyro_getAngularRateCentiDps(void)
{
    return gAngularRateCentiDps;
}

int32_t Gyro_getYawCentiDeg(void)
{
    return gYawCentiDeg;
}

int32_t Gyro_getIntegratedAngleCentiDeg(void)
{
    return gIntegratedAngleCentiDeg;
}

uint32_t Gyro_getValidFrameCount(void)
{
    return gValidFrameCount;
}

#include "gyro.h"

#include "ti_msp_dl_config.h"

#define GYRO_FRAME_HEADER               (0x5AU)
#define GYRO_TYPE_RATE                  (0xAAU)
#define GYRO_TYPE_YAW                   (0xBBU)
#define GYRO_FRAME_SIZE                 (5U)
#define GYRO_ONLINE_TIMEOUT_MS          (500U)

volatile int16_t gGyroRawYaw;
volatile int16_t gGyroRawRate;
volatile uint32_t gGyroValidFrameCount;
volatile uint32_t gGyroChecksumErrorCount;
volatile uint16_t gGyroAgeMs;
volatile float gGyroCourseZeroDeg;

static volatile bool gHasYaw;
static volatile bool gHasRate;
static uint8_t gFrame[GYRO_FRAME_SIZE];
static uint8_t gFrameIndex;

static float Gyro_wrap180(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static void Gyro_parseByte(uint8_t byte)
{
    uint8_t sum;
    int16_t value;

    if (gFrameIndex == 0U) {
        if (byte != GYRO_FRAME_HEADER) {
            return;
        }
        gFrame[0] = byte;
        gFrameIndex = 1U;
        return;
    }

    gFrame[gFrameIndex++] = byte;
    if (gFrameIndex < GYRO_FRAME_SIZE) {
        return;
    }
    gFrameIndex = 0U;

    sum = (uint8_t) (gFrame[0] + gFrame[1] +
                     gFrame[2] + gFrame[3]);
    if (sum != gFrame[4]) {
        gGyroChecksumErrorCount++;
        return;
    }

    value = (int16_t) (((uint16_t) gFrame[3] << 8U) |
                       (uint16_t) gFrame[2]);
    if (gFrame[1] == GYRO_TYPE_RATE) {
        gGyroRawRate = value;
        gHasRate = true;
    } else if (gFrame[1] == GYRO_TYPE_YAW) {
        gGyroRawYaw = value;
        gHasYaw = true;
    } else {
        return;
    }

    gGyroAgeMs = 0U;
    gGyroValidFrameCount++;
}

static void Gyro_sendByte(uint8_t byte)
{
    while (DL_UART_isBusy(GYRO_UART_INST)) {
    }
    DL_UART_Main_transmitData(GYRO_UART_INST, byte);
}

static void Gyro_sendCommand(const uint8_t command[5])
{
    uint8_t i;
    for (i = 0U; i < 5U; i++) {
        Gyro_sendByte(command[i]);
    }
}

static void Gyro_delayMs(uint32_t ms)
{
    delay_cycles((CPUCLK_FREQ / 1000U) * ms);
}

void Gyro_init(void)
{
    gGyroRawYaw = 0;
    gGyroRawRate = 0;
    gGyroValidFrameCount = 0U;
    gGyroChecksumErrorCount = 0U;
    gGyroAgeMs = UINT16_MAX;
    gHasYaw = false;
    gHasRate = false;
    gFrameIndex = 0U;

    DL_UART_Main_enableInterrupt(
        GYRO_UART_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(GYRO_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(GYRO_UART_INST_INT_IRQN);
}

void Gyro_task1ms(void)
{
    if (gGyroAgeMs < UINT16_MAX) {
        gGyroAgeMs++;
    }
}

void Gyro_setOutputRate100Hz(void)
{
    static const uint8_t unlock[5] = {0x55U, 0xAAU, 0x13U, 0x8EU, 0x5FU};
    static const uint8_t rate100Hz[5] = {0x55U, 0xAAU, 0x02U, 0x09U, 0x00U};

    Gyro_sendCommand(unlock);
    Gyro_delayMs(100U);
    Gyro_sendCommand(rate100Hz);
    Gyro_delayMs(100U);
}

bool Gyro_isOnline(void)
{
    return (gGyroValidFrameCount != 0U) &&
           (gGyroAgeMs < GYRO_ONLINE_TIMEOUT_MS);
}

bool Gyro_hasYaw(void)
{
    return gHasYaw;
}

bool Gyro_hasRate(void)
{
    return gHasRate;
}

float Gyro_getYawDeg(void)
{
    return ((float) gGyroRawYaw * 180.0f) / 32768.0f;
}

float Gyro_getRateDps(void)
{
    return ((float) gGyroRawRate * 2000.0f) / 32768.0f;
}

bool Gyro_captureCourseZero(void)
{
    if (!gHasYaw) return false;
    gGyroCourseZeroDeg = Gyro_getYawDeg();
    return true;
}

float Gyro_getCourseYawDeg(void)
{
    return Gyro_wrap180(Gyro_getYawDeg() - gGyroCourseZeroDeg);
}

void Gyro_zeroYawAndSave(void)
{
    static const uint8_t unlock[5] = {0x55U, 0xAAU, 0x13U, 0x8EU, 0x5FU};
    static const uint8_t zeroYaw[5] = {0x55U, 0xAAU, 0x15U, 0x00U, 0x00U};
    static const uint8_t save[5] = {0x55U, 0xAAU, 0x00U, 0x00U, 0x00U};

    Gyro_sendCommand(unlock);
    Gyro_delayMs(100U);
    Gyro_sendCommand(zeroYaw);
    Gyro_delayMs(100U);
    Gyro_sendCommand(save);
}

void GYRO_UART_INST_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(GYRO_UART_INST)) {
        case DL_UART_IIDX_RX:
            Gyro_parseByte(
                (uint8_t) DL_UART_Main_receiveData(GYRO_UART_INST));
            break;
        default:
            break;
    }
}

#include "encoder.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"

static volatile int32_t gLeftCount;
static volatile int32_t gRightCount;
static uint8_t gLeftState;
static uint8_t gRightState;

/*
 * Quadrature transition table. Illegal two-bit jumps contribute zero, which
 * prevents a single noisy transition from creating a large count error.
 */
static const int8_t gQuadratureDelta[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static uint8_t readLeftState(void)
{
    uint8_t state = 0U;

    if (DL_GPIO_readPins(ENCODER_GPIO_PORT, ENCODER_GPIO_E1A_PIN) != 0U) {
        state |= 2U;
    }
    if (DL_GPIO_readPins(ENCODER_GPIO_PORT, ENCODER_GPIO_E1B_PIN) != 0U) {
        state |= 1U;
    }
    return state;
}

static uint8_t readRightState(void)
{
    uint8_t state = 0U;

    if (DL_GPIO_readPins(ENCODER_GPIO_PORT, ENCODER_GPIO_E2A_PIN) != 0U) {
        state |= 2U;
    }
    if (DL_GPIO_readPins(ENCODER_GPIO_PORT, ENCODER_GPIO_E2B_PIN) != 0U) {
        state |= 1U;
    }
    return state;
}

static uint32_t magnitude32(int32_t value)
{
    if (value >= 0) {
        return (uint32_t)value;
    }
    return (uint32_t)(-(int64_t)value);
}

void Encoder_init(void)
{
    gLeftCount = 0;
    gRightCount = 0;
    gLeftState = readLeftState();
    gRightState = readRightState();
}

void Encoder_reset(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    gLeftCount = 0;
    gRightCount = 0;
    gLeftState = readLeftState();
    gRightState = readRightState();

    if (primask == 0U) {
        __enable_irq();
    }
}

void Encoder_onGpioInterrupt(uint32_t gpioAStatus)
{
    const uint32_t leftMask = ENCODER_GPIO_E1A_PIN | ENCODER_GPIO_E1B_PIN;
    const uint32_t rightMask = ENCODER_GPIO_E2A_PIN | ENCODER_GPIO_E2B_PIN;

    if ((gpioAStatus & leftMask) != 0U) {
        uint8_t newState = readLeftState();
        int8_t delta = gQuadratureDelta[(gLeftState << 2U) | newState];
        gLeftState = newState;
        gLeftCount += (int32_t)delta * ENCODER_LEFT_DIRECTION_SIGN;
    }

    if ((gpioAStatus & rightMask) != 0U) {
        uint8_t newState = readRightState();
        int8_t delta = gQuadratureDelta[(gRightState << 2U) | newState];
        gRightState = newState;
        gRightCount += (int32_t)delta * ENCODER_RIGHT_DIRECTION_SIGN;
    }
}

void Encoder_getSnapshot(EncoderSnapshot *snapshot)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    snapshot->left = gLeftCount;
    snapshot->right = gRightCount;

    if (primask == 0U) {
        __enable_irq();
    }
}

uint32_t Encoder_getAverageMagnitude(void)
{
    EncoderSnapshot snapshot;
    Encoder_getSnapshot(&snapshot);

    return (magnitude32(snapshot.left) + magnitude32(snapshot.right)) / 2U;
}

uint32_t Encoder_countsToMillimeters(uint32_t counts)
{
    /*
     * circumference = pi * 65 mm = approximately 204.204 mm.
     * Use micrometers to retain precision without floating-point operations.
     */
    const uint32_t wheelCircumferenceUm = 204204U;
    uint64_t distanceUm = (uint64_t)counts * wheelCircumferenceUm;

    return (uint32_t)(distanceUm /
        ((uint64_t)ENCODER_COUNTS_PER_WHEEL_REV * 1000U));
}

#ifndef ARC_INTERFACE_H_
#define ARC_INTERFACE_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ARC_NONE = 0,
    ARC_RIGHT_C_TO_B,
    ARC_RIGHT_B_TO_C,
    ARC_LEFT_D_TO_A
} Arc_Request;

typedef enum {
    ARC_STATUS_IDLE = 0,
    ARC_STATUS_ACQUIRING,
    ARC_STATUS_TRACKING,
    ARC_STATUS_ALIGNING,
    ARC_STATUS_COMPLETE,
    ARC_STATUS_FAULT
} Arc_Status;

void ArcInterface_init(void);
void ArcInterface_request(Arc_Request request);
void ArcInterface_requestRelativeExit(Arc_Request request);
void ArcInterface_requestNoExitAlign(Arc_Request request);
void ArcInterface_requestFromDiagonal(Arc_Request request);
void ArcInterface_requestFromDiagonalNoExitAlign(Arc_Request request);
/* Gray/encoder-only bench test; does not require a live gyro stream. */
void ArcInterface_requestGrayPidTest(Arc_Request request);
void ArcInterface_task1ms(void);
Arc_Request ArcInterface_getRequest(void);
Arc_Status ArcInterface_getStatus(void);
void ArcInterface_markComplete(void); /* Optional manual/test override. */
bool ArcInterface_takeComplete(void);
bool ArcInterface_hasFault(void);

extern volatile unsigned int gArcRequestDebug;
extern volatile uint8_t gArcStatusDebug;
extern volatile uint8_t gArcFaultDebug;
extern volatile uint8_t gArcRawBitsDebug;
extern volatile int16_t gArcLineErrorDebug;
extern volatile int16_t gArcCorrectionDebug;
extern volatile int16_t gArcHeadingCorrectionDebug;
extern volatile uint16_t gArcHeadingBlendDebug;
extern volatile float gArcYawDebug;
extern volatile float gArcTargetYawDebug;
extern volatile float gArcAccumulatedYawDebug;
extern volatile uint32_t gArcSegmentPulsesDebug;

#endif

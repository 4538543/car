#ifndef GYRO_H_
#define GYRO_H_

#include <stdbool.h>
#include <stdint.h>

void Gyro_init(void);
void Gyro_task1ms(void);
/* Applies 100 Hz output until power-off; avoids repeatedly writing module flash. */
void Gyro_setOutputRate100Hz(void);

bool Gyro_isOnline(void);
bool Gyro_hasYaw(void);
bool Gyro_hasRate(void);
float Gyro_getYawDeg(void);
float Gyro_getRateDps(void);
/* Defines the current car heading as the software course-frame 0 degrees. */
bool Gyro_captureCourseZero(void);
/* Signed yaw relative to the captured course zero, wrapped to -180...180. */
float Gyro_getCourseYawDeg(void);

/* Sends unlock -> yaw zero -> save. Keep the car still while calling. */
void Gyro_zeroYawAndSave(void);

/* Debug symbols visible in the CCS Expressions window. */
extern volatile int16_t gGyroRawYaw;
extern volatile int16_t gGyroRawRate;
extern volatile uint32_t gGyroValidFrameCount;
extern volatile uint32_t gGyroChecksumErrorCount;
extern volatile uint16_t gGyroAgeMs;
extern volatile float gGyroCourseZeroDeg;

#endif

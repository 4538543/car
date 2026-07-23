#include "mission.h"

#include <stdbool.h>

#include "app_config.h"
#include "arc_interface.h"
#include "encoder.h"
#include "gyro.h"
#include "indicator.h"
#include "line_sensor.h"
#include "motor.h"
#include "oled.h"
#include "straight_control.h"
#include "task_buttons.h"
#include "turn_control.h"

typedef enum {
    MODE_NONE = 0, MODE_Q1, MODE_Q2, MODE_Q3, MODE_Q4
} MissionMode;

typedef enum {
    STATE_IDLE = 0,
    STATE_STRAIGHT,
    STATE_TURNING,
    STATE_WAIT_ARC,
    STATE_BRAKING,
    STATE_FINISHED,
    STATE_FAULT
} MissionState;

/* Each route owns explicit phases. This prevents a Q3/Q4 transition change
 * from being interpreted as a Q2 segment with the same numeric index. */
typedef enum {
    PHASE_NONE = 0,
    PHASE_Q1_AB,
    PHASE_Q2_AB,
    PHASE_Q2_BC_ARC,
    PHASE_Q2_CD,
    PHASE_Q2_DA_ARC,
    PHASE_Q2_PID_TEST,
    PHASE_FIG_AC_TURN,
    PHASE_FIG_AC_STRAIGHT,
    PHASE_FIG_CB_ARC,
    PHASE_FIG_BD_TURN,
    PHASE_FIG_BD_STRAIGHT,
    PHASE_FIG_DA_ARC
} MissionPhase;

volatile uint8_t gMissionMode;
volatile uint8_t gMissionState;
volatile uint8_t gMissionFault;
volatile uint8_t gMissionLap;
volatile uint8_t gMissionPhase;

static uint16_t gBrakeMs;

static void finish(bool fault)
{
    Motor_brakeAll();
    gBrakeMs = BRAKE_TIME_MS;
    gMissionFault = fault ? 1U : 0U;
    gMissionState = fault ? STATE_FAULT : STATE_BRAKING;
    if (fault) Indicator_fault(); else Indicator_finished();
}

static void startQ1Straight(float distanceMm, MissionPhase phase)
{
    StraightControl_startQ1Test(distanceMm);
    gMissionPhase = (uint8_t)phase;
    gMissionState = STATE_STRAIGHT;
}

static void startAbsoluteStraight(float distanceMm, float targetYawDeg,
                                  MissionPhase phase)
{
    StraightControl_startAbsolute(distanceMm, targetYawDeg);
    gMissionPhase = (uint8_t)phase;
    gMissionState = STATE_STRAIGHT;
}

static void startAbsoluteTurn(float targetYawDeg, MissionPhase phase)
{
    if (!TurnControl_startAbsolute(targetYawDeg)) {
        finish(true);
        return;
    }
    gMissionPhase = (uint8_t)phase;
    gMissionState = STATE_TURNING;
}

static void startArc(Arc_Request request, bool steepEntry,
                     bool skipExitAlign, MissionPhase phase)
{
    if (steepEntry && skipExitAlign) {
        ArcInterface_requestFromDiagonalNoExitAlign(request);
    } else if (steepEntry) {
        ArcInterface_requestFromDiagonal(request);
    } else if (skipExitAlign) {
        ArcInterface_requestNoExitAlign(request);
    } else {
        ArcInterface_request(request);
    }
    gMissionPhase = (uint8_t)phase;
    gMissionState = STATE_WAIT_ARC;
}

static void startFigureEightLap(void)
{
    /* Q3/Q4 deliberately use the robust discrete connection: first turn to
     * the absolute diagonal course, then drive a straight chord to C. */
    startAbsoluteTurn(DIAGONAL_AC_HEADING_DEG, PHASE_FIG_AC_TURN);
}

static void startGrayPidTest(void)
{
    ArcInterface_requestGrayPidTest(ARC_RIGHT_B_TO_C);
    gMissionPhase = PHASE_Q2_PID_TEST;
    gMissionState = STATE_WAIT_ARC;
}

static void startMode(MissionMode mode)
{
    Motor_coastAll();
    Motor_disable();
    ArcInterface_init();
    gMissionMode = (uint8_t)mode;
    gMissionFault = 0U;
    gMissionLap = 0U;
    gMissionPhase = PHASE_NONE;
    Indicator_start();

    /* One common course frame for every controller: the heading at the mode
     * button press is 0 degrees. This is a RAM-only offset and does not write
     * or re-zero the gyro module itself. */
    if (!Gyro_captureCourseZero()) {
        if ((mode != MODE_Q1) && (mode != MODE_Q2)) {
            finish(true);
            return;
        }
    }

    switch (mode) {
        case MODE_Q1:
            /* Q1/Q2 capture the actual yaw at the button press. A small gyro
             * zero residual must not create a visible steering hook. */
            startQ1Straight(STRAIGHT_AB_CD_MM, PHASE_Q1_AB);
            break;
        case MODE_Q2:
            /* Temporary PB22 tuning mode: place the gray array on the
             * clockwise arc, then run only one semicircle. Skip exit heading
             * alignment so the observed motion contains gray PID only. */
            startGrayPidTest();
            break;
        case MODE_Q3:
        case MODE_Q4:
            startFigureEightLap();
            break;
        default:
            finish(true);
            break;
    }
}

static void straightDone(void)
{
    switch ((MissionPhase)gMissionPhase) {
        case PHASE_Q1_AB:
            finish(false);
            break;
        case PHASE_Q2_AB:
            startArc(ARC_RIGHT_B_TO_C, false, false, PHASE_Q2_BC_ARC);
            break;
        case PHASE_Q2_CD:
            startArc(ARC_LEFT_D_TO_A, false, false, PHASE_Q2_DA_ARC);
            break;
        case PHASE_FIG_AC_STRAIGHT:
            if ((MissionMode)gMissionMode == MODE_Q3) {
                finish(false);
                break;
            }
            startArc(ARC_RIGHT_C_TO_B, true, true, PHASE_FIG_CB_ARC);
            break;
        case PHASE_FIG_BD_STRAIGHT:
            startArc(ARC_LEFT_D_TO_A, true, false, PHASE_FIG_DA_ARC);
            break;
        default:
            finish(true);
            break;
    }
}

static void arcDone(void)
{
    switch ((MissionPhase)gMissionPhase) {
        case PHASE_Q2_BC_ARC:
            startAbsoluteStraight(STRAIGHT_AB_CD_MM,
                COURSE_CD_HEADING_DEG, PHASE_Q2_CD);
            break;
        case PHASE_Q2_DA_ARC:
        case PHASE_Q2_PID_TEST:
            finish(false);
            break;
        case PHASE_FIG_CB_ARC:
            startAbsoluteTurn(DIAGONAL_BD_HEADING_DEG,
                PHASE_FIG_BD_TURN);
            break;
        case PHASE_FIG_DA_ARC:
            gMissionLap++;
            if (((MissionMode)gMissionMode == MODE_Q3) ||
                (gMissionLap >= Q4_LAPS)) {
                finish(false);
            } else {
                startFigureEightLap();
            }
            break;
        default:
            finish(true);
            break;
    }
}

static void turnDone(void)
{
    switch ((MissionPhase)gMissionPhase) {
        case PHASE_FIG_AC_TURN:
            startAbsoluteStraight(STRAIGHT_AC_BD_MM,
                DIAGONAL_AC_HEADING_DEG, PHASE_FIG_AC_STRAIGHT);
            break;
        case PHASE_FIG_BD_TURN:
            startAbsoluteStraight(STRAIGHT_AC_BD_MM,
                DIAGONAL_BD_HEADING_DEG, PHASE_FIG_BD_STRAIGHT);
            break;
        default:
            finish(true);
            break;
    }
}

void Mission_init(void)
{
    TaskButtons_init();
    Indicator_init();
    Motor_init();
    Encoder_init();
    LineSensor_init();
    Gyro_init();
    Gyro_setOutputRate100Hz();
    OLED_init();
    ArcInterface_init();
    gMissionMode = MODE_NONE;
    gMissionState = STATE_IDLE;
    gMissionFault = 0U;
    gMissionLap = 0U;
    gMissionPhase = PHASE_NONE;
    gBrakeMs = 0U;
}

void Mission_task1ms(void)
{
    TaskButton_Event key;

    Gyro_task1ms();
    OLED_task1ms();
    Indicator_task1ms();

    /* A new debounced mode always resets the old arc controller before any
     * motion state runs, so tasks cannot overwrite each other's motor output. */
    key = TaskButtons_poll1ms();
    if (key == TASK_BUTTON_Q1) startMode(MODE_Q1);
    else if (key == TASK_BUTTON_Q2) startMode(MODE_Q2);
    else if (key == TASK_BUTTON_Q3) startMode(MODE_Q3);
    else if (key == TASK_BUTTON_Q4) startMode(MODE_Q4);

    ArcInterface_task1ms();

    if (gMissionState == STATE_STRAIGHT) {
        Straight_Result result = StraightControl_task1ms();
        if (result == STRAIGHT_REACHED) straightDone();
        else if (result == STRAIGHT_TIMEOUT) finish(true);
    } else if (gMissionState == STATE_TURNING) {
        Turn_Result result = TurnControl_task1ms();
        if (result == TURN_REACHED) turnDone();
        else if (result == TURN_FAULT) finish(true);
    } else if (gMissionState == STATE_WAIT_ARC) {
        if (ArcInterface_hasFault()) {
            finish(true);
        } else if (ArcInterface_takeComplete()) {
            arcDone();
        }
    } else if ((gMissionState == STATE_BRAKING) ||
               (gMissionState == STATE_FAULT)) {
        if (gBrakeMs > 0U) gBrakeMs--;
        if (gBrakeMs == 0U) {
            Motor_coastAll();
            Motor_disable();
            if (gMissionState == STATE_BRAKING) {
                gMissionState = STATE_FINISHED;
            }
        }
    }
}

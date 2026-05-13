#include "ThrusterControl.h"

#define THRUSTER_TIMER_PERIOD_TICKS     24999

#define THRUSTER_REVERSE_TICKS          1250
#define THRUSTER_NEUTRAL_TICKS          1875
#define THRUSTER_FORWARD_TICKS          2500

#define THRUSTER_FULL_OFFSET_TICKS      (THRUSTER_FORWARD_TICKS - THRUSTER_NEUTRAL_TICKS)
#define THRUSTER_MAX_CMD                127

static uint16_t CommandToTicks(int16_t cmd);

// sets up the output compares, timer, and pins associated with the thrusters
void ThrusterControl_Init(void)
{
    // Timer3 at 50 Hz using Fcy = 20 MHz, prescaler = 1:16
    T3CONbits.ON = 0;
    T3CONbits.TCS = 0;
    T3CONbits.TCKPS = 0b100; 
    PR3 = THRUSTER_TIMER_PERIOD_TICKS;
    TMR3 = 0;

    // OC2 -> RB8
    TRISBbits.TRISB8 = 0;
    RPB8Rbits.RPB8R = 0b0101;

    // OC3 -> RB9
    TRISBbits.TRISB9 = 0;
    RPB9Rbits.RPB9R = 0b0101;

    // OC2 setup
    OC2CONbits.ON = 0;
    OC2CONbits.OCTSEL = 1;
    OC2R = THRUSTER_NEUTRAL_TICKS;
    OC2RS = THRUSTER_NEUTRAL_TICKS;
    OC2CONbits.OCM = 0b110;

    // OC3 setup
    OC3CONbits.ON = 0;
    OC3CONbits.OCTSEL = 1;
    OC3R = THRUSTER_NEUTRAL_TICKS;
    OC3RS = THRUSTER_NEUTRAL_TICKS;
    OC3CONbits.OCM = 0b110;

    T3CONbits.ON = 1;
    OC2CONbits.ON = 1;
    OC3CONbits.ON = 1;
}


// x: left/right, y: forward/backward
void ThrusterControl_SetThrust(uint8_t xJoyStick, uint8_t yJoyStick)
{
    int16_t forwardCmd;
    int16_t turnCmd;
    int16_t leftCmd;
    int16_t rightCmd;

    forwardCmd = (int16_t)yJoyStick - THRUSTER_CONTROL_JOY_CENTER;
    turnCmd = (int16_t)xJoyStick - THRUSTER_CONTROL_JOY_CENTER;

    leftCmd = forwardCmd + turnCmd;
    rightCmd = forwardCmd - turnCmd;

    ThrusterControl_SetRaw(leftCmd, rightCmd);
}


// cmd range is roughly -127 to +127
void ThrusterControl_SetRaw(int16_t leftCmd, int16_t rightCmd)
{
    if (leftCmd > THRUSTER_MAX_CMD) {
        leftCmd = THRUSTER_MAX_CMD;
    } else if (leftCmd < -THRUSTER_MAX_CMD) {
        leftCmd = -THRUSTER_MAX_CMD;
    }

    if (rightCmd > THRUSTER_MAX_CMD) {
        rightCmd = THRUSTER_MAX_CMD;
    } else if (rightCmd < -THRUSTER_MAX_CMD) {
        rightCmd = -THRUSTER_MAX_CMD;
    }

    OC2RS = CommandToTicks(leftCmd);
    OC3RS = CommandToTicks(rightCmd);
}


static uint16_t CommandToTicks(int16_t cmd)
{
    int32_t ticks;
    int32_t maxOffsetTicks;

    maxOffsetTicks = (int32_t)(THRUSTER_FULL_OFFSET_TICKS *
                              THRUSTER_CONTROL_MAX_THRUST);

    ticks = THRUSTER_NEUTRAL_TICKS;
    ticks += ((int32_t)cmd * maxOffsetTicks) / THRUSTER_MAX_CMD;

    if (ticks > THRUSTER_FORWARD_TICKS) {
        ticks = THRUSTER_FORWARD_TICKS;
    } else if (ticks < THRUSTER_REVERSE_TICKS) {
        ticks = THRUSTER_REVERSE_TICKS;
    }

    return (uint16_t)ticks;
}
/****************************************************************************
 Module
   Servo_HAL.c

 Revision
   1.0.1

 Description
   Servo Motor HAL for simplifying motor direction and angle in other modules

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 03/03/26 11:53 DA       Creation of HAL
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Servo_HAL.h"
#include "dbprintf.h"
#include <sys/attribs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <xc.h>

/*----------------------------- Module Defines ----------------------------*/
#define ARM_SERVO_MAX_ANGLE   270  // Degrees
#define MICRO_SERVO_MAX_ANGLE 180  // Degrees
#define MICRO_SERVO_TICK_RANGE 5000
#define ARM_SERVO_TICK_RANGE 5000 


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine. They should be functions
   relevant to the behavior of this state machine
*/
uint16_t servoAngleToTicks(Servo_t whichServo, uint16_t angle);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

/*
Min - Max Pulse Periods
  Arm Servo: 500-2500 microsec
  Micro Servo: 1000-2000 microsec

Pre-Scaler: 1:8
20M / 8 = 2.5M ticks / sec
0.4 microseconds per tick
*/

// Manufacturer specified periods converted to ticks using 1:8 pre-scaler
uint16_t armServoMinTicks = 1250;	   // 500 us / 0.4 us/tick
uint16_t armServoMaxTicks = 6250;    // 2500 us / 0.4 us/tick

uint16_t microServoMinTicks = 1250;
uint16_t microServoMaxTicks = 6250;

// Set OCs for the servos
static ServoSetup_t Servos[] = {
  {&OC2RS}    // Battery Servo  
};

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
  servoSetAngle

 Parameters
  Servo_t servo (SERVO_ARM, SERVO_PADDLE, or SERVO_FLAG), angle (unsigned).
  Arm servo range: 0-270 deg
  Other servos (micro): 0-180 deg

 Returns
  None

 Description
  Takes in servo motor and angle and sets the appropriate motor to that angle
   
 Notes

 Author
     Dani Algazi, 3/03/26, 17:54
****************************************************************************/
void servoSetAngle(Servo_t whichServo, int16_t angle) {
  
  // Convert angle to ticks
  uint16_t angleInTicks = servoAngleToTicks(whichServo, angle);

  const ServoSetup_t *currentServo = &Servos[whichServo];
  *currentServo->pin_OCxRS = angleInTicks;

}


/****************************************************************************
 Function
  servoInit

 Parameters
  None

 Returns
  None

 Description
  Sets up both motors with PWM channels, pin assignments, etc.
   
 Notes

 Author
     Dani Algazi, 3/03/26, 17:54
****************************************************************************/
void servoInit(void) {

  // ---------- Digital Output Pin Setup -------------
  // Set pins to digital pins (0)
  //ANSELBbits.ANSB8 = 0;   


  // Set pins to output pins (0)
  TRISBbits.TRISB8 = 0;   

  // ---------- Timer 3 Setup -------------
  T3CONbits.ON = 0;         // Stop Timer
  T3CONbits.TCKPS = 0b011;   // 1:8 prescaler
  PR3 = 49999;              // 20MHZ / (8*(PR2+1)) = 50 Hz (recommended for these servos)
  TMR3 = 0;                 // Set timer to 0 at start (note: previously set tmr2=pr2 which worked)
  T3CONbits.ON = 1;         // Start Timer

  // ---------- PWM and OC Setup -------------

  // Turn off OC 2 during setup
  OC2CONbits.ON = 0; 

  // Set OC2 as PWM mode
  OC2CONbits.OCM = 0b110;

  // Use timer 3 for OC 1, 2, and 5
  OC2CONbits.OCTSEL = 1;


  // Initialize OCxR and OCxRS for 3 and 4
  OC2R = 0;
  OC2RS = 0;

  // Map OC to PPS (Table 11-2)
  /* OC to PPS code in PIC 32
  OC1	  0b0101
  OC2	  0b0101
  OC3	  0b0101
  OC4	  0b0101
  OC5	  0b0110
  */

  RPB8R = 0b0101;     // Map OC2 to RB8 (Battery Servo)

  // Turn on OC2 to finish setup  
  OC2CONbits.ON = 1;
}


// ************************** PRIVATE FUNCTIONS ***************************

/****************************************************************************
 Function
  servoAngleToTicks

 Parameters
  Which servo (arm, paddle, flag), and desired angle in degrees

 Returns
  Angle in ticks

 Description
  Takes in a servo motor and angle, and sets that servo to the desired angle.

 Notes

 Author
     Dani Algazi, 3/03/26, 17:54
****************************************************************************/
uint16_t servoAngleToTicks(Servo_t whichServo, uint16_t angle) {

  uint16_t ticks = 0;

  if ((whichServo == SERVO_BATTERY)) {
    // Guard against out of bounds angles
    if (angle < 0) angle = 0;
    if (angle > MICRO_SERVO_MAX_ANGLE) angle = MICRO_SERVO_MAX_ANGLE;
    // Convert angles to ticks
    ticks = microServoMinTicks + ((uint32_t) angle * MICRO_SERVO_TICK_RANGE) / MICRO_SERVO_MAX_ANGLE;

  } else {
    return 0;
  }

  return ticks;
}
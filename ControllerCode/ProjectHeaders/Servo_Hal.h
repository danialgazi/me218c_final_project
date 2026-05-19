/****************************************************************************
 Module
   Servo_HAL.h

 Revision
   1.0.1

 Description
   Servo HAL header for simplifying Servo direction, speed, etc. in other functions

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 03/03/26 13:31 DA       Creation of HAL
****************************************************************************/

#ifndef Servo_HAL
#define Servo_HAL

#include <stdbool.h> 
#include <stdint.h>

typedef enum { 
  SERVO_BATTERY = 0
  // Add more if wanted  
} Servo_t;

typedef struct {
  volatile uint32_t * const pin_OCxRS;   // Servo Pin duty cycle OC
} ServoSetup_t;


// ******************** PUBLIC FUNCTIONS ***********************

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
   
Example
  servoSetAngle(SERVO_ARM, 210);
****************************************************************************/
void servoSetAngle(Servo_t whichServo, int16_t angle);


/****************************************************************************
 Function
  servoInit

 Parameters
  None

 Returns
  None

 Description
  Sets up both motors with PWM channels, pin assignments, etc.
   
Example
  
****************************************************************************/
void servoInit(void);

#endif //Servo_HAL defined

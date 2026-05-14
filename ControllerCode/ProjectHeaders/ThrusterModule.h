/****************************************************************************

  Header file for Thruster Module
  Interprets raw Thruster ADC values from ADService into thruster power 
  information for the motors.

 ****************************************************************************/

#ifndef ThrusterModule_H
#define ThrusterModule_H

#include <stdint.h>
#include <stdbool.h>

// Public Function Prototypes
uint8_t ThrusterModule_GetJoy1(void);            // Public getter fxn for joystick1 data
uint8_t ThrusterModule_GetJoy2(void);            // Public getter fxn for joystick2 data

// --------------------------------------------------
// Private Function Prototypes
void ThrusterModule_EncodeThrusterPower(void);   // call periodically to encode thruster power

#endif /* ThrusterModule_H */

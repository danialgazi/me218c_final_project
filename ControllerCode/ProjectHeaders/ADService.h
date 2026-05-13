/****************************************************************************

  Header file for AD service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ServAD_H
#define ServAD_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitADService(uint8_t Priority);
bool PostADService(ES_Event_t ThisEvent);
ES_Event_t RunADService(ES_Event_t ThisEvent);

// Functions for getting data from analog channels
uint32_t getIMUX(void);   // IMU x-axis
uint32_t getIMUY(void);   // IMU y-axis
uint32_t getIMUZ(void);   // IMU z-axis
uint32_t getJoystickX(void);  // Joystick x-direction
uint32_t getJoystickY(void);  // Joystick y-direction
uint32_t getBoatSelectVal(void);  // Boat select potentiometer
uint8_t ADC10ToByte(uint32_t adcVal); // Convert 10-bit ADC value to 8-bit byte

#endif /* ServAD_H */


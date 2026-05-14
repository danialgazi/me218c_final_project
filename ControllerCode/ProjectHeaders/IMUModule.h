/****************************************************************************

  Header file for IMU Module
  Interprets raw IMU ADC values from ADService into shake detection events

 ****************************************************************************/

#ifndef IMUModule_H
#define IMUModule_H

#include <stdint.h>
#include <stdbool.h>

// Public Function Prototypes

void IMUModule_Check4Shake(void);   // call periodically to check for shake

#endif /* IMUModule_H */

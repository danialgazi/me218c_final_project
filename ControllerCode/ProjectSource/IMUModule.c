/****************************************************************************
 Module
   IMUModule.c

 Revision
   1.0.0

 Description
   Interprets raw IMU ADC values from ADService. Detects shake events
   and posts them to the appropriate state machine.

 History
 When           Who     What/Why
 -------------- ---     --------
 05/10/26       DA      scaffolding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "IMUModule.h"
#include "ADService.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define SHAKE_THRESHOLD 590000  // TODO: tune this value

/*---------------------------- Module Variables ---------------------------*/

/*------------------------------ Module Code ------------------------------*/

void IMUModule_Check4Shake(void)
{
  uint32_t imuX = getIMUX();
  uint32_t imuY = getIMUY();
  uint32_t imuZ = getIMUZ();

  uint32_t magSq = (imuX * imuX) + (imuY * imuY) + (imuZ * imuZ);
  DB_printf("MagSq = %u\r\n", magSq);
  
  // Only post if IMU crosses the threshold
  if (magSq > SHAKE_THRESHOLD)
  {
    DB_printf("Shake Detected/r/n");
    ES_Event_t ShakeEvent;
    ShakeEvent.EventType = ES_IMU_SHAKE_DETECTED; 
    ES_PostAll(ShakeEvent);       // TODO: CHANGE THIS TO REAL SM
  }
}


/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

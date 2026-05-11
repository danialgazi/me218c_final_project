/****************************************************************************
 Module
   TestAnalogInputsService.c

 Revision
   1.0.1

 Description
   This is a TestAnalogInputs file for implementing a simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TestAnalogInputsService.h"
#include "ThrusterModule.h"
/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTestAnalogInputsService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitTestAnalogInputsService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/

  // Set pins to analog
  ANSELAbits.ANSA0 = 1;   // AN0, joystick X
  ANSELAbits.ANSA1 = 1;   // AN1, joystick Y
  ANSELBbits.ANSB2 = 1;   // AN4, boat select
  ANSELBbits.ANSB3 = 1;   // AN5, IMU X
  ANSELBbits.ANSB12 = 1;  // AN12, IMU Y
  ANSELBbits.ANSB13 = 1;  // AN11, IMU Z
  
  // Set analog pins to input
  TRISAbits.TRISA0 = 1;    // AN0, joystick X
  TRISAbits.TRISA1 = 1;     // AN1, joystick Y
  TRISBbits.TRISB2 = 1;     // AN4, boat select
  TRISBbits.TRISB3 = 1;     // AN5, IMU X
  TRISBbits.TRISB12 = 1;    // AN12, IMU Y
  TRISBbits.TRISB13 = 1;    // AN11, IMU Z   

  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostTestAnalogInputsService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostTestAnalogInputsService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTestAnalogInputsService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunTestAnalogInputsService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/

  if (ThisEvent.EventType == ES_IMU_SHAKE_DETECTED) {
    DB_printf("IMU Shake Detected\r\n");
  }

  uint8_t currentThrustByte = ThrusterModule_GetJoy1();
  uint8_t currentSteerByte = ThrusterModule_GetJoy2();
  DB_printf("Joy1 (thrust): %u  Joy2 (steer): %u\r\n", currentThrustByte, currentSteerByte);

  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/


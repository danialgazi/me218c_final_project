/****************************************************************************
 Module
   ADService.c

 Revision
   1.0.1

 Description
   This service reads and converts analog to digital. Taken from the Events
   and Services Template file.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
 01/13/26 21:50 DA       start of file for lab5. pseudo code to first draft
 01/20/26 23:00 DA       adaptation to lab6 (PWM and DC motor driving)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ADService.h"
#include "TestAnalogInputsService.h"
#include "PIC32_AD_Lib.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define AD_TIMER_PERIOD  50   //  50 ms
// Analog Pins AN0, AN1, AN4, AN5, AN12, AN11
#define ANALOG_PINS (BIT0HI | BIT1HI | BIT4HI | BIT5HI | BIT12HI | BIT11HI)
#define NUM_ANALOG_PINS 6

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint32_t imuX, imuY, imuZ, potVal, joystickX, joystickY, boatSelectPotVal;
static uint32_t currentPotVal = 0; // out of 1023
uint32_t ResultsArray[NUM_ANALOG_PINS];   // An array to store ADC_MultiRead vals

// Results array should be size = number of analog channels

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitADService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     Daniel Algazi, 1/13/2026, 21:51
****************************************************************************/
bool InitADService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/

  // AD Configure
  //ADC_ConfigAutoScan(POT_PIN);  // Configure pot data pin for analog

  
  // Set pins to digital/analog
  //ANSELAbits.ANSA1 = 1;     // RB2 (AN4) to analog (pot data pin)

  // Set pins to input/output
  //TRISAbits.TRISA1 =  1;    // RB2 to input (pot data pin)

  // Configure and enable the ADC using your library function
  if (!ADC_ConfigAutoScan(BIT0HI | BIT1HI | BIT4HI | BIT5HI | BIT11HI | BIT12HI)) {
    DB_printf("ILLEGAL PIN FOR ADC");
  }
  
  // Start AD Timer to keep track of potentiometer polling frequency
  ES_Timer_InitTimer(AD_TIMER, AD_TIMER_PERIOD);

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
     PostADService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     Daniel Algazi, 1/13/2026, 21:51
****************************************************************************/
bool PostADService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunADService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
     Daniel Algazi, 1/13/2026, 21:51
****************************************************************************/
ES_Event_t RunADService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  // //DB_printf("RunADService: ES_TIMOUT \n");

  // Check if AD Timer elapsed
  if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == AD_TIMER) {

    // Get Current potentiometer value
    uint32_t ResultsArray[NUM_ANALOG_PINS]; 
    // AD1CSSL = ANALOG_PINS;         
    ADC_MultiRead(ResultsArray);
    
    // Set current value of each sensor from multiread array contents
    // Map the numerically sorted results to the controller variables
    joystickX = ResultsArray[0];           // AN0: RVx (Joystick X)
    joystickY = ResultsArray[1];           // AN1: RVy (Joystick Y)
    boatSelectPotVal = ResultsArray[2];    // AN4: Boat Select 
    imuX = ResultsArray[3];                // AN5: AccelX
    imuZ = ResultsArray[4];                // AN11: AccelZ (sorted before 12)
    imuY = ResultsArray[5];                // AN12: AccelY

    ////DB_printf("Line Sensor Value %u\r\n", (unsigned int)currentPotVal);

    uint32_t currentMagSq = (imuX * imuX) + (imuY * imuY) + (imuZ * imuZ);

    // Debug prints
    // DB_printf("IMU X Value: %u\r\n", imuX);
    // DB_printf("IMU Y Value: %u\r\n", imuY);
    // DB_printf("IMU Z Value: %u\r\n", imuZ);
    // DB_printf("IMU Magnitude Value: %u\r\n", currentMagSq);
    // DB_printf("Joystick X Value: %u\r\n", joystickX);
    DB_printf("Joystick Y Value: %u\r\n", joystickY);
    // DB_printf("Boat Select Potentiometer Value: %u\r\n", boatSelectPotVal);

    // Reset timer
    ES_Timer_InitTimer(AD_TIMER, AD_TIMER_PERIOD);
    
  }

  return ReturnEvent;
}

/*
  These functions let you access the analog input data from the controller
  as values from 0-1023
*/
uint32_t getIMUX(void) { return imuX; }
uint32_t getIMUY(void) { return imuY; }
uint32_t getIMUZ(void) { return imuZ; }
uint32_t getJoystickX(void) { return joystickX; }
uint32_t getJoystickY(void) { return joystickY; }
uint32_t getBoatSelectVal(void) { return boatSelectPotVal; }


/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/


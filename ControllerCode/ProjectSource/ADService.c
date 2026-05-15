/****************************************************************************
 Module
   ADService.c

 Description
   Service for reading analog inputs using PIC32_AD_Lib auto-scan mode.
****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ADService.h"
#include "PIC32_AD_Lib.h"
#include "dbprintf.h"
#include <stdint.h>
#include <stdbool.h>

/*----------------------------- Module Defines ----------------------------*/

#define AD_TIMER_PERIOD  50   // ms

// Analog channels being scanned:
// AN0  = joystick X
// AN1  = joystick Y
// AN4  = boat select potentiometer
// AN5  = IMU X
// AN11 = IMU Z
// AN12 = IMU Y
#define ANALOG_PINS       (BIT0HI | BIT1HI | BIT4HI | BIT5HI | BIT11HI | BIT12HI)
#define NUM_ANALOG_PINS   6

/*---------------------------- Module Functions ---------------------------*/

static void initAD(void);

/*---------------------------- Module Variables ---------------------------*/

static uint8_t MyPriority;

static uint32_t ResultsArray[NUM_ANALOG_PINS];

static uint32_t joystickX;
static uint32_t joystickY;
static uint32_t boatSelectPotVal;
static uint32_t imuX;
static uint32_t imuY;
static uint32_t imuZ;

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
   InitADService

 Description
   Initializes the AD service.
****************************************************************************/
bool InitADService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;

  initAD();

  ES_Timer_InitTimer(AD_TIMER, AD_TIMER_PERIOD);

  ThisEvent.EventType = ES_INIT;

  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
   PostADService

 Description
   Posts events to this service.
****************************************************************************/
bool PostADService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
   RunADService

 Description
   Periodically reads the latest ADC scan results and stores them in named
   module-level variables.
****************************************************************************/
ES_Event_t RunADService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

  if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == AD_TIMER))
  {
    ADC_MultiRead(ResultsArray);

    // ADC_MultiRead returns values in increasing analog channel order:
    // AN0, AN1, AN4, AN5, AN11, AN12
    joystickX        = ResultsArray[0];   // AN0
    joystickY        = ResultsArray[1];   // AN1
    boatSelectPotVal = ResultsArray[2];   // AN4
    imuX             = ResultsArray[3];   // AN5
    imuZ             = ResultsArray[4];   // AN11
    imuY             = ResultsArray[5];   // AN12

    // Optional debug print
//     DB_printf("Joystick X: %u\r\n", joystickX);
//     DB_printf("Joystick Y: %u\r\n", joystickY);
//     DB_printf("Boat Select: %u\r\n", boatSelectPotVal);
//     DB_printf("IMU X: %u\r\n", imuX);
//     DB_printf("IMU Y: %u\r\n", imuY);
//     DB_printf("IMU Z: %u\r\n", imuZ);

    ES_Timer_InitTimer(AD_TIMER, AD_TIMER_PERIOD);
  }

  return ReturnEvent;
}

/****************************************************************************
 Getter Functions

 Description
   Return the most recently stored analog values.
****************************************************************************/

uint32_t getIMUX(void)
{
  return imuX;
}

uint32_t getIMUY(void)
{
  return imuY;
}

uint32_t getIMUZ(void)
{
  return imuZ;
}

uint32_t getJoystickX(void)
{
  return joystickX;
}

uint32_t getJoystickY(void)
{
  return joystickY;
}

uint32_t getBoatSelectVal(void)
{
  return boatSelectPotVal;
}

uint8_t ADC10ToByte(uint32_t adcVal)
{
    uint8_t value;

    if (adcVal > 1023) {
        adcVal = 1023;
    }

    value = (uint8_t)(adcVal >> 2);

    if (value == 0x7E) {
        value = 0x7F;
    }

    return value;
}

/***************************************************************************
 Private Functions
 ***************************************************************************/

static void initAD(void)
{
  // Set pins to analog mode
  ANSELAbits.ANSA0 = 1;    // AN0, joystick X
  ANSELAbits.ANSA1 = 1;    // AN1, joystick Y
  ANSELBbits.ANSB2 = 1;    // AN4, boat select
  ANSELBbits.ANSB3 = 1;    // AN5, IMU X
  ANSELBbits.ANSB13 = 1;   // AN11, IMU Z
  ANSELBbits.ANSB12 = 1;   // AN12, IMU Y

  // Set pins as inputs
  TRISAbits.TRISA0 = 1;    // AN0, joystick X
  TRISAbits.TRISA1 = 1;    // AN1, joystick Y
  TRISBbits.TRISB2 = 1;    // AN4, boat select
  TRISBbits.TRISB3 = 1;    // AN5, IMU X
  TRISBbits.TRISB13 = 1;   // AN11, IMU Z
  TRISBbits.TRISB12 = 1;   // AN12, IMU Y

  // Configure ADC auto-scan
  if (!ADC_ConfigAutoScan(ANALOG_PINS))
  {
    DB_printf("ERROR: ADC_ConfigAutoScan failed\r\n");
  }
}



/*------------------------------ End of file ------------------------------*/
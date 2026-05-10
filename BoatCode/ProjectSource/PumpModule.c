/****************************************************************************
 Module
   PumpModule.c

 Description
   Module for controlling the on board Pump (i.e our cannon)

 Author
   Daniel Rullan
****************************************************************************/

#include "PumpModule.h"
#include <xc.h>
#include <stdbool.h>

/*----------------------------- Module Defines ----------------------------*/
//#define PUMP_ANSEL  ANSELBbits.ANSB11
#define PUMP_TRIS   TRISBbits.TRISB11
#define PUMP_LAT    LATBbits.LATB11

/*----------------------------- Module Variables ----------------------------*/
static bool PumpState;

/****************************************************************************
 Function
   Pump_Init

 Description
   Initialize pumps MOSFET control pin 
****************************************************************************/
void Pump_Init(void)
{
  // Set up pin to be digital output
    //PUMP_ANSEL = 0;
    PUMP_LAT = 0;      // set output low before enabling output
    PUMP_TRIS = 0;     // output

    PumpState = false;
}


/****************************************************************************
 Function
   Pump_On

 Description
   Turns pump MOSFET on.
****************************************************************************/
void Pump_On(void)
{
    PUMP_LAT = 1;
    PumpState = true;
}


/****************************************************************************
 Function
   Pump_Off

 Description
   Turns pump MOSFET off.
****************************************************************************/
void Pump_Off(void)
{
    PUMP_LAT = 0;
    PumpState = false;
}


/****************************************************************************
 Function
   Pump_IsOn

 Description
   Returns current software pump state.
****************************************************************************/
bool Pump_IsOn(void)
{
    return PumpState;
}
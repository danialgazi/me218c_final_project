/****************************************************************************
 Module
   DigitalInputService.c

 Description
   Digital input service for controller buttons and switches.

   Handles:
   - Shoot button on RB11
   - Refuel switch on RB12
   - Pair button on RB13

   Raw CN interrupt starts debounce.
   Debounce timer confirms stable state and posts clean events.
****************************************************************************/

#include <xc.h>
#include <sys/attribs.h>
#include <stdint.h>
#include <stdbool.h>

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "DigitalInputService.h"

/*----------------------------- Module Defines ----------------------------*/

// Pin assignments
#define REFUEL_PIN          PORTBbits.RB9
#define PAIR_PIN            PORTBbits.RB10
#define SHOOT_PIN           PORTBbits.RB11

// Active-high logic
#define INPUT_ACTIVE        1

// Debounce time in ms
#define DEBOUNCE_TIME_MS    20


/*---------------------------- Module Variables ---------------------------*/

static uint8_t MyPriority;

// Last confirmed stable states
static uint8_t LastStableShoot;
static uint8_t LastStableRefuel;
static uint8_t LastStablePair;

// Current held/on flags, useful for packet building
static bool ShootHeld = false;
static bool RefuelOn = false;
static bool PairHeld = false;

// Used so multiple CN interrupts during debounce do not restart everything
static bool Debouncing = false;

/*---------------------------- Private Function Prototypes ----------------*/

static void DigitalInput_HWInit(void);
static void DigitalInput_ReadInitialStates(void);
static void DigitalInput_PostIfChanged(void);

static void DigitalInput_EnableCNInterrupts(void);
static void DigitalInput_DisableCNInterrupts(void);
static void DigitalInput_ClearCNFlags(void);

static void PostCleanEvent(ES_EventType_t EventType);

/*------------------------------ Module Code ------------------------------*/

bool InitDigitalInputService(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;

    DigitalInput_HWInit();
    DigitalInput_ReadInitialStates();

    ThisEvent.EventType = ES_INIT;
    ThisEvent.EventParam = 0;

    return ES_PostToService(MyPriority, ThisEvent);
}

bool PostDigitalInputService(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunDigitalInputService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;
    ReturnEvent.EventParam = 0;

    switch (ThisEvent.EventType)
    {
        case ES_INIT:
        {
            // Nothing else needed here for now.
            break;
        }

        case ES_TIMEOUT:
        {
            if (ThisEvent.EventParam == DIGITAL_INPUT_DEBOUNCE_TIMER)
            {
                /*
                   Debounce period is over.
                   Now read pins once and post events only if stable value changed.
                */
                DigitalInput_PostIfChanged();

                Debouncing = false;

                DigitalInput_ClearCNFlags();
                DigitalInput_EnableCNInterrupts();
            }
            break;
        }

        default:
        {
            break;
        }
    }

    return ReturnEvent;
}

/***************************************************************************
 Public helper functions
 ***************************************************************************/

bool DigitalInput_IsShootHeld(void)
{
    return ShootHeld;
}

bool DigitalInput_IsPairHeld(void)
{
    return PairHeld;
}

bool DigitalInput_IsRefuelOn(void)
{
    return RefuelOn;
}

/***************************************************************************
 Private functions
 ***************************************************************************/

static void DigitalInput_HWInit(void)
{
    // Shoot button: RB11
    //ANSELBbits.ANSB11 = 0;
    TRISBbits.TRISB11 = 1;

    // Refuel switch: RB9
    //ANSELBbits.ANSB9 = 0;
    TRISBbits.TRISB9 = 1;

    // Pair button: RB10
    //ANSELBbits.ANSB10 = 0;
    TRISBbits.TRISB10 = 1;

    // Set up Interrupt stuff
    CNCONBbits.ON = 1;      // Turn Change Notice Interrupts on
    
    CNENBbits.CNIEB9 = 1; 
    CNENBbits.CNIEB10 = 1;
    CNENBbits.CNIEB11 = 1;

    DigitalInput_ClearCNFlags();

    // Configure CN interrupt priority/subpriority
    IPC8bits.CNIP = 3;   // priority 3
    IPC8bits.CNIS = 0;   // subpriority 0

    DigitalInput_EnableCNInterrupts();
}

static void DigitalInput_ReadInitialStates(void)
{
    LastStableShoot = SHOOT_PIN;
    LastStableRefuel = REFUEL_PIN;
    LastStablePair = PAIR_PIN;

    ShootHeld = (LastStableShoot == INPUT_ACTIVE);
    RefuelOn = (LastStableRefuel == INPUT_ACTIVE);
    PairHeld = (LastStablePair == INPUT_ACTIVE);
}

static void DigitalInput_PostIfChanged(void)
{
    uint8_t CurrentShoot = SHOOT_PIN;
    uint8_t CurrentRefuel = REFUEL_PIN;
    uint8_t CurrentPair = PAIR_PIN;

    // Shoot button changed
    if (CurrentShoot != LastStableShoot)
    {
        LastStableShoot = CurrentShoot;

        if (CurrentShoot == INPUT_ACTIVE)
        {
            ShootHeld = true;
            PostCleanEvent(ES_SHOOT_BUTTON_PRESSED);
        }
        else
        {
            ShootHeld = false;
            PostCleanEvent(ES_SHOOT_BUTTON_RELEASED);
        }
    }

    // Refuel switch changed
    if (CurrentRefuel != LastStableRefuel)
    {
        LastStableRefuel = CurrentRefuel;

        if (CurrentRefuel == INPUT_ACTIVE)
        {
            RefuelOn = true;
            PostCleanEvent(ES_REFUEL_SW_ON);
        }
        else
        {
            RefuelOn = false;
            PostCleanEvent(ES_REFUEL_SW_OFF);
        }
    }

    // Pair button changed
    if (CurrentPair != LastStablePair)
    {
        LastStablePair = CurrentPair;

        if (CurrentPair == INPUT_ACTIVE)
        {
            PairHeld = true;
            PostCleanEvent(ES_PAIR_BUTTON_PRESSED);
        }
        else
        {
            PairHeld = false;
            PostCleanEvent(ES_PAIR_BUTTON_RELEASED);
        }
    }
}

static void PostCleanEvent(ES_EventType_t EventType)
{
    // Create event
    ES_Event_t NewEvent;
    NewEvent.EventType = EventType;
    NewEvent.EventParam = 0;

    // Post Event
    ES_PostAll(NewEvent);
}

static void DigitalInput_EnableCNInterrupts(void)
{
    DigitalInput_ClearCNFlags();
    IEC1bits.CNBIE = 1;   // Enable Change Notice interrupt for Port B
}

static void DigitalInput_DisableCNInterrupts(void)
{
    IEC1bits.CNBIE = 0;   // Disable Change Notice interrupt for Port B
}

static void DigitalInput_ClearCNFlags(void)
{
    volatile uint32_t dummyRead = PORTB;
    (void)dummyRead;

    IFS1bits.CNBIF = 0;   // Clear Change Notice interrupt flag for Port B
}

/***************************************************************************
 Interrupt Service Routine
 ***************************************************************************/

/*
   Change Notice ISR.

   This ISR does NOT post button events directly.
   It only starts debounce.
*/
void __ISR(_CHANGE_NOTICE_VECTOR, IPL3SOFT) ChangeNoticeISR(void)
{
    DigitalInput_DisableCNInterrupts();
    DigitalInput_ClearCNFlags();

    if (!Debouncing)
    {
        Debouncing = true;
        ES_Timer_InitTimer(DIGITAL_INPUT_DEBOUNCE_TIMER, DEBOUNCE_TIME_MS);
    }
}

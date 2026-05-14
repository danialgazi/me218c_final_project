/****************************************************************************
 Module
   TemplateFSM.c

 Revision
   1.0.1

 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BoatSM.h"
#include "BoatCom.h"
#include "ThrusterControl.h"
#include "ES_Timers.h"

/*----------------------------- Module Defines ----------------------------*/
#define TEST_QUACKRAFT_ADDRESS     0x2084
#define TEST_MALLARD_ADDRESS       0x2184
#define TEST_CHARGE_BYTE           0x55
#define PAIRING_ACKNOWLEDGE_BYTE 0xFF
#define UNPAIRING_TIMEOUT 4000 // ms to wait before unpairing due to inactivity (4 seconds)
#define FULL_CHARGE 100 // set it as defined as 100 for now
#define MIN_MESSAGES_TO_FULL_CHARGE 25 // this is the number of charge commands we need to get to full charge
#define CHARGE_INCREMENT (FULL_CHARGE / MIN_MESSAGES_TO_FULL_CHARGE) // how much to increase charge status on each charge command received
#define MAX_CHARGE_TIME 40000 // 40 seconds - max time of charge if we are running the motors
#define THROTTLE_THRESHOLD 10 // the amount we need to deviate from center to count as "throttle applied"

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

static bool isThrottleApplied(uint8_t joy1, uint8_t joy2);
static void resetChargeAccounting(void);
static uint8_t decreaseChargeStatus(uint8_t currentCharge, uint8_t joy1, uint8_t joy2);
static uint8_t increaseChargeStatus(uint8_t currentCharge);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static BoatState_t CurrentState;
static uint8_t chargeStatus = 0;
static uint16_t pairedControllerAddress = 0;
static uint16_t lastDriveTime = 0;
static uint16_t leftoverMs = 0;
static bool firstChargeCall = true;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitBoatFSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitBoatFSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState;
  // post the initial transition event
  // hardware inits
    BoatCom_InitUART();
    ThrusterControl_Init();
    ThrusterControl_SetThrust(THRUSTER_CONTROL_JOY_CENTER, THRUSTER_CONTROL_JOY_CENTER);

//    ANSELBbits.ANSELB10 = 0; // set RB10 to digital for duck shooter
    TRISBbits.TRISB10 = 0; // set RB10 to output
    LATBbits.LATB10 = 0; // initialize RB10 to low
  
    DB_printf("\r\nBoatSM initialized\r\n");
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
     PostTemplateFSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostBoatFSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunBoatFSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunBoatFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState:
    {
      if (ThisEvent.EventType == ES_INIT)
      {
        DB_printf("BoatSM: InitPState\r\n");
        CurrentState = Unpaired;
      }
    }
    break;

    case Unpaired:
    {
      switch (ThisEvent.EventType)
      {
        case ES_PAIRING_COMMAND:
        {
          // Pair with the controller, send ACK, and start timeout
          CurrentState = Driving;
          pairedControllerAddress = ThisEvent.EventParam;

          DB_printf("\r\nPaired with controller %d\r\n", pairedControllerAddress);

          BoatCom_SendAck(pairedControllerAddress, PAIRING_ACKNOWLEDGE_BYTE);
          ES_Timer_InitTimer(BOAT_SM_TIMER, UNPAIRING_TIMEOUT);

          chargeStatus = FULL_CHARGE; // reset charge status to full on new pairing
          resetChargeAccounting();
        }
        break;

        default:
          break;
      }
    }
    break;

    case Driving:
    {
      switch (ThisEvent.EventType)
      {
        case ES_DRIVING_COMMAND:
        {
          BoatCom_Command_t latestCommand = BoatCom_GetLatestCommand();

          // Reset unpairing timer on every drive command received
          ES_Timer_InitTimer(BOAT_SM_TIMER, UNPAIRING_TIMEOUT);

          if (chargeStatus != 0)
          {
            // Send command to thrusters
            ThrusterControl_SetThrust(latestCommand.joy2Byte,
                                      latestCommand.joy1Byte);

            if ((latestCommand.digiByte & 1) == 1)
            {
              DB_printf("Duck shooter activated!\r\n");
              LATBbits.LATB10 = 1;
            }
            else
            {
              DB_printf("Duck shooter not activated\r\n");
              LATBbits.LATB10 = 0;
            }

            chargeStatus = decreaseChargeStatus(chargeStatus,
                                                latestCommand.joy1Byte,
                                                latestCommand.joy2Byte);

            BoatCom_SendAck(pairedControllerAddress, chargeStatus);
          }
          else
          {
            // Out of fuel, ignore drive command and idle thrusters
            ThrusterControl_SetThrust(THRUSTER_CONTROL_JOY_CENTER,
                                      THRUSTER_CONTROL_JOY_CENTER);

            LATBbits.LATB10 = 0;

            BoatCom_SendAck(pairedControllerAddress, chargeStatus);
          }
        }
        break;

        case ES_CHARGING_COMMAND:
        {
          // Reset unpairing timer on every charge command received
          ES_Timer_InitTimer(BOAT_SM_TIMER, UNPAIRING_TIMEOUT);

          // Set thrusters to idle while charging
          ThrusterControl_SetThrust(THRUSTER_CONTROL_JOY_CENTER,
                                    THRUSTER_CONTROL_JOY_CENTER);

          LATBbits.LATB10 = 0;

          chargeStatus = increaseChargeStatus(chargeStatus);

          BoatCom_SendAck(pairedControllerAddress, chargeStatus);
        }
        break;

        case ES_PAIRING_COMMAND:
        {
          if (ThisEvent.EventParam == pairedControllerAddress)
          {
            DB_printf("\r\nReceived pairing command from currently paired controller, resetting unpairing timer\r\n");

            ES_Timer_InitTimer(BOAT_SM_TIMER, UNPAIRING_TIMEOUT);
            BoatCom_SendAck(pairedControllerAddress, PAIRING_ACKNOWLEDGE_BYTE);
          }
          else
          {
            DB_printf("\r\nReceived pairing command from different controller, ignoring\r\n");
          }
        }
        break;

        case ES_TIMEOUT:
        {
          if (ThisEvent.EventParam == BOAT_SM_TIMER)
          {
            ThrusterControl_SetThrust(THRUSTER_CONTROL_JOY_CENTER,
                                      THRUSTER_CONTROL_JOY_CENTER);

            LATBbits.LATB10 = 0;

            pairedControllerAddress = 0;
            CurrentState = Unpaired;

            DB_printf("\r\nBoatSM timeout: unpaired\r\n");
          }
        }
        break;

        default:
          break;
      }
    }
    break;

    default:
      break;
  }

  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
****************************************************************************/
BoatState_t QueryBoatFSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

 static bool isThrottleApplied(uint8_t joy1, uint8_t joy2)
{
  if ((joy1 > THRUSTER_CONTROL_JOY_CENTER + THROTTLE_THRESHOLD) ||
      (joy1 < THRUSTER_CONTROL_JOY_CENTER - THROTTLE_THRESHOLD) ||
      (joy2 > THRUSTER_CONTROL_JOY_CENTER + THROTTLE_THRESHOLD) ||
      (joy2 < THRUSTER_CONTROL_JOY_CENTER - THROTTLE_THRESHOLD))
  {
    return true;
  }
  else
  {
    return false;
  }
}

static uint8_t decreaseChargeStatus(uint8_t currentCharge, uint8_t joy1, uint8_t joy2)
{
  uint16_t currentTime = ES_Timer_GetTime();

  if (firstChargeCall)
  {
    lastDriveTime = currentTime;
    firstChargeCall = false;
    return currentCharge;
  }

  uint16_t elapsedMs = currentTime - lastDriveTime;
  lastDriveTime = currentTime;

  if (!isThrottleApplied(joy1, joy2))
  {
    return currentCharge;
  }

  uint16_t msPerChargePoint = MAX_CHARGE_TIME / FULL_CHARGE;

  uint16_t activeMs = leftoverMs + elapsedMs;
  uint8_t chargeDecrease = activeMs / msPerChargePoint;

  leftoverMs = activeMs % msPerChargePoint;

  if (chargeDecrease >= currentCharge)
  {
    return 0;
  }
  else
  {
    return currentCharge - chargeDecrease;
  }
}

static uint8_t increaseChargeStatus(uint8_t currentCharge)
{
  if (currentCharge + CHARGE_INCREMENT >= FULL_CHARGE) {
    return FULL_CHARGE;
  } else {
    return currentCharge + CHARGE_INCREMENT;
  }
}

static void resetChargeAccounting(void)
{
  lastDriveTime = ES_Timer_GetTime();
  leftoverMs = 0;
  firstChargeCall = true;
}

/****************************************************************************
 Module
   ControllerFSM.c

 Description
   Flat FSM for controller boat selection, pairing, driving, and refueling.

 Notes
   Controller packet, pairing, ack timeout, and refuel timers are routed to
   PostControllerFSM in ES_Configure.
****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ControllerFSM.h"
#include "ControllerCom.h"
#include "ADService.h"
#include "IMUModule.h"
#include "NeopixelModule.h"
#include "DigitalInputService.h"

/*----------------------------- Module Defines ----------------------------*/
// Timing and other constants for controller behavior
#define CONTROLLER_PACKET_PERIOD_MS      200u   //  Send packets at 5 Hz, 200 ms period
#define CONTROLLER_PAIR_RETRY_MS         200u   // Send pairing requests at 5 Hz, 200 ms period
#define CONTROLLER_ACK_TIMEOUT_MS        4000u  // Wait 4 seconds for an ack before assuming pairing failed or connection lost and returning to boat select
#define CONTROLLER_REFUEL_SAMPLE_MS      50u    // Sample refuel status every 50 ms
#define CONTROLLER_REFUEL_DURATION_MS    5000u 

// Team and boat selection constants
#define BOAT_SELECT_ADC_MAX              1023u
#define FUEL_FULL_PERCENT                100u

// Digital input bit masks for drive packet
#define SHOOT_DIGI_MASK                  CONTROLLER_COM_BUTTON_SMACK

/*---------------------------- Module Functions ---------------------------*/
static void ResetControllerData(void);
static uint8_t ReadSelectedTeamIndex(void);
static uint16_t GetSelectedQuackraftAddress(void);
static uint16_t GetMyMallardAddress(void);
static void SendPairingRequest(void);
static void SendDrivePacket(void);
static void SendRefuelPacket(void);
static void RestartAckTimer(void);
static void StartDrivingTimers(void);
static void StopDrivingTimers(void);
static void StartRefuelTimers(void);
static void StopRefuelTimers(void);
static void RestartPairingTimer(void);
static void StopPairingTimer(void);
static void HandleBoatAck(ES_Event_t ThisEvent);
static void StartContinuousShake(void);
static void StopContinuousShake(void);
static void UpdateLinearRefuel(void);
static void SampleForContinuousShake(void);
static void EnterBoatSelect(void);
static void EnterPairing(void);
static void EnterDriving(void);
static void EnterRefuel(void);

/*---------------------------- Module Variables ---------------------------*/
static ControllerState_t CurrentState;
static uint8_t MyPriority;

static uint8_t SelectedTeamIndex;
static uint16_t QuackraftAddress;
static uint16_t MallardAddress;

// State variables for boat pairing and control
static bool IsPaired;
static bool ShootButtonPressed;
static bool PairButtonPressed;
static bool RefuelSwitchOn;
static bool IsShakingContinuously;
static bool SawShakeThisSample;

// State variables for refueling logic
static uint8_t FuelPercent;
static uint16_t ContinuousShakeMs;

/*------------------------------ Module Code ------------------------------*/

bool InitControllerFSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  CurrentState = ControllerInitPState;
  NeopixelModule_Init();

  ThisEvent.EventType = ES_INIT;
  return ES_PostToService(MyPriority, ThisEvent);
}

bool PostControllerFSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunControllerFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

  switch (CurrentState)
  {
    case ControllerInitPState:
    {
      if (ThisEvent.EventType == ES_INIT)
      {
        // Perform any initialization needed for this state machine
        ResetControllerData();
        EnterBoatSelect();
      }
    }
    break;

    case BoatSelect:
    {
      switch (ThisEvent.EventType)
      {
        case ES_PAIR_BUTTON_PRESSED:
        {
          // Start pairing process
          PairButtonPressed = true;
          // Read boat selection and addresses, send initial pairing request, and start timers
          SelectedTeamIndex = ReadSelectedTeamIndex();
          QuackraftAddress = GetSelectedQuackraftAddress();
          MallardAddress = GetMyMallardAddress();
          SendPairingRequest();
          RestartPairingTimer();
          RestartAckTimer();
          EnterPairing();
        }
        break;

        case ES_PAIR_BUTTON_RELEASED:
          // Reset pairing state variable
          PairButtonPressed = false;
          break;

        default:
          break;
      }
    }
    break;

    case Pairing:
    {
      switch (ThisEvent.EventType)
      {
        case ES_BOAT_ACK:
        {
          // Handle ack to check for pairing success and transition to driving if successful
          HandleBoatAck(ThisEvent);
          if (IsPaired)   // Succesful            
          {
            // Stop pairing timers and start driving timers
            StopPairingTimer();
            EnterDriving();
          }
        }
        break;

        case ES_PAIR_BUTTON_RELEASED:
          // Reset pairing state variable
          PairButtonPressed = false;
          break;
        
        case ES_TIMEOUT:
        {
          // Check resending pairing request on pairing timer timeout
          if (ThisEvent.EventParam == CONTROLLER_PAIR_TIMER)
          {
            SendPairingRequest();
            RestartPairingTimer();
          }
          // Return to boat select on ack timer timeout
          else if (ThisEvent.EventParam == CONTROLLER_ACK_TIMER)
          {
            // Assume pairing failed or connection lost. bascially reset
            StopPairingTimer();
            ES_Timer_StopTimer(CONTROLLER_ACK_TIMER);
            EnterBoatSelect();
          }
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
        // Note: Boat selection is currently disabled during driving to prevent accidental deselection.
        // case ES_PAIR_BUTTON_PRESSED:
        // {
        //   PairButtonPressed = true;
        //   IsPaired = false;
        //   StopDrivingTimers();
        //   ControllerCom_SendIdle(QuackraftAddress);
        //   CurrentState = BoatSelect;
        // }
        // break;

        case ES_PAIR_BUTTON_RELEASED:
          PairButtonPressed = false;
          break;

        case ES_SHOOT_BUTTON_PRESSED:
        // Set shoot button state variable to be included in drive packets
          ShootButtonPressed = true;
          break;

        case ES_SHOOT_BUTTON_RELEASED:
        // Reset shoot button state variable to be included in drive packets
          ShootButtonPressed = false;
          break;

        case ES_REFUEL_SW_ON:
        {
          // Transition to refuel state on refuel switch activation
          RefuelSwitchOn = true;
          // Reset timers
          StopDrivingTimers();
          EnterRefuel();
        }
        break;

        case ES_BOAT_ACK:
          // Handle acks to check for fuel updates and connection status
          HandleBoatAck(ThisEvent);
          break;

        case ES_TIMEOUT:
        {
          // Check if packet timer expired to send next drive packet
          if (ThisEvent.EventParam == CONTROLLER_PACKET_TIMER)
          {
            // Send drive packet and restart timer
            SendDrivePacket();
            ES_Timer_InitTimer(CONTROLLER_PACKET_TIMER,
                               CONTROLLER_PACKET_PERIOD_MS);
          }
          // Check if connection lost due to ack timeout
          else if (ThisEvent.EventParam == CONTROLLER_ACK_TIMER)
          {
            // Assume connection lost. basically reset to boat select
            IsPaired = false;
            StopDrivingTimers();
            EnterBoatSelect();
          }
        }
        break;

        default:
          break;
      }
    }
    break;

    case Refuel:
    {
      switch (ThisEvent.EventType)
      {
        // Refuel switch can be turned off to return to driving state
        case ES_REFUEL_SW_OFF:
        {
          // Transition back to driving state on refuel switch deactivation
          RefuelSwitchOn = false;
          StopRefuelTimers();
          EnterDriving();
        }
        break;

        case ES_IMU_SHAKE_DETECTED:
        // Start or continue continuous shaking to refuel
          StartContinuousShake();
          SawShakeThisSample = true;
          break;

        case ES_BOAT_ACK:
        // Handle acks to check for fuel updates and connection status
          HandleBoatAck(ThisEvent);
          break;

        case ES_TIMEOUT:
        {
          // Check if packet timer expired to send next refuel packet
          if (ThisEvent.EventParam == CONTROLLER_PACKET_TIMER)
          {
            // Send refuel packet and restart timer
            SendRefuelPacket();
            ES_Timer_InitTimer(CONTROLLER_PACKET_TIMER,
                               CONTROLLER_PACKET_PERIOD_MS);
          }
          else if (ThisEvent.EventParam == CONTROLLER_REFUEL_TIMER)
          {
            // Sample for continuous shake to update refuel status and fuel percent
            SampleForContinuousShake();
            ES_Timer_InitTimer(CONTROLLER_REFUEL_TIMER,
                               CONTROLLER_REFUEL_SAMPLE_MS);
          }
          else if (ThisEvent.EventParam == CONTROLLER_ACK_TIMER)
          {
            // Assume connection lost. basically reset to boat select
            IsPaired = false;
            RefuelSwitchOn = false;
            StopRefuelTimers();
            EnterBoatSelect();
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

ControllerState_t QueryControllerFSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

 /* Reset all controller data to initial values */
static void ResetControllerData(void)
{
  SelectedTeamIndex = 0;
  // Set addresses based on initial team index
  QuackraftAddress = QuackraftAddresses[SelectedTeamIndex];
  MallardAddress = MallardAddresses[MY_TEAM_INDEX];
  // Reset state variables
  IsPaired = false;
  ShootButtonPressed = false;
  PairButtonPressed = false;
  RefuelSwitchOn = false;
  IsShakingContinuously = false;
  SawShakeThisSample = false;
  // Start with full fuel on reset
  FuelPercent = FUEL_FULL_PERCENT;
  ContinuousShakeMs = 0;
}

/* Read the selected team index from the potentiometer */
static uint8_t ReadSelectedTeamIndex(void)
{
  // Read potentiometer value 
  uint32_t potVal = getBoatSelectVal();
  uint32_t scaledIndex;

  // Clip potentiometer value to max
  if (potVal > BOAT_SELECT_ADC_MAX)
  {
    potVal = BOAT_SELECT_ADC_MAX;
  }

  // Scale potentiometer value to range of team 
  scaledIndex = (potVal * CONTROLLER_COM_NUM_TEAMS) /
                (BOAT_SELECT_ADC_MAX + 1u);

  // Clip scaled index to valid range just in case I messed up lol
  if (scaledIndex >= CONTROLLER_COM_NUM_TEAMS)
  {
    scaledIndex = CONTROLLER_COM_NUM_TEAMS - 1u;
  }

  return (uint8_t)scaledIndex;
}

/* Get the address of the selected Quackraft */
static uint16_t GetSelectedQuackraftAddress(void)
{
  return QuackraftAddresses[SelectedTeamIndex];
}

/* Get the address of the my Mallard */
static uint16_t GetMyMallardAddress(void)
{
  return MallardAddresses[MY_TEAM_INDEX];
}

/* Send a pairing request to the selected Quackraft */
static void SendPairingRequest(void)
{
  ControllerCom_SendPairing(QuackraftAddress, MallardAddress);
}

/* Send a drive packet to the selected Quackraft */
static void SendDrivePacket(void)
{
  // digital byte starts at 0 and sets bits corresponding to which buttons are pressed
  uint8_t digiByte = 0x00;

  if (ShootButtonPressed)
  {
    // Set shoot bit in digital byte if shoot button is pressed
    digiByte |= SHOOT_DIGI_MASK;
  }

  // Send drive packet with joystick values and digital byte
  ControllerCom_SendDriving(QuackraftAddress,
                            ADC10ToByte(getJoystickY()),
                            ADC10ToByte(getJoystickX()),
                            digiByte);
}

/* Send a refuel packet to the selected Quackraft */
static void SendRefuelPacket(void)
{
  ControllerCom_SendCharging(QuackraftAddress);
}

/* Restart the boat acknowledgment timer */
static void RestartAckTimer(void)
{
  ES_Timer_InitTimer(CONTROLLER_ACK_TIMER, CONTROLLER_ACK_TIMEOUT_MS);
}

/* Start the driving timers */
static void StartDrivingTimers(void)
{
  // REstart ACk Timer to give ful period in driving state before assuming connection was lost
  RestartAckTimer();
  ES_Timer_InitTimer(CONTROLLER_PACKET_TIMER, CONTROLLER_PACKET_PERIOD_MS);
}

/* Stop the driving timers */
static void StopDrivingTimers(void)
{
  ES_Timer_StopTimer(CONTROLLER_PACKET_TIMER);
  ES_Timer_StopTimer(CONTROLLER_ACK_TIMER);
}

/* Start the refuel timers */
static void StartRefuelTimers(void)
{
  ContinuousShakeMs = 0;
  IsShakingContinuously = false;
  // REstart ACk Timer to give ful period in refuel state before assuming connection was lost
  RestartAckTimer();
  ES_Timer_InitTimer(CONTROLLER_PACKET_TIMER, CONTROLLER_PACKET_PERIOD_MS);
  // Refuel timer is used to sample for continuous shaking and update fuel percent accordingly
  ES_Timer_InitTimer(CONTROLLER_REFUEL_TIMER, CONTROLLER_REFUEL_SAMPLE_MS);
}

/* Stop the refuel timers */
static void StopRefuelTimers(void)
{
  ES_Timer_StopTimer(CONTROLLER_PACKET_TIMER);
  ES_Timer_StopTimer(CONTROLLER_REFUEL_TIMER);
  ES_Timer_StopTimer(CONTROLLER_ACK_TIMER);
  StopContinuousShake();
}

/* Restart the pairing timer */
static void RestartPairingTimer(void)
{
  ES_Timer_InitTimer(CONTROLLER_PAIR_TIMER, CONTROLLER_PAIR_RETRY_MS);
}

/* Stop the pairing timer */
static void StopPairingTimer(void)
{
  ES_Timer_StopTimer(CONTROLLER_PAIR_TIMER);
}

/* Handle acknowledgment from the selected Quackraft */
static void HandleBoatAck(ES_Event_t ThisEvent)
{
  // Check for pairing success
  if (ThisEvent.EventParam == CONTROLLER_COM_PAIRING_SUCCESS)
  {
    // 0xFF means pairing success, any other value is fuel percentage
    IsPaired = true;
  }
  else if (ThisEvent.EventParam <= FUEL_FULL_PERCENT) // Fuel percentage update
  {
    // Fuel Percent is included in EventParam of ack events, so update fuel percent on each ack
    FuelPercent = (uint8_t)ThisEvent.EventParam;
  }

  // Restart ack timer on every ack received to monitor connection status
  RestartAckTimer();
}

/* Start continuous shaking */
static void StartContinuousShake(void)
{
  // Variable used to track if controller is being shaken continuously for refueling. Only update fuel percent if shaking continuously, not just a single shake.
  IsShakingContinuously = true;
}

/* Stop continuous shaking */
static void StopContinuousShake(void)
{
  // Reset continuous shake tracking variables
  IsShakingContinuously = false;
  SawShakeThisSample = false;
  ContinuousShakeMs = 0;
}

/* Update fuel percentage based on continuous shaking */
static void UpdateLinearRefuel(void)
{
  uint32_t nextFuel;

  // Clip fuel percent if already at or above full
  if (FuelPercent >= FUEL_FULL_PERCENT)
  {
    FuelPercent = FUEL_FULL_PERCENT;
    return;
  }

  /* Increase continuous shake time and calculate next fuel percent based on 
  linear refuel rate. Continuous shake time is tracked in sample intervals 
  defined by CONTROLLER_REFUEL_SAMPLE_MS.*/
  ContinuousShakeMs += CONTROLLER_REFUEL_SAMPLE_MS;
  if (ContinuousShakeMs > CONTROLLER_REFUEL_DURATION_MS)
  {
    // Cap continuous shake time at refuel duration to cap fuel percent at 100%
    ContinuousShakeMs = CONTROLLER_REFUEL_DURATION_MS;
  }

  // Calculate next fuel percent based on linear refuel rate and cap at 100%
  nextFuel = ((uint32_t)ContinuousShakeMs * FUEL_FULL_PERCENT) /
             CONTROLLER_REFUEL_DURATION_MS;

  // Again check if nextFuel is at or above Full, clip at full
  if (nextFuel > FUEL_FULL_PERCENT)
  {
    nextFuel = FUEL_FULL_PERCENT;
  }

  // Update fuel percent  
  FuelPercent = (uint8_t)nextFuel;
}

/* Sample for continuous shake, does this by checking if a shake was detected 
since the last sample. If shaking continuously, update fuel percent based on 
linear refuel rate. If not shaking continuously, reset refuel status. 
This function should be called on a timer in the refuel state at a rate 
defined by CONTROLLER_REFUEL_SAMPLE_MS. */
static void SampleForContinuousShake(void)
{
  if (SawShakeThisSample)
  {
    IsShakingContinuously = true;
    UpdateLinearRefuel();
  }
  else
  {
    StopContinuousShake();
  }

  SawShakeThisSample = false;

  IMUModule_Check4Shake();
}

// Helpers for transitioning between states, also sets neopixel pattern for each state
static void EnterBoatSelect(void)
{
  CurrentState = BoatSelect;
  LEDS_Unpaired();
}

static void EnterPairing(void)
{
  CurrentState = Pairing;
  LEDS_Pairing();
}

static void EnterDriving(void)
{
  StartDrivingTimers();
  CurrentState = Driving;
  LEDS_Driving();
}

static void EnterRefuel(void)
{
  StartRefuelTimers();
  CurrentState = Refuel;
  LEDS_Refueling();
}

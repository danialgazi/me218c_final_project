/****************************************************************************
 Module
   CommsService.c
****************************************************************************/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "CommsService.h"
#include "ControllerCom.h"

/*----------------------------- Module Defines ----------------------------*/

#define DEFAULT_TEAM_INDEX   0

/*---------------------------- Module Variables ---------------------------*/

static uint8_t MyPriority;

static uint8_t CurrentTeamIndex = DEFAULT_TEAM_INDEX;
static uint8_t CurrentJoy1 = CONTROLLER_COM_JOY_CENTER;
static uint8_t CurrentJoy2 = CONTROLLER_COM_JOY_CENTER;
static uint8_t CurrentDigi = 0x00;

/*---------------------------- Private Functions --------------------------*/

static void SendCurrentDriveCommand(void);
static void SetIdleCommand(void);

/*------------------------------ Module Code ------------------------------*/

bool InitCommsService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;

  ControllerCom_InitUART();

  CurrentTeamIndex = DEFAULT_TEAM_INDEX;
  SetIdleCommand();

  ThisEvent.EventType = ES_INIT;
  return ES_PostToService(MyPriority, ThisEvent);
}


bool PostCommsService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}


ES_Event_t RunCommsService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

  switch (ThisEvent.EventType) {
    case ES_INIT:
      ControllerCom_SendIdle(QuackraftAddresses[CurrentTeamIndex]);
      break;

    case ES_NEW_KEY:
    {
      char key = (char)ThisEvent.EventParam;

      switch (key) {
        case '1':
          CurrentTeamIndex = 0;
          break;

        case '2':
          CurrentTeamIndex = 1;
          break;

        case '3':
          CurrentTeamIndex = 2;
          break;

        case '4':
          CurrentTeamIndex = 3;
          break;

        case '5':
          CurrentTeamIndex = 4;
          break;

        case 'w':
          CurrentJoy1 = CONTROLLER_COM_JOY_MAX;
          CurrentJoy2 = CONTROLLER_COM_JOY_CENTER;
          CurrentDigi = 0x00;
          SendCurrentDriveCommand();
          break;

        case 's':
          CurrentJoy1 = CONTROLLER_COM_JOY_MIN;
          CurrentJoy2 = CONTROLLER_COM_JOY_CENTER;
          CurrentDigi = 0x00;
          SendCurrentDriveCommand();
          break;

        case 'a':
          CurrentJoy1 = CONTROLLER_COM_JOY_CENTER;
          CurrentJoy2 = CONTROLLER_COM_JOY_MIN;
          CurrentDigi = 0x00;
          SendCurrentDriveCommand();
          break;

        case 'd':
          CurrentJoy1 = CONTROLLER_COM_JOY_CENTER;
          CurrentJoy2 = CONTROLLER_COM_JOY_MAX;
          CurrentDigi = 0x00;
          SendCurrentDriveCommand();
          break;

        case 'x':
          SetIdleCommand();
          ControllerCom_SendIdle(QuackraftAddresses[CurrentTeamIndex]);
          break;

        case 'c':
          ControllerCom_SendCharging(QuackraftAddresses[CurrentTeamIndex]);
          break;

        case 'p':
          ControllerCom_SendPairing(QuackraftAddresses[CurrentTeamIndex],
                                    MallardAddresses[MY_TEAM_INDEX]);
          break;

        case 'q':
          CurrentJoy1 = CONTROLLER_COM_JOY_CENTER;
          CurrentJoy2 = CONTROLLER_COM_JOY_CENTER;
          CurrentDigi = CONTROLLER_COM_BUTTON_COLLECT;
          SendCurrentDriveCommand();
          break;

        case 'e':
          CurrentJoy1 = CONTROLLER_COM_JOY_CENTER;
          CurrentJoy2 = CONTROLLER_COM_JOY_CENTER;
          CurrentDigi = CONTROLLER_COM_BUTTON_SMACK;
          SendCurrentDriveCommand();
          break;

        default:
          break;
      }

      break;
    }

    case ES_BOAT_ACK:
      DB_printf("Boat ACK / charge byte: 0x%02X\r\n", ThisEvent.EventParam);
      break;

    default:
      break;
  }

  return ReturnEvent;
}


/***************************************************************************
 private functions
 ***************************************************************************/

static void SendCurrentDriveCommand(void)
{
  ControllerCom_SendDriving(QuackraftAddresses[CurrentTeamIndex],
                            CurrentJoy1,
                            CurrentJoy2,
                            CurrentDigi);
}


static void SetIdleCommand(void)
{
  CurrentJoy1 = CONTROLLER_COM_JOY_CENTER;
  CurrentJoy2 = CONTROLLER_COM_JOY_CENTER;
  CurrentDigi = 0x00;
}
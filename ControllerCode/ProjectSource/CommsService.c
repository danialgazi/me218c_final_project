/****************************************************************************
 Module
   CommsService.c
****************************************************************************/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "CommsService.h"
#include "ControllerCom.h"
#include "ADService.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/

#define TEST_QUACKRAFT_ADDRESS     0x2084
#define TEST_MALLARD_ADDRESS       0x2184
#define ALERT_PERIOD  200 // 200 ms between alerts to the boat (5hz)

/*---------------------------- Module Variables ---------------------------*/

static uint8_t MyPriority;
static bool joystickDrive = false;

/*------------------------------ Module Code ------------------------------*/

bool InitCommsService(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;

    DB_printf("\r\n--- Controller Comms Test Init ---\r\n");

    if (ControllerCom_InitUART()) {
        DB_printf("Controller UART initialized\r\n");
    } else {
        DB_printf("Controller UART init FAILED\r\n");
    }

    DB_printf("Target boat address: %d\r\n", TEST_QUACKRAFT_ADDRESS);
    DB_printf("This controller address: %d\r\n", TEST_MALLARD_ADDRESS);

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
            DB_printf("\r\nController controls:\r\n");
            DB_printf("  p = send pairing packet\r\n");
            DB_printf("  w = drive forward\r\n");
            DB_printf("  s = drive backward\r\n");
            DB_printf("  a = turn left\r\n");
            DB_printf("  d = turn right\r\n");
            DB_printf("  x = idle / stop\r\n");
            DB_printf("  c = send charging packet\r\n");
            DB_printf("  j = drive with joystick\r\n");
            DB_printf("--------------------------------\r\n");
            ES_Timer_InitTimer(COMMS_TIMER, ALERT_PERIOD);
            break;

        case ES_NEW_KEY:
        {
            char key = (char)ThisEvent.EventParam;

            DB_printf("\r\nController key: %c\r\n", key);

            switch (key) {
                case 'p':
                    DB_printf("Sending PAIRING packet\r\n");
                    DB_printf("  Test boat: %d\r\n", TEST_QUACKRAFT_ADDRESS);
                    DB_printf("  Source controller: %d\r\n", TEST_MALLARD_ADDRESS);

                    ControllerCom_SendPairing(TEST_QUACKRAFT_ADDRESS,
                                              TEST_MALLARD_ADDRESS);
                    break;

                case 'w':
                    DB_printf("Sending DRIVING packet: FORWARD\r\n");
                    ControllerCom_SendDriving(TEST_QUACKRAFT_ADDRESS,
                                              CONTROLLER_COM_JOY_MAX,
                                              CONTROLLER_COM_JOY_CENTER,
                                              0x00);
                    break;

                case 's':
                    DB_printf("Sending DRIVING packet: BACKWARD\r\n");
                    ControllerCom_SendDriving(TEST_QUACKRAFT_ADDRESS,
                                              CONTROLLER_COM_JOY_MIN,
                                              CONTROLLER_COM_JOY_CENTER,
                                              0x00);
                    break;

                case 'a':
                    DB_printf("Sending DRIVING packet: LEFT\r\n");
                    ControllerCom_SendDriving(TEST_QUACKRAFT_ADDRESS,
                                              CONTROLLER_COM_JOY_CENTER,
                                              CONTROLLER_COM_JOY_MIN,
                                              0x00);
                    break;

                case 'd':
                    DB_printf("Sending DRIVING packet: RIGHT\r\n");
                    ControllerCom_SendDriving(TEST_QUACKRAFT_ADDRESS,
                                              CONTROLLER_COM_JOY_CENTER,
                                              CONTROLLER_COM_JOY_MAX,
                                              0x00);
                    break;

                case 'x':
                    DB_printf("Sending IDLE packet\r\n");
                    ControllerCom_SendIdle(TEST_QUACKRAFT_ADDRESS);
                    break;

                case 'c':
                    DB_printf("Sending CHARGING packet\r\n");
                    ControllerCom_SendCharging(TEST_QUACKRAFT_ADDRESS);
                    break;
                  case 'j':
                    joystickDrive = !joystickDrive;
                    if (joystickDrive) {
                        DB_printf("Joystick drive ENABLED\r\n");
                    } else {
                        DB_printf("Joystick drive DISABLED\r\n");
                    }
                    break;

                default:
                    DB_printf("Unknown key. Use p, w, s, a, d, x, c\r\n");
                    break;
            }

            break;
        }

        case ES_BOAT_ACK:
            DB_printf("\r\nController received BOAT ACK\r\n");
            DB_printf("  Charge/ACK byte: %d (%u)\r\n",
                      ThisEvent.EventParam,
                      ThisEvent.EventParam);

            if (ThisEvent.EventParam == CONTROLLER_COM_PAIRING_SUCCESS) {
                DB_printf("  Pairing success byte received\r\n");
            }
            break;
          case ES_TIMEOUT:
            if (ThisEvent.EventParam == COMMS_TIMER) {
              if (joystickDrive) {
                ControllerCom_SendDriving(TEST_QUACKRAFT_ADDRESS,
                                          ADC10ToByte(getJoystickY()),
                                          ADC10ToByte(getJoystickX()),
                                          0x00);
                DB_printf("Joystick drive packet sent, values: %u, %u\r\n", ADC10ToByte(getJoystickY()), ADC10ToByte(getJoystickX()));
              }
              ES_Timer_InitTimer(COMMS_TIMER, ALERT_PERIOD);
            }
            break;
        default:
            break;
    }

    return ReturnEvent;
}
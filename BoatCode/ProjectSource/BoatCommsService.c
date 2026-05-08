/****************************************************************************
 Module
   BoatCommsService.c
****************************************************************************/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BoatCommsService.h"
#include "BoatCom.h"

/*----------------------------- Module Defines ----------------------------*/

#define DEFAULT_MALLARD_INDEX     0
#define TEST_CHARGE_BYTE          0xFF

/*---------------------------- Module Variables ---------------------------*/

static uint8_t MyPriority;
static uint16_t PairedMallardAddress = 0;

/*---------------------------- Private Functions --------------------------*/

static void HandleControllerPacket(void);

/*------------------------------ Module Code ------------------------------*/

bool InitBoatCommsService(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;

    BoatCom_InitUART();

    PairedMallardAddress = MallardAddresses[DEFAULT_MALLARD_INDEX];

    ThisEvent.EventType = ES_INIT;
    return ES_PostToService(MyPriority, ThisEvent);
}


bool PostBoatCommsService(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}


ES_Event_t RunBoatCommsService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;

    switch (ThisEvent.EventType) {
        case ES_INIT:
            break;

        case ES_CONTROLLER_PACKET:
            HandleControllerPacket();
            break;

        case ES_NEW_KEY:
        {
            char key = (char)ThisEvent.EventParam;

            switch (key) {
                case 't':
                    BoatCom_SendTestAck();
                    DB_printf("Forced test ACK sent\r\n");
                    break;

                case '1':
                    PairedMallardAddress = MallardAddresses[0];
                    break;

                case '2':
                    PairedMallardAddress = MallardAddresses[1];
                    break;

                case '3':
                    PairedMallardAddress = MallardAddresses[2];
                    break;

                case '4':
                    PairedMallardAddress = MallardAddresses[3];
                    break;

                case '5':
                    PairedMallardAddress = MallardAddresses[4];
                    break;

                default:
                    break;
            }

            break;
        }

        default:
            break;
    }

    return ReturnEvent;
}


/***************************************************************************
 private functions
 ***************************************************************************/

static void HandleControllerPacket(void)
{
    BoatCom_Command_t command = BoatCom_GetLatestCommand();

    switch (command.statusByte) {
        case BOAT_COM_STATUS_PAIRING:
            PairedMallardAddress = command.sourceMallardAddress;
            BoatCom_SendAck(PairedMallardAddress, BOAT_COM_PAIRING_SUCCESS);
            DB_printf("Pairing packet received. ACK sent\r\n");
            break;

        case BOAT_COM_STATUS_CHARGING:
            BoatCom_SendAck(PairedMallardAddress, TEST_CHARGE_BYTE);
            DB_printf("Charging packet received. ACK sent\r\n");
            break;

        case BOAT_COM_STATUS_DRIVING:
            DB_printf("Driving packet: Joy1=%u Joy2=%u Digi=0x%02X\r\n",
                      command.joy1Byte,
                      command.joy2Byte,
                      command.digiByte);
            break;

        default:
            break;
    }
}
/****************************************************************************
 Module
   BoatCommsService.c
****************************************************************************/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BoatCommsService.h"
#include "BoatCom.h"

/*----------------------------- Module Defines ----------------------------*/

#define TEST_QUACKRAFT_ADDRESS     0x2083
#define TEST_MALLARD_ADDRESS       0x2183
#define TEST_CHARGE_BYTE           0x55

/*---------------------------- Module Variables ---------------------------*/

static uint8_t MyPriority;
static uint16_t PairedMallardAddress = TEST_MALLARD_ADDRESS;

/*------------------------------ Module Code ------------------------------*/

bool InitBoatCommsService(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;

    DB_printf("\r\n--- Boat Comms Test Init ---\r\n");

    if (BoatCom_InitUART()) {
        DB_printf("Boat UART initialized\r\n");
    } else {
        DB_printf("Boat UART init FAILED\r\n");
    }

    PairedMallardAddress = TEST_MALLARD_ADDRESS;

    DB_printf("This boat address: %d\r\n", TEST_QUACKRAFT_ADDRESS);
    DB_printf("Expected controller address: %d\r\n", TEST_MALLARD_ADDRESS);

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
            DB_printf("\r\nBoat controls:\r\n");
            DB_printf("  t = force ACK to controller %d\r\n", TEST_MALLARD_ADDRESS);
            DB_printf("--------------------------------\r\n");
            break;

        case ES_PAIRING_COMMAND:
            PairedMallardAddress = ThisEvent.EventParam;

            DB_printf("\r\nPAIRING event received\r\n");
            DB_printf("Mallard address param: %d\r\n", PairedMallardAddress);

            if (PairedMallardAddress == TEST_MALLARD_ADDRESS) {
                DB_printf("Address matches expected controller\r\n");
            } else {
                DB_printf("Unexpected controller address\r\n");
            }

            DB_printf("Sending pairing ACK: %d\r\n", BOAT_COM_PAIRING_SUCCESS);
            BoatCom_SendAck(PairedMallardAddress, BOAT_COM_PAIRING_SUCCESS);
            break;

        case ES_CHARGING_COMMAND:
            DB_printf("\r\nCHARGING event received\r\n");
            DB_printf("ACK to Mallard: %d\r\n", PairedMallardAddress);
            BoatCom_SendAck(PairedMallardAddress, TEST_CHARGE_BYTE);
            break;

        case ES_DRIVING_COMMAND:
        {
            BoatCom_Command_t command = BoatCom_GetLatestCommand();

            DB_printf("\r\nDRIVING event received\r\n");
            DB_printf("Joy1: %d\r\n", command.joy1Byte);
            DB_printf("Joy2: %d\r\n", command.joy2Byte);
            DB_printf("Digi: %d\r\n", command.digiByte);

            if (command.joy1Byte == BOAT_COM_JOY_CENTER &&
                command.joy2Byte == BOAT_COM_JOY_CENTER &&
                command.digiByte == 0) {
                DB_printf("Action: idle\r\n");
            } else if (command.joy1Byte > BOAT_COM_JOY_CENTER) {
                DB_printf("Action: forward\r\n");
            } else if (command.joy1Byte < BOAT_COM_JOY_CENTER) {
                DB_printf("Action: backward\r\n");
            } else if (command.joy2Byte > BOAT_COM_JOY_CENTER) {
                DB_printf("Action: right\r\n");
            } else if (command.joy2Byte < BOAT_COM_JOY_CENTER) {
                DB_printf("Action: left\r\n");
            }

            if (command.digiByte & 0x01) {
                DB_printf("Collect bit set\r\n");
            }

            if (command.digiByte & 0x02) {
                DB_printf("Smack bit set\r\n");
            }

            DB_printf("Sending drive ACK to %d\r\n", PairedMallardAddress);
            BoatCom_SendAck(PairedMallardAddress, TEST_CHARGE_BYTE);
            break;
        }

        case ES_NEW_KEY:
        {
            char key = (char)ThisEvent.EventParam;

            if (key == 't') {
                DB_printf("\r\nForce sending ACK\r\n");
                DB_printf("Dest controller: %d\r\n", TEST_MALLARD_ADDRESS);
                DB_printf("ACK byte: %d\r\n", TEST_CHARGE_BYTE);

                BoatCom_SendAck(TEST_MALLARD_ADDRESS, TEST_CHARGE_BYTE);
            } else {
                DB_printf("\r\nUnknown key: %c\r\n", key);
                DB_printf("Use t to force ACK\r\n");
            }

            break;
        }

        default:
            break;
    }

    return ReturnEvent;
}
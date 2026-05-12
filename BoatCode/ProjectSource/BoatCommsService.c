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
#define TEST_CHARGE_BYTE           0xFF

/*---------------------------- Module Variables ---------------------------*/

static uint8_t MyPriority;
static uint16_t PairedMallardAddress = TEST_MALLARD_ADDRESS;

/*---------------------------- Private Functions --------------------------*/

static void HandleControllerPacket(void);

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

        case ES_CONTROLLER_PACKET:
            DB_printf("\r\nBoat received valid controller packet\r\n");
            HandleControllerPacket();
            break;

        case ES_NEW_KEY:
        {
            char key = (char)ThisEvent.EventParam;

            if (key == 't') {
                DB_printf("\r\nForce sending ACK to controller\r\n");
                DB_printf("  Destination controller: %d\r\n", TEST_MALLARD_ADDRESS);
                DB_printf("  ACK byte: %d\r\n", TEST_CHARGE_BYTE);

                BoatCom_SendAck(TEST_MALLARD_ADDRESS, TEST_CHARGE_BYTE);
            } else {
                DB_printf("\r\nUnknown boat test key: %c\r\n", key);
                DB_printf("Use t to force ACK\r\n");
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

    DB_printf("Packet fields:\r\n");
    DB_printf("  Status: %d\r\n", command.statusByte);
    DB_printf("  Joy1:    (%u)\r\n", command.joy1Byte);
    DB_printf("  Joy2:   (%u)\r\n", command.joy2Byte);
    DB_printf("  Digi:   %d\r\n", command.digiByte);

    switch (command.statusByte) {
        case BOAT_COM_STATUS_PAIRING:
            PairedMallardAddress = command.sourceMallardAddress;

            DB_printf("Decoded command: PAIRING\r\n");
            DB_printf("  Controller address from packet: %d\r\n",
                      PairedMallardAddress);

            if (PairedMallardAddress == TEST_MALLARD_ADDRESS) {
                DB_printf("  Address matches expected controller\r\n");
            } else {
                DB_printf("  WARNING: unexpected controller address\r\n");
            }

            DB_printf("  Sending pairing ACK: %d\r\n",
                      BOAT_COM_PAIRING_SUCCESS);

            BoatCom_SendAck(PairedMallardAddress, BOAT_COM_PAIRING_SUCCESS);
            break;

        case BOAT_COM_STATUS_DRIVING:
            DB_printf("Decoded command: DRIVING\r\n");

            if (command.joy1Byte == BOAT_COM_JOY_CENTER &&
                command.joy2Byte == BOAT_COM_JOY_CENTER &&
                command.digiByte == 0x00) {
                DB_printf("  Boat action: IDLE / STOP\r\n");
            } else if (command.joy1Byte > BOAT_COM_JOY_CENTER) {
                DB_printf("  Boat action: DRIVING FORWARD\r\n");
            } else if (command.joy1Byte < BOAT_COM_JOY_CENTER) {
                DB_printf("  Boat action: DRIVING BACKWARD\r\n");
            } else if (command.joy2Byte > BOAT_COM_JOY_CENTER) {
                DB_printf("  Boat action: TURNING RIGHT\r\n");
            } else if (command.joy2Byte < BOAT_COM_JOY_CENTER) {
                DB_printf("  Boat action: TURNING LEFT\r\n");
            }

            if (command.digiByte & 0x01) {
                DB_printf("  Button action: COLLECT bit set\r\n");
            }

            if (command.digiByte & 0x02) {
                DB_printf("  Button action: SMACK bit set\r\n");
            }

            DB_printf("  Sending driving ACK to %dX\r\n",
                      PairedMallardAddress);

            BoatCom_SendAck(PairedMallardAddress, TEST_CHARGE_BYTE);
            break;

        case BOAT_COM_STATUS_CHARGING:
            DB_printf("Decoded command: CHARGING\r\n");
            DB_printf("  Sending charge ACK to %d\r\n",
                      PairedMallardAddress);

            BoatCom_SendAck(PairedMallardAddress, TEST_CHARGE_BYTE);
            break;

        default:
            DB_printf("Unknown command status: %d\r\n",
                      command.statusByte);
            break;
    }
}
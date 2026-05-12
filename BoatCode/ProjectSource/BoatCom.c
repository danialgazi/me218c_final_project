#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BoatCom.h"
#include <xc.h>
#include <sys/attribs.h> // keep these includes in for some reason they are needed for  the isr

static volatile uint8_t rxPacket[BOAT_COM_RX_PACKET_SIZE];
static volatile uint8_t rxIndex = 0;

static volatile uint8_t txPacket[BOAT_COM_TX_PACKET_SIZE];
static volatile uint8_t txIndex = 0;
static volatile uint8_t txLength = 0;
static volatile bool txBusy = false;

static volatile BoatCom_Command_t latestCommand;

const uint16_t QuackraftAddresses[BOAT_COM_NUM_TEAMS] = {
    0x2081,
    0x2082,
    0x2083,
    0x2084,
    0x2085
};

const uint16_t MallardAddresses[BOAT_COM_NUM_TEAMS] = {
    0x2181,
    0x2182,
    0x2183,
    0x2184,
    0x2185
};


bool BoatCom_InitUART(void)
{
    U2MODEbits.UARTEN = 0;
    
    U2STAbits.OERR = 0;
        volatile uint8_t dummy;

        while (U2STAbits.URXDA) {
            dummy = U2RXREG;
        }

    // UART TX: RB14 -> U2TX
    ANSELBbits.ANSB14 = 0;
    TRISBbits.TRISB14 = 0;
    RPB14Rbits.RPB14R = 0b0010;

    // UART RX: RB5 -> U2RX
//    ANSELBbits.ANSB5 = 0; it has no analog functionality
    TRISBbits.TRISB5 = 1;
    U2RXRbits.U2RXR = 0b0001; // double checked

    U2MODEbits.STSEL = 0;
    U2MODEbits.PDSEL = 0;
    U2BRG = 130;

    U2STAbits.URXISEL = 0b00;
    U2STAbits.UTXISEL = 0b00;

    IEC1bits.U2RXIE = 0;
    IEC1bits.U2TXIE = 0;

    IFS1bits.U2RXIF = 0;
    IFS1bits.U2TXIF = 0;

    IPC9bits.U2IP = 4;
    IPC9bits.U2IS = 0;

    U2STAbits.UTXEN = 1;
    U2STAbits.URXEN = 1;
    U2MODEbits.UARTEN = 1;

    IEC1bits.U2RXIE = 1;
    __builtin_enable_interrupts();

    return true;
}


BoatCom_Command_t BoatCom_GetLatestCommand(void)
{
    BoatCom_Command_t command;

    command.sourceMallardAddress = latestCommand.sourceMallardAddress;
    command.statusByte = latestCommand.statusByte;
    command.joy1Byte = latestCommand.joy1Byte;
    command.joy2Byte = latestCommand.joy2Byte;
    command.digiByte = latestCommand.digiByte;

    return command;
}


uint8_t BoatCom_CalculateChecksum(uint8_t *data, uint8_t length)
{
    uint8_t sum = 0;

    for (uint8_t i = 0; i < length; i++) {
        sum += data[i];
    }

    return 0xFF - sum;
}


bool BoatCom_CheckControllerPacket(uint8_t *packet)
{
    uint8_t sum = 0;
    uint16_t sourceAddress;

//    DB_printf("CHK start\r\n");

    if (packet[0] != BOAT_COM_START_DELIMITER) {
        DB_printf("BAD start %d\r\n", packet[0]);
        return false;
    }

    if (packet[1] != BOAT_COM_RX_LENGTH_MSB ||
        packet[2] != BOAT_COM_RX_LENGTH_LSB) {
        DB_printf("BAD len %d %d\r\n", packet[1], packet[2]);
        return false;
    }

    if (packet[3] != BOAT_COM_RX_API_ID) {
        DB_printf("BAD api %d\r\n", packet[3]);
        return false;
    }

    sourceAddress = ((uint16_t)packet[4] << 8) | packet[5];

//    DB_printf("src %d\r\n", sourceAddress);
//    DB_printf("rssi %d\r\n", packet[6]);
//    DB_printf("opts %d\r\n", packet[7]);

    for (uint8_t i = 3; i < BOAT_COM_RX_PACKET_SIZE; i++) {
        sum += packet[i];
    }

//    DB_printf("sum %d\r\n", sum);

    if (sum != 255) {
        DB_printf("BAD sum\r\n");
        return false;
    }

//    DB_printf("CHK good\r\n");
    return true;
}

static void BoatCom_ProcessControllerPacket(void)
{
    ES_Event_t event;

//    DB_printf("PROC pkt\r\n");

    if (BoatCom_CheckControllerPacket((uint8_t *)rxPacket)) {
        latestCommand.statusByte = rxPacket[8];
        latestCommand.joy1Byte = rxPacket[9];
        latestCommand.joy2Byte = rxPacket[10];
        latestCommand.digiByte = rxPacket[11];

//        DB_printf("status %d\r\n", latestCommand.statusByte);
//        DB_printf("joy1 %d\r\n", latestCommand.joy1Byte);
//        DB_printf("joy2 %d\r\n", latestCommand.joy2Byte);
//        DB_printf("digi %d\r\n", latestCommand.digiByte);

        switch (latestCommand.statusByte) {
            case BOAT_COM_STATUS_PAIRING:
                latestCommand.sourceMallardAddress =
                    ((uint16_t)latestCommand.joy1Byte << 8) |
                    latestCommand.joy2Byte;

//                DB_printf("pair addr %d\r\n",
//                          latestCommand.sourceMallardAddress);

                event.EventType = ES_PAIRING_COMMAND;
                event.EventParam = latestCommand.sourceMallardAddress;
                ES_PostAll(event);

//                DB_printf("post pair\r\n");
                break;

            case BOAT_COM_STATUS_CHARGING:
                event.EventType = ES_CHARGING_COMMAND;
                event.EventParam = 0;
                ES_PostAll(event);

//                DB_printf("post charge\r\n");
                break;

            case BOAT_COM_STATUS_DRIVING:
                event.EventType = ES_DRIVING_COMMAND;
                event.EventParam = 0;
                ES_PostAll(event);

//                DB_printf("post drive\r\n");
                break;

            default:
//                DB_printf("bad status %d\r\n", latestCommand.statusByte);
                break;
        }
    } else {
//        DB_printf("PROC bad\r\n");
    }
}


void BoatCom_SendAck(uint16_t mallardAddress, uint8_t chargeByte)
{
    if (txBusy) {
        return;
    }

    IEC1bits.U2TXIE = 0;

    txPacket[0] = BOAT_COM_START_DELIMITER;
    txPacket[1] = BOAT_COM_TX_LENGTH_MSB;
    txPacket[2] = BOAT_COM_TX_LENGTH_LSB;
    txPacket[3] = BOAT_COM_API_ID;
    txPacket[4] = BOAT_COM_FRAME_ID;
    txPacket[5] = (uint8_t)(mallardAddress >> 8);
    txPacket[6] = (uint8_t)(mallardAddress & 0xFF);
    txPacket[7] = BOAT_COM_OPTIONS;
    txPacket[8] = chargeByte;
    txPacket[9] = BoatCom_CalculateChecksum((uint8_t *)&txPacket[3], 6);

    txIndex = 0;
    txLength = BOAT_COM_TX_PACKET_SIZE;
    txBusy = true;

    U2TXREG = txPacket[txIndex];
    txIndex++;

    IFS1bits.U2TXIF = 0;
    IEC1bits.U2TXIE = 1;
}


void BoatCom_SendTestAck(void)
{
    BoatCom_SendAck(MallardAddresses[MY_TEAM_INDEX], BOAT_COM_PAIRING_SUCCESS);
}


void __ISR(_UART2_VECTOR, IPL4SOFT) UART2InterruptHandler(void)
{
    if (IFS1bits.U2RXIF) {
//        DB_printf("RX int\r\n");

        while (U2STAbits.URXDA) {
            uint8_t byte = U2RXREG;

//            DB_printf("b %d\r\n", byte);
//            DB_printf("i %d\r\n", rxIndex);

            if (byte == BOAT_COM_START_DELIMITER) {
//                DB_printf("start\r\n");
                rxIndex = 0;
            }

            if (rxIndex == 0 && byte != BOAT_COM_START_DELIMITER) {
//                DB_printf("skip\r\n");
                continue;
            }

            rxPacket[rxIndex] = byte;
            rxIndex++;

//            DB_printf("ni %d\r\n", rxIndex);

            if (rxIndex >= BOAT_COM_RX_PACKET_SIZE) {
//                DB_printf("full\r\n");
                BoatCom_ProcessControllerPacket();
                rxIndex = 0;
            }
        }

        if (U2STAbits.OERR) {
            DB_printf("OERR\r\n");
            U2STAbits.OERR = 0;
        }

        if (U2STAbits.FERR) {
            DB_printf("FERR\r\n");
        }

        IFS1bits.U2RXIF = 0;
    }

    if (IFS1bits.U2TXIF && IEC1bits.U2TXIE) {
//        DB_printf("TX int\r\n");

        while (!U2STAbits.UTXBF && txIndex < txLength) {
//            DB_printf("tx %d\r\n", txPacket[txIndex]);
            U2TXREG = txPacket[txIndex];
            txIndex++;
        }

        if (txIndex >= txLength) {
            IEC1bits.U2TXIE = 0;
            txBusy = false;
//            DB_printf("TX done\r\n");
        }

        IFS1bits.U2TXIF = 0;
    }
}
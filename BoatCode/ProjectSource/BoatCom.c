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
    return true; // testing for now - I don't actually know if we need all this debugging
    uint8_t sum = 0;
    uint16_t destinationAddress;

    if (packet[0] != BOAT_COM_START_DELIMITER) {
        return false;
    }

    if (packet[1] != BOAT_COM_RX_LENGTH_MSB ||
        packet[2] != BOAT_COM_RX_LENGTH_LSB) {
        return false;
    }

    if (packet[3] != BOAT_COM_API_ID) {
        return false;
    }

    if (packet[4] != BOAT_COM_FRAME_ID) {
        return false;
    }

    destinationAddress = ((uint16_t)packet[5] << 8) | packet[6];

    if (destinationAddress != QuackraftAddresses[MY_TEAM_INDEX]) {
        return false;
    }

    if (packet[7] != BOAT_COM_OPTIONS) {
        return false;
    }

    for (uint8_t i = 3; i < BOAT_COM_RX_PACKET_SIZE; i++) {
        sum += packet[i];
    }

    return (sum == 0xFF);
}


static void BoatCom_ProcessControllerPacket(void)
{
    if (BoatCom_CheckControllerPacket((uint8_t *)rxPacket)) {
        latestCommand.statusByte = rxPacket[8];
        latestCommand.joy1Byte = rxPacket[9];
        latestCommand.joy2Byte = rxPacket[10];
        latestCommand.digiByte = rxPacket[11];

        if (latestCommand.statusByte == BOAT_COM_STATUS_PAIRING) {
            latestCommand.sourceMallardAddress =
                ((uint16_t)latestCommand.joy1Byte << 8) | latestCommand.joy2Byte;
        }

        ES_Event_t event;
        event.EventType = ES_CONTROLLER_PACKET;
        event.EventParam = latestCommand.statusByte;
        ES_PostAll(event);
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
    if (IFS1bits.U2RXIF) { // I actually think the if statement needs to be on the inside - since 
        // the interrupt needs to trigger multiple times to build the full command
        // steps for tomorrow - hook it up to the logic analyzer, do some print statements in the isr
        // to verify we are building the right command (I think we are getting lost in our
        // error checking) and then verify the command is built correct
        DB_printf("we got an intterupt in the register!");
        while (U2STAbits.URXDA) {
            uint8_t byte = U2RXREG;

            if (byte == BOAT_COM_START_DELIMITER) {
                rxIndex = 0;
            }

            if (rxIndex == 0 && byte != BOAT_COM_START_DELIMITER) {
                continue;
            }

            rxPacket[rxIndex] = byte;
            rxIndex++;


        }
        if (rxIndex >= 9) {
                BoatCom_ProcessControllerPacket();
                rxIndex = 0;
            }

        if (U2STAbits.OERR) {
            U2STAbits.OERR = 0;
        }

        IFS1bits.U2RXIF = 0;
    }

    if (IFS1bits.U2TXIF && IEC1bits.U2TXIE) {
        while (!U2STAbits.UTXBF && txIndex < txLength) {
            U2TXREG = txPacket[txIndex];
            txIndex++;
        }

        if (txIndex >= txLength) {
            IEC1bits.U2TXIE = 0;
            txBusy = false;
        }

        IFS1bits.U2TXIF = 0;
    }
}
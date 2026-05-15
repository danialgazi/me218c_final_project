#include "ControllerCom.h"
#include "ES_Configure.h"
#include "ES_Framework.h"
#include <xc.h>
#include <sys/attribs.h> // keep these includes in for some reason they are needed for  the isr

static volatile uint8_t rxPacket[CONTROLLER_COM_RX_PACKET_SIZE];
static volatile uint8_t rxIndex = 0;
static volatile uint8_t lastChargeByte = 0;
static volatile uint8_t txPacket[CONTROLLER_COM_TX_PACKET_SIZE];
static volatile uint8_t txIndex = 0;
static volatile uint8_t txLength = 0;
static volatile bool txBusy = false;
static volatile uint8_t receivedByte = 0;
static volatile bool byteReceived = false;

const uint16_t QuackraftAddresses[CONTROLLER_COM_NUM_TEAMS] = {
    0x2081,
    0x2082,
    0x2083,
    0x2084,
    0x2085
};

const uint16_t MallardAddresses[CONTROLLER_COM_NUM_TEAMS] = {
    0x2181,
    0x2182,
    0x2183,
    0x2184,
    0x2185
};

// does uart init and configures the pins
bool ControllerCom_InitUART(void){
    U2MODEbits.UARTEN = 0; // Disable UART before configuration
    U2MODEbits.STSEL = 0; // 1 stop bit
    U2MODEbits.PDSEL = 0; // 8N1 (8 data bits, no parity)
    U2BRG = 130; // Baud rate setting for 9600 baud with Fcy = 20MHz
    U2MODEbits.UARTEN = 1; // Enable UART

    U2STAbits.UTXEN = 1; // Enable UART transmission
    U2STAbits.URXEN = 1; // Enable UART reception
    U2STAbits.URXISEL = 00; // interrupt on every recieved byte
    U2STAbits.UTXISEL = 00; // interrupt on every transmitted byte - used to send out the commands packet continiously

    // interrupt setupt for recieve
    IEC1bits.U2RXIE = 1; // Enable UART receive interrupt

    IFS1bits.U2RXIF = 0; // Clear receive interrupt flag
    IPC9bits.U2IP = 4; // Set UART receive interrupt priority (adjust as needed)

    // UART TX: RB14 -> U2TX
    ANSELBbits.ANSB14 = 0;
    TRISBbits.TRISB14 = 0;
    RPB14Rbits.RPB14R = 0b0010;

    // UART RX: RB5 -> U2RX
    //ANSELBbits.ANSB5 = 0;
    TRISBbits.TRISB5 = 1;
    U2RXRbits.U2RXR = 0b0001;

    return true;
}

void ControllerCom_BuildPacket(ControllerCom_Command_t command,
                               uint8_t *packetOut)
{
    packetOut[0] = CONTROLLER_COM_START_DELIMITER;
    packetOut[1] = CONTROLLER_COM_LENGTH_MSB;
    packetOut[2] = CONTROLLER_COM_LENGTH_LSB;
    packetOut[3] = CONTROLLER_COM_API_ID;
    packetOut[4] = CONTROLLER_COM_FRAME_ID;

    packetOut[5] = (uint8_t)(command.destinationAddress >> 8);
    packetOut[6] = (uint8_t)(command.destinationAddress & 0xFF);

    packetOut[7] = CONTROLLER_COM_OPTIONS;
    packetOut[8] = command.statusByte;
    packetOut[9] = command.joy1Byte;
    packetOut[10] = command.joy2Byte;
    packetOut[11] = command.digiByte;

    packetOut[12] = ControllerCom_CalculateChecksum(&packetOut[3], 9);
}

void ControllerCom_SendCommand(ControllerCom_Command_t command)
{
    if (txBusy) {
        return;
    }

    IEC1bits.U2TXIE = 0;

    ControllerCom_BuildPacket(command, (uint8_t *)txPacket);

    txIndex = 0;
    txLength = CONTROLLER_COM_TX_PACKET_SIZE;
    txBusy = true;

    U2TXREG = txPacket[txIndex];
    txIndex++;

    IFS1bits.U2TXIF = 0;
    IEC1bits.U2TXIE = 1;
}

uint8_t ControllerCom_CalculateChecksum(uint8_t *data, uint8_t length)
{
    uint8_t sum = 0;

    for (uint8_t i = 0; i < length; i++) {
        sum += data[i];
    }

    return 0xFF - sum;
}

void ControllerCom_SendDriving(uint16_t destinationAddress,
                               uint8_t joy1,
                               uint8_t joy2,
                               uint8_t digi)
{
    ControllerCom_Command_t command;

    command.destinationAddress = destinationAddress;
    command.statusByte = CONTROLLER_COM_STATUS_DRIVING;
    command.joy1Byte = joy1;
    command.joy2Byte = joy2;
    command.digiByte = digi;

    ControllerCom_SendCommand(command);
}


void ControllerCom_SendIdle(uint16_t destinationAddress)
{
    ControllerCom_SendDriving(destinationAddress,
                              CONTROLLER_COM_JOY_CENTER,
                              CONTROLLER_COM_JOY_CENTER,
                              0x00);
}


void ControllerCom_SendCharging(uint16_t destinationAddress)
{
    ControllerCom_Command_t command;

    command.destinationAddress = destinationAddress;
    command.statusByte = CONTROLLER_COM_STATUS_CHARGING;
    command.joy1Byte = 0x00;
    command.joy2Byte = 0x00;
    command.digiByte = 0x00;

    ControllerCom_SendCommand(command);
}

// we should edit these to set the mallard address as our perminant return address since it should be constant for our controller
void ControllerCom_SendPairing(uint16_t destinationAddress,
                               uint16_t mallardAddress)
{
    ControllerCom_Command_t command;

    command.destinationAddress = destinationAddress;
    command.statusByte = CONTROLLER_COM_STATUS_PAIRING;
    command.joy1Byte = (uint8_t)(mallardAddress >> 8);
    command.joy2Byte = (uint8_t)(mallardAddress & 0xFF);
    command.digiByte = 0x00;

    ControllerCom_SendCommand(command);
}

bool ControllerCom_CheckBoatPacket(uint8_t *packet)
{
    uint8_t sum = 0;
    uint16_t sourceAddress;

//    DB_printf("CHK boat\r\n");

    if (packet[0] != CONTROLLER_COM_START_DELIMITER) {
        DB_printf("BAD start %d\r\n", packet[0]);
        return false;
    }

    if (packet[1] != CONTROLLER_COM_RX_LENGTH_MSB ||
        packet[2] != CONTROLLER_COM_RX_LENGTH_LSB) {
        DB_printf("BAD len %d %d\r\n", packet[1], packet[2]);
        return false;
    }

    if (packet[3] != CONTROLLER_COM_RX_API_ID) {
        DB_printf("BAD api %d\r\n", packet[3]);
        return false;
    }

    sourceAddress = ((uint16_t)packet[4] << 8) | packet[5];
//
//    DB_printf("src %d\r\n", sourceAddress);
//    DB_printf("rssi %d\r\n", packet[6]);
//    DB_printf("opts %d\r\n", packet[7]);

    for (uint8_t i = 3; i < CONTROLLER_COM_RX_PACKET_SIZE; i++) {
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

static void ControllerCom_ProcessBoatPacket(void)
{
//    DB_printf("PROC boat\r\n");

    if (ControllerCom_CheckBoatPacket((uint8_t *)rxPacket)) {
        lastChargeByte = rxPacket[8];

//        DB_printf("ack %d\r\n", lastChargeByte);

        ES_Event_t event;
        event.EventType = ES_BOAT_ACK;
        event.EventParam = lastChargeByte;

        ES_PostAll(event);

//        DB_printf("post ack\r\n");
    } else {
        DB_printf("PROC bad\r\n");
    }
}

uint8_t ControllerCom_GetLastChargeByte(void)
{
    return lastChargeByte;
}

void __ISR(_UART2_VECTOR, IPL4SOFT) UART2InterruptHandler(void)
{
    if (IFS1bits.U2RXIF) {
//        DB_printf("RX int\r\n");

        while (U2STAbits.URXDA) {
            uint8_t byte = U2RXREG;

//            DB_printf("b %d\r\n", byte);
//            DB_printf("i %d\r\n", rxIndex);

            if (rxIndex == 0) {
                if (byte != CONTROLLER_COM_START_DELIMITER) {
                    DB_printf("weird packet start %d\r\n", byte);
                    continue;
                }
            }

            rxPacket[rxIndex] = byte;
            rxIndex++;

//            DB_printf("ni %d\r\n", rxIndex);

            if (rxIndex >= CONTROLLER_COM_RX_PACKET_SIZE) {
//                DB_printf("full\r\n");
                ControllerCom_ProcessBoatPacket();
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

    if (U2STAbits.OERR) {
        U2STAbits.OERR = 0;
    }
}
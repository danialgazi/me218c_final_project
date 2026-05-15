#ifndef CONTROLLER_COM_H
#define CONTROLLER_COM_H

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>
#include <sys/attribs.h>

/*******************************************************************************
 * Constants
 ******************************************************************************/

#define CONTROLLER_COM_START_DELIMITER      0x7E
#define CONTROLLER_COM_LENGTH_MSB           0x00
#define CONTROLLER_COM_LENGTH_LSB           0x09
#define CONTROLLER_COM_API_ID               0x01
#define CONTROLLER_COM_FRAME_ID             0x00
#define CONTROLLER_COM_OPTIONS              0x01

#define CONTROLLER_COM_TX_PACKET_SIZE       13

#define CONTROLLER_COM_JOY_MIN              0x00
#define CONTROLLER_COM_JOY_CENTER           0x7F
#define CONTROLLER_COM_JOY_MAX              0xFF

#define CONTROLLER_COM_STATUS_DRIVING       0x00
#define CONTROLLER_COM_STATUS_CHARGING      0x01
#define CONTROLLER_COM_STATUS_PAIRING       0x02

#define CONTROLLER_COM_BUTTON_COLLECT       0x01
#define CONTROLLER_COM_BUTTON_SMACK         0x02

#define CONTROLLER_COM_NUM_TEAMS            5

#define CONTROLLER_COM_RX_PACKET_SIZE       10

#define CONTROLLER_COM_RX_API_ID        0x81

#define CONTROLLER_COM_RX_LENGTH_MSB        0x00
#define CONTROLLER_COM_RX_LENGTH_LSB        0x06

#define CONTROLLER_COM_PAIRING_SUCCESS      0xFF
#define MY_TEAM_INDEX              3

/*******************************************************************************
 * Address Arrays
 ******************************************************************************/

extern const uint16_t QuackraftAddresses[CONTROLLER_COM_NUM_TEAMS];
extern const uint16_t MallardAddresses[CONTROLLER_COM_NUM_TEAMS];


/*******************************************************************************
 * Types
 ******************************************************************************/

typedef struct {
    uint16_t destinationAddress;
    uint8_t statusByte;
    uint8_t joy1Byte;
    uint8_t joy2Byte;
    uint8_t digiByte;
} ControllerCom_Command_t;


/*******************************************************************************
 * Public Functions
 ******************************************************************************/

bool ControllerCom_InitUART(void);

void ControllerCom_SendCommand(ControllerCom_Command_t command);

void ControllerCom_SendDriving(uint16_t destinationAddress,
                               uint8_t joy1,
                               uint8_t joy2,
                               uint8_t digi);

void ControllerCom_SendIdle(uint16_t destinationAddress);

void ControllerCom_SendCharging(uint16_t destinationAddress);

void ControllerCom_SendPairing(uint16_t destinationAddress,
                               uint16_t mallardAddress);

uint8_t ControllerCom_CalculateChecksum(uint8_t *data, uint8_t length);

void ControllerCom_BuildPacket(ControllerCom_Command_t command,
                               uint8_t *packetOut);

uint8_t ControllerCom_GetLastChargeByte(void);

bool ControllerCom_CheckBoatPacket(uint8_t *packet);

#endif /* CONTROLLER_COM_H */
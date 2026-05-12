#ifndef BOAT_COM_H
#define BOAT_COM_H

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>
#include <sys/attribs.h>

#define BOAT_COM_START_DELIMITER       0x7E

#define BOAT_COM_RX_PACKET_SIZE        13
#define BOAT_COM_TX_PACKET_SIZE        10

#define BOAT_COM_RX_LENGTH_MSB         0x00
#define BOAT_COM_RX_LENGTH_LSB         0x09

#define BOAT_COM_RX_API_ID             0x81

#define BOAT_COM_TX_LENGTH_MSB         0x00
#define BOAT_COM_TX_LENGTH_LSB         0x06

#define BOAT_COM_API_ID                0x01
#define BOAT_COM_FRAME_ID              0x00
#define BOAT_COM_OPTIONS               0x01

#define BOAT_COM_STATUS_DRIVING        0x00
#define BOAT_COM_STATUS_CHARGING       0x01
#define BOAT_COM_STATUS_PAIRING        0x02

#define BOAT_COM_PAIRING_SUCCESS       0xFF

#define BOAT_COM_NUM_TEAMS             5

#define MY_TEAM_INDEX                  2

extern const uint16_t QuackraftAddresses[BOAT_COM_NUM_TEAMS];
extern const uint16_t MallardAddresses[BOAT_COM_NUM_TEAMS];

typedef struct {
    uint16_t sourceMallardAddress;
    uint8_t statusByte;
    uint8_t joy1Byte;
    uint8_t joy2Byte;
    uint8_t digiByte;
} BoatCom_Command_t;

bool BoatCom_InitUART(void);

BoatCom_Command_t BoatCom_GetLatestCommand(void);

void BoatCom_SendAck(uint16_t mallardAddress, uint8_t chargeByte);

void BoatCom_SendTestAck(void);

bool BoatCom_CheckControllerPacket(uint8_t *packet);

uint8_t BoatCom_CalculateChecksum(uint8_t *data, uint8_t length);

#endif /* BOAT_COM_H */
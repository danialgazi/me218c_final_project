/****************************************************************************

  Header file for Controller flat state machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef ControllerFSM_H
#define ControllerFSM_H

#include "ES_Configure.h"
#include "ES_Types.h"

typedef enum
{
  ControllerInitPState,
  BoatSelect,
  Pairing,
  Driving,
  Refuel
} ControllerState_t;

bool InitControllerFSM(uint8_t Priority);
bool PostControllerFSM(ES_Event_t ThisEvent);
ES_Event_t RunControllerFSM(ES_Event_t ThisEvent);
ControllerState_t QueryControllerFSM(void);

#endif /* ControllerFSM_H */
